/***************************************************************************\
    imgdb.h - iqdb library API

    Copyright (C) 2008 piespy@gmail.com

    Originally based on imgSeek code, these portions
    Copyright (C) 2003 Ricardo Niederberger Cabral.

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

#ifndef IMGDBASE_H
#define IMGDBASE_H

#include <memory>
#include <string>
#include <vector>
#include <map>

#include <iqdb/haar.h>
#include <iqdb/haar_signature.h>
#include <iqdb/imglib.h>
#include <iqdb/resizer.h>
#include <iqdb/sqlite_db.h>
#include <iqdb/types.h>

namespace iqdb {

// Exceptions.
class base_error : public std::exception {
public:
    base_error(std::string what) noexcept : what_(what) {}
    const char* what() const noexcept { return what_.c_str(); }

    const std::string what_;
};

#define DEFINE_ERROR(derived, base)                            \
    class derived : public base {                                \
    public:                                                      \
        derived(std::string what) noexcept : base(what) {}         \
    };

// Fatal error, cannot recover.
DEFINE_ERROR(fatal_error, base_error)

// Non-fatal, may retry the call after correcting the problem, and continue using the library.
DEFINE_ERROR(simple_error, base_error)
DEFINE_ERROR(param_error, simple_error) // An argument was invalid, e.g. non-existent image ID.
DEFINE_ERROR(image_error, simple_error) // Could not successfully extract image data from the given file.

typedef struct {
    Score v[3];
} lumin_native;

struct sim_value {
    postId id;
    Score score;
    sim_value(postId id_, Score score_) : id(id_), score(score_) {};
    bool operator<(const sim_value &other) const { return score < other.score; }
};

struct image_info {
    image_info() {}
    image_info(postId i, const std::string& md5, const lumin_native &a)
        : id(i), md5(md5), avgl(a) {}

    postId id;
    std::string md5;
    lumin_native avgl;
};

typedef std::vector<sim_value> sim_vector;
typedef Idx sig_t[NUM_COEFS];

class IQDB {
public:
    IQDB(std::string filename = ":memory:");

    // query for similar images by hash string
    sim_vector queryFromSignature(const HaarSignature& img, size_t numres = 10);

    // query for similar images by binary blob
    sim_vector queryFromBlob(const std::string blob, int numres = 10);

    // get how many images are stored in this DB
    uint64_t getImgCount();

    // check if a post is deleted or not
    bool isDeleted(postId iqdb_id); 

    // add a new image to the DB
    void addImage(postId id, const std::string& md5, const HaarSignature& signature);

    // get an image from the DB. will be std::nullopt if not found
    std::optional<Image> getImage(postId post_id);

    // get all images from the DB with the matching md5 hash
    std::vector<Image> getByMD5(const std::string& md5);

    // remove an image from the DB
    void removeImage(postId id);

    // load an sqlite DB from file
    void loadDatabase(std::string filename);

private:
    // cache a post in memory
    void addImageInMemory(postId post_id, const std::string& md5, const HaarSignature& signature);

    // cached data
    std::map<postId, image_info> m_info;

    // SQLite DB that is operated on
    std::unique_ptr<SqliteDB> sqlite_db_;

    bucket_set imgbuckets;

    // how many images are stored in the DB
    uint64_t img_count = 0;

private:
    void operator=(const IQDB &);

};

}

#endif
