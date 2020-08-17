#ifndef TEXTURESTATE_HPP
#define TEXTURESTATE_HPP

#include "ft_objectstate.hpp"

namespace frametrim {

class TextureState : public ObjectState
{
public:
   TextureState(GLint glID, PCall gen_call);

   void bind_unit(PCall bind, PCall unit);

   void data(PCall call);
   void use(PCall call);

private:
   virtual void do_append_calls_to(CallSet& list) const;

   PCall m_gen_call;
   PCall m_last_unit_call;
   PCall m_last_bind_call;
   CallSet m_data_upload_set;
   CallSet m_data_use_set;




};

using PTextureState = std::shared_ptr<TextureState>;

}

#endif // TEXTURESTATE_HPP
