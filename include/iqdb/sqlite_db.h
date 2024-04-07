#ifndef IQDB_SQLITE_DB_H
#define IQDB_SQLITE_DB_H

#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <vector>

#include <sqlite_orm/sqlite_orm.h>
#include <iqdb/haar_signature.h>
#include <iqdb/types.h>

namespace iqdb {

// A model representing an image signature stored in the SQLite database.
struct Image {
    postId post_id;        // The external (Danbooru) post ID.
    std::string md5;
    double avglf1;         // The `double avglf[3]` array.
    double avglf2;
    double avglf3;
    std::vector<char> sig; // The `int16_t sig[3][40]` array, stored as a binary blob.

    HaarSignature haar() const;
};

// Initialize the database, creating the table if it doesn't exist.
static auto initStorage(const std::string& path = ":memory:") {
    using namespace sqlite_orm;

    auto storage = make_storage(path,
        // index has to be made before the table, it's done in reverse order as input
        make_index("idx_images_md5", &Image::md5),
        make_table("images",
            make_column("post_id",  &Image::post_id, primary_key()),
            make_column("md5",  &Image::md5),
            make_column("avglf1",   &Image::avglf1),
            make_column("avglf2",   &Image::avglf2),
            make_column("avglf3",   &Image::avglf3),
            make_column("sig",      &Image::sig)
        )
    );

    storage.sync_schema();
    return storage;
}

// An SQLite database containing a table of image hashes.
class SqliteDB {
public:
    using Storage = decltype(initStorage());

    // Open database at path. Default to a temporary memory-only database.
    SqliteDB(const std::string& path = ":memory:") : storage_(initStorage(path)) {};

    // Get an image from the database, if it exists.
    std::optional<Image> getImage(postId post_id);

    std::vector<Image> getByMD5(const std::string& md5);

    // Add the image to the database. Replace the image if it already exists. Returns the internal IQDB id.
    void addImage(postId post_id, const std::string& md5, HaarSignature signature);

    // Remove the image from the database.
    void removeImage(postId post_id);

    // Call a function for each image in the database.
    void eachImage(std::function<void (const Image&)>);

private:
    // The SQLite database.
    Storage storage_;

    // A mutex around the database
    std::shared_mutex sql_mutex_;
};

}

#endif
