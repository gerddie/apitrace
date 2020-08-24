#ifndef GLOBALSTATE_HPP
#define GLOBALSTATE_HPP

#include "ft_objectstate.hpp"

namespace frametrim  {

class GlobalState
{
public:
    GlobalState();

    bool in_target_frame() const;

    CallSet global_callset();

    PObjectState draw_framebuffer() const;
    PObjectState read_framebuffer() const;

};

}

#endif // GLOBALSTATE_HPP
