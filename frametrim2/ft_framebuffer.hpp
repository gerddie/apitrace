#ifndef CFRAMEBUFFER_HPP
#define CFRAMEBUFFER_HPP

#include "trace_parser.hpp"

#include <memory>

namespace frametrim {

class FramebufferState
{
public:
    using Pointer = std::shared_ptr<FramebufferState>;

    FramebufferState();

};

}

#endif // CFRAMEBUFFER_HPP
