#ifndef CFRAMEBUFFER_HPP
#define CFRAMEBUFFER_HPP

#include "trace_parser.hpp"

#include <memory>

namespace frametrim {

class Framebuffer
{
public:
    using Pointer = std::shared_ptr<Framebuffer>;

    Framebuffer();

};

}

#endif // CFRAMEBUFFER_HPP
