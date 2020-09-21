#ifndef CFRAMETRIMMER_HPP
#define CFRAMETRIMMER_HPP

#include "trace_parser.hpp"

#include <vector>

namespace frametrim {

class CFrameTrimmer
{
public:
    CFrameTrimmer();
    ~CFrameTrimmer();

    void call(const trace::Call& call, bool in_target_frame);

    std::vector<unsigned> get_sorted_call_ids() const;
private:
    struct CFrameTrimmeImpl *impl;

};

}

#endif // CFRAMETRIMMER_HPP
