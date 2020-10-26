#ifndef CFRAMETRIMMER_HPP
#define CFRAMETRIMMER_HPP

#include "trace_parser.hpp"

#include <vector>

namespace frametrim {

enum Frametype {
    ft_none = 0,
    ft_key_frame = 1,
    ft_retain_frame = 2
};

class FrameTrimmer
{
public:
    FrameTrimmer();
    ~FrameTrimmer();

    void call(const trace::Call& call, Frametype target_frame_type);

    void finalize();

    std::vector<unsigned> get_sorted_call_ids();
private:
    struct FrameTrimmeImpl *impl;

};

}

#endif // CFRAMETRIMMER_HPP
