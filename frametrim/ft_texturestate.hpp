#ifndef TEXTURESTATE_HPP
#define TEXTURESTATE_HPP

#include "ft_framebufferstate.hpp"

namespace frametrim {

class TextureState : public SizedObjectState
{
public:
   using SizedObjectState::SizedObjectState;

   void bind_unit(PCall bind, PCall unit);

   void data(PCall call);
   void use(PCall call);

   void rendertarget_of(unsigned layer, PFramebufferState fbo);

private:
   virtual void do_emit_calls_to_list(CallSet& list) const;

   PCall m_last_unit_call;
   PCall m_last_bind_call;
   CallSet m_data_upload_set;
   CallSet m_data_use_set;

   std::unordered_map<unsigned, PFramebufferState> m_fbo;
};

using PTextureState = std::shared_ptr<TextureState>;

}

#endif // TEXTURESTATE_HPP
