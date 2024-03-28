/***************************************************************************\
    imgdb.cpp - iqdb library implementation

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

#include <sys/mman.h>

#include <algorithm>
#include <memory>
#include <vector>

#include <iqdb/debug.h>
#include <iqdb/imgdb.h>
#include <iqdb/imglib.h>
#include <iqdb/haar_signature.h>
#include <iqdb/sqlite_db.h>

namespace iqdb {

void bucket_set::add(const HaarSignature &sig, postId iqdb_id) {
  eachBucket(sig, [&](bucket_t& bucket) {
    bucket.push_back(iqdb_id);
  });
}

void bucket_set::remove(const HaarSignature &sig, postId iqdb_id) {
    eachBucket(sig, [&](bucket_t& bucket) {
        // https://en.wikipedia.org/wiki/Erase-remove_idiom
        bucket.erase(std::remove(bucket.begin(), bucket.end(), iqdb_id), bucket.end());
    });
}

bucket_t& bucket_set::at(int color, int coef) {
    const int sign = coef < 0;
    return buckets[color][sign][abs(coef)];
}

void bucket_set::eachBucket(const HaarSignature &sig, std::function<void(bucket_t&)> func) {
    for (int c = 0; c < sig.num_colors(); c++) {
        for (int i = 0; i < NUM_COEFS; i++) {
            const int coef = sig.sig[c][i];
            auto& bucket = at(c, coef);
            func(bucket);
        }
    }
}

void IQDB::addImage(postId post_id, const HaarSignature& haar) {
    removeImage(post_id);
    sqlite_db_->addImage(post_id, haar);
    addImageInMemory(post_id, haar);

    DEBUG("Added post {} to memory and database (haar={})\n", post_id, haar.to_string());
}

void IQDB::addImageInMemory(postId post_id, const HaarSignature& haar) {
    imgbuckets.add(haar, post_id);
    img_count++;

    image_info info;
    info.id = post_id;
    info.avgl.v[0] = static_cast<Score>(haar.avglf[0]);
    info.avgl.v[1] = static_cast<Score>(haar.avglf[1]);
    info.avgl.v[2] = static_cast<Score>(haar.avglf[2]);

    m_info.emplace(post_id, info);
}

void IQDB::loadDatabase(std::string filename) {
    sqlite_db_ = std::make_unique<SqliteDB>(filename);
    m_info.clear();
    imgbuckets = bucket_set();

    sqlite_db_->eachImage([&](const iqdb::Image& image) {
        addImageInMemory(image.post_id, image.haar());

        if (img_count % 250000 == 0) {
            INFO("loaded image (post {})...\n", image.post_id);
        }
    });

    INFO("loaded {} images from {}\n", getImgCount(), filename);
}

bool IQDB::isDeleted(postId iqdb_id) {
    return !m_info.at(iqdb_id).avgl.v[0];
}

std::optional<Image> IQDB::getImage(postId post_id) {
    return sqlite_db_->getImage(post_id);
}

sim_vector IQDB::queryFromBlob(const std::string blob, int numres) {
    HaarSignature signature = HaarSignature::from_file_content(blob);
    return queryFromSignature(signature, numres);
}

sim_vector IQDB::queryFromSignature(const HaarSignature &signature, size_t numres) {
  Score scale = 0;
  //std::vector<Score> scores(m_info.size(), 0);
  std::map<postId, Score> scores;
  std::priority_queue<sim_value> pqResults; /* results priority queue; largest at top */
  sim_vector V; /* output results */

  DEBUG("querying signature={} json={}\n", signature.to_string(), signature.to_json());

  // Luminance score (DC coefficient).
  for (auto const& elem : m_info) {
    const image_info& image_info = elem.second;
    Score s = 0;

    for (int c = 0; c < signature.num_colors(); c++) {
        s += weights[0][c] * std::abs(image_info.avgl.v[c] - static_cast<Score>(signature.avglf[c]));
    }

    scores.emplace(elem.first, s);
  }

/*
  // Luminance score (DC coefficient).
  for (size_t i = 0; i < scores.size(); i++) {
    auto image_info = m_info[i];
    Score s = 0;

    for (int c = 0; c < signature.num_colors(); c++) {
      s += weights[0][c] * std::abs(image_info.avgl.v[c] - static_cast<Score>(signature.avglf[c]));
    }

    scores[i] = s;
  }
*/

    for (int c = 0; c < signature.num_colors(); c++) {
        for (int b = 0; b < NUM_COEFS; b++) { // for every coef on a sig
            const int coef = signature.sig[c][b];
            bucket_t& bucket = imgbuckets.at(c, coef);

            std::string bs;
            for (const postId& i : bucket) {
              bs += i;
            }
            //INFO("bucket {}\n", bs);

            if (bucket.empty()) {
                continue;
            }

            const int w = imgBin.bin[abs(coef)];
            Score weight = weights[w][c];
            scale -= weight;

            for (postId index : bucket) {
                scores[index] -= weight;
                // scores[index] -= weight;
            }
        }
    }

  // print out elems
    for (auto const& elem : scores) {
        INFO("{} => {}", elem.first, elem.second);
    }

/*
  // Fill up the numres-bounded priority queue (largest at top):
  uint64_t i = 0;
  for (; pqResults.size() < numres && i < scores.size(); i++) {
    if (!isDeleted(i)) {
      pqResults.emplace(i, scores[i]);
    }
  }

  for (; i < scores.size(); i++) {
    if (!isDeleted(i) && scores[i] < pqResults.top().score) {
      pqResults.pop();
      pqResults.emplace(i, scores[i]);
    }
  }

  if (scale != 0) {
    scale = static_cast<Score>(1.0) / scale;
  }

  while (!pqResults.empty()) {
    sim_value value = pqResults.top();
    value.id = m_info[value.id].post_id; // XXX replace iqdb id with post id
    value.score = value.score * 100 * scale;

    V.push_back(value);
    pqResults.pop();
  }
  */

  std::reverse(V.begin(), V.end());
  return V;
}

void IQDB::removeImage(postId post_id) {
  auto image = sqlite_db_->getImage(post_id);
  if (image == std::nullopt) {
    WARN("couldn't remove post #{}; post not in sqlite database\n", post_id);
    return;
  }

  imgbuckets.remove(image->haar(), image->post_id);
  m_info.at(image->post_id).avgl.v[0] = 0;
  sqlite_db_->removeImage(post_id);
  --img_count;

  DEBUG("removed post #{} from memory and database\n", post_id);
}

uint64_t IQDB::getImgCount() {
  return img_count;
}

IQDB::IQDB(std::string filename) : sqlite_db_(nullptr) {
  loadDatabase(filename);
}

}
