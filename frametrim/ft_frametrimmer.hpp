#ifndef CFRAMETRIMMER_HPP
#define CFRAMETRIMMER_HPP

#include "trace_parser.hpp"

#include <vector>

namespace frametrim {

class FrameTrimmer
{
public:
    FrameTrimmer();
    ~FrameTrimmer();

    void call(const trace::Call& call, bool in_target_frame);

    void finalize();

    std::vector<unsigned> get_sorted_call_ids();
private:
    struct FrameTrimmeImpl *impl;

};

}

#endif // CFRAMETRIMMER_HPP