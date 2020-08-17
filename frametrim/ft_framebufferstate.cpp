#include "ft_framebufferstate.hpp"

namespace frametrim {

void FramebufferState::bind(PCall call)
{
   m_bind_call = call;
}

}
