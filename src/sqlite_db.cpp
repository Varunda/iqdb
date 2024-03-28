#include <fstream>
#include <iostream>
#include <optional>
#include <vector>

#include <iqdb/debug.h>
#include <iqdb/imglib.h>
#include <iqdb/sqlite_db.h>
#include <iqdb/types.h>

namespace iqdb {

using namespace sqlite_orm;

HaarSignature Image::haar() const {
  lumin_t avglf = { avglf1, avglf2, avglf3 };
  return HaarSignature(avglf, *(signature_t*)sig.data());
}

void SqliteDB::eachImage(std::function<void (const Image&)> func) {
  for (auto& image : storage_.iterate<Image>()) {
    func(image);
  }
}

std::optional<Image> SqliteDB::getImage(postId post_id) {
  std::unique_lock lock(sql_mutex_);
  auto results = storage_.get_all<Image>(where(c(&Image::post_id) == post_id));

  if (results.size() == 1) {
    return results[0];
  } else {
    DEBUG("couldn't find post {} in sqlite database\n", post_id);
    return std::nullopt;
  }
}

void SqliteDB::addImage(postId post_id, HaarSignature signature) {
    const char* sig_ptr = (const char*)signature.sig;
    std::vector<char> sig_blob(sig_ptr, sig_ptr + sizeof(signature.sig));

    Image image = {
        post_id, signature.avglf[0], signature.avglf[1], signature.avglf[2], sig_blob
    };

    INFO("adding post to DB [post_id={}]\n", image.post_id);

    storage_.transaction([&] {
        removeImage(post_id);
        storage_.replace(image);
        return true;
    });
}

void SqliteDB::removeImage(postId post_id) {
  storage_.remove_all<Image>(where(c(&Image::post_id) == post_id));
}

}
