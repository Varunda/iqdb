#ifndef IQDB_TYPES_H
#define IQDB_TYPES_H_N

#include <cstdint>

namespace iqdb {

using postId = std::string; // An external (Danbooru) post ID.

// The type used for calculating similarity scores during queries, and for
// storing `avgl` values in the `m_info` array.
using Score = float;

}

#endif
