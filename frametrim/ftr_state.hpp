#ifndef FTR_STATE_HPP
#define FTR_STATE_HPP

#include "ft_common.hpp"

namespace frametrim_reverse {

class TraceMirror {
public:
    TraceMirror();
    ~TraceMirror();

    void process(trace::Call& call, bool required);

private:

    struct TraceMirrorImpl *impl;

};

}

#endif // FTR_STATE_HPP
