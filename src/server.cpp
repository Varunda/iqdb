/***************************************************************************\
    server.cpp - iqdb server (database maintenance and queries)

    Copyright (C) 2008 piespy@gmail.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
\**************************************************************************/

#include <csignal>
#include <cstddef>
#include <cstring>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>

#include <iqdb/debug.h>
#include <iqdb/haar_signature.h>
#include <iqdb/imgdb.h>
#include <iqdb/imglib.h>
#include <iqdb/types.h>

#include <httplib.h>
#include <nlohmann/json.hpp>

using httplib::Server;
using iqdb::IQDB;
using nlohmann::json;

namespace iqdb {

static Server server;

static void signal_handler(int signal, siginfo_t *info, void *ucontext) {
    INFO("received signal {} ({})\n", signal, strsignal(signal));

    if (signal == SIGSEGV) {
        INFO("address: {}\n", info->si_addr);
        DEBUG("{}", get_backtrace(2));
        exit(1);
    }

    if (server.is_running()) {
        server.stop();
    }
}

void install_signal_handlers() {
    struct sigaction action = {};
    sigfillset(&action.sa_mask);
    action.sa_flags = SA_RESTART | SA_SIGINFO;

    action.sa_sigaction = signal_handler;

    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGSEGV, &action, NULL);
}

void http_server(const std::string host, const int port, const std::string database_filename) {
    INFO("starting server...\n");

    std::shared_mutex mutex_;
    auto memory_db = std::make_unique<IQDB>(database_filename);
    INFO("created DB from {}\n", database_filename.c_str());

    install_signal_handlers();

    // GET /images/:id
    //    get the info about an image based on an image ID
    //
    server.Get("/images/:post_id", [&](const httplib::Request& request, httplib::Response& response) {
        std::unique_lock lock(mutex_);

        const postId post_id = request.path_params.at("post_id");
        INFO("getting post_id {}\n", post_id.c_str());
        std::optional<Image> image = memory_db->getImage(post_id);

        json data;
        if (image == std::nullopt) {
            data = { { "message", "not found" } };
            response.status = 404;
        } else {
            data = {
                { "post_id", post_id.c_str() },
                { "hash", image->haar().to_string() },
                { "avglf", { image->avglf1, image->avglf2, image->avglf3 }},
            };
        }

        response.set_content(data.dump(4), "application/json");
    });

    // POST /images/:post_id
    //      post a new image, creating a hash for it
    // must include a file named "file" as part of the POST request
    //    
    server.Post("/images/:post_id", [&](const httplib::Request& request, httplib::Response& response) {
        std::unique_lock lock(mutex_);

        if (!request.has_file("file")) {
            throw iqdb::param_error("`POST /images/:id` requires a `file` param");
        }

        const postId post_id = request.path_params.at("post_id");
        INFO("posting image [post_id='{}']\n", post_id);
        const auto &file = request.get_file_value("file");
        const HaarSignature signature = HaarSignature::from_file_content(file.content);
        memory_db->addImage(post_id, signature);

        json data = {
            {"post_id", post_id},
            {"hash", signature.to_string()},
            {"signature", {
                {"avglf", signature.avglf},
                {"sig", signature.sig},
            }}};

        response.set_content(data.dump(4), "application/json");
    });

    // DELETE /images/:id
    //      delete an image from the DB
    server.Delete("/images/(.+)", [&](const httplib::Request& request, httplib::Response& response) {
        std::unique_lock lock(mutex_);

        const postId post_id = request.matches[1];
        INFO("removing post from DB [post_id={}]\n", post_id);
        memory_db->removeImage(post_id);

        json data = {
            { "post_id", post_id },
        };

        response.set_content(data.dump(4), "application/json");
    });

    // POST /query
    //    include either :hash or :file
    //    can include ?limit to limit how many results are returned
    server.Post("/query", [&](const httplib::Request& request, httplib::Response& response) {
        std::shared_lock lock(mutex_);

        int limit = 10;
        sim_vector matches;
        json data = json::array();

        if (request.has_param("limit")) {
            limit = stoi(request.get_param_value("limit"));
        }

        if (request.has_param("hash")) {
            const std::string& hash = request.get_param_value("hash");
            HaarSignature haar = HaarSignature::from_hash(hash);
            matches = memory_db->queryFromSignature(haar, limit);
        } else if (request.has_file("file")) {
            const auto &file = request.get_file_value("file");
            matches = memory_db->queryFromBlob(file.content, limit);
        } else {
            throw param_error("`POST /query` requires a `file` or `hash` param");
        }

        for (const auto &match : matches) {
            std::optional<Image> image = memory_db->getImage(match.id);
            if (image == std::nullopt) {
                WARN("failed to find image {} from memory_db\n", match.id);
                continue;
            }

            const HaarSignature& haar = image->haar();

            data += {
                {"post_id", match.id},
                {"score", match.score},
                {"hash", haar.to_string()},
                {"signature", {
                    {"avglf", haar.avglf}
                    //{"sig", haar.sig},
                }
            }};
        }

        response.set_content(data.dump(4), "application/json");
    });

    // GET /status
    //    returns how many images are in the DB
    server.Get("/status", [&](const httplib::Request& request, httplib::Response& response) {
        std::shared_lock lock(mutex_);

        const size_t count = memory_db->getImgCount();
        json data = {
            { "images", count },
            { "version", "honooru" }
        };

        response.set_content(data.dump(4), "application/json");
    });

    server.set_logger([](const auto &req, const auto &res) {
        INFO("{} \"{} {} {}\" {} {}\n", req.remote_addr, req.method, req.path, req.version, res.status, res.body.size());
    });

    server.set_exception_handler([](const auto &req, auto &res, std::exception_ptr ep) {

        json data;
        try {
            std::rethrow_exception(ep);
        } catch (std::exception& e) {
            const auto name = demangle_name(typeid(e).name());
            const auto message = e.what();
            data = {
                {"exception", name},
                {"message", message},
                {"backtrace", last_exception_backtrace}
            };

            DEBUG("exception: {} ({})\n{}\n", name, message, last_exception_backtrace);
        } catch (...) {
            data = {
                {"message", "uncaught rethrow"}
            };
        }

        res.set_content(data.dump(4), "application/json");
        res.status = 500;
    });

    INFO("listening on {}:{}\n", host, port);
    server.listen(host.c_str(), port);
    INFO("stopping server...\n");
}

void help() {
    printf(
        "Usage: iqdb COMMAND [ARGS...]\n"
        "  iqdb http [host] [port] [dbfile]  Run HTTP server on given host/port.\n"
        "  iqdb help                         Show this help.\n");

    exit(0);
}

} // namespace iqdb
