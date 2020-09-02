#ifndef FT_COMMON_HPP
#define FT_COMMON_HPP

#include "trace_model.hpp"
#include <memory>
#include <cstring>

namespace frametrim {

using PCall=std::shared_ptr<trace::Call>;

struct string_part_less {
    bool operator () (const char *lhs, const char *rhs)
    {
        int len = std::min(strlen(lhs), strlen(rhs));
        return strncmp(lhs, rhs, len) < 0;
    }
};

}

#endif // FT_COMMON_HPP
