#ifndef FRAMEBUFFERSTATE_HPP
#define FRAMEBUFFERSTATE_HPP

#include "ft_genobjectstate.hpp"

namespace frametrim {

class FramebufferState : public GenObjectState
{
public:
   using GenObjectState::GenObjectState;

   void bind(PCall call);

private:

   PCall m_bind_call;


};

using PFramebufferState = std::shared_ptr<FramebufferState>;

}

#endif // FRAMEBUFFERSTATE_HPP
