#ifndef GLOBALSTATE_HPP
#define GLOBALSTATE_HPP

#include "ft_objectstate.hpp"

namespace frametrim  {

class GlobalState
{
public:
    virtual bool in_target_frame() const = 0;

    virtual CallSet& global_callset() = 0;

    virtual PObjectState draw_framebuffer() const = 0;

    virtual PObjectState read_framebuffer() const = 0;

    virtual void collect_state_calls(CallSet& list) = 0;
};

}

#endif // GLOBALSTATE_HPP
