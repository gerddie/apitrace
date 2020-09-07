#ifndef FTR_STATE_HPP
#define FTR_STATE_HPP

#include "ft_common.hpp"
#include "ftr_tracecall.hpp"

namespace frametrim_reverse {

class TraceMirror {
public:
    TraceMirror();
    ~TraceMirror();

    void process(trace::Call& call, bool required);

    std::vector<unsigned> trace() const;

private:

    struct TraceMirrorImpl *impl;
};

}

#endif // FTR_STATE_HPP
