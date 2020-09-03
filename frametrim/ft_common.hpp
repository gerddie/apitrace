#ifndef FT_COMMON_HPP
#define FT_COMMON_HPP

#include "trace_model.hpp"
#include <memory>
#include <unordered_set>
#include <cstring>

namespace frametrim {

using PCall=std::shared_ptr<trace::Call>;

using CallIdSet = std::unordered_set<unsigned>;

struct string_part_less {
    bool operator () (const char *lhs, const char *rhs)
    {
        int len = std::min(strlen(lhs), strlen(rhs));
        return strncmp(lhs, rhs, len) < 0;
    }
};

inline static unsigned
equal_chars(const char *l, const char *r)
{
    unsigned retval = 0;
    while (*l && *r && *l == *r) {
        ++retval;
        ++l; ++r;
    }
    if (!*l && !*r)
        ++retval;

    return retval;
}

}

#endif // FT_COMMON_HPP
