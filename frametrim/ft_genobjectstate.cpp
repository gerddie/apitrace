

#include "ft_genobjectstate.hpp"


namespace frametrim {

GenObjectState::GenObjectState(GLint glID, PCall gen_call):
   ObjectState(glID),
   m_gen_call(gen_call)
{
   assert(m_gen_call);
}

void GenObjectState::emit_gen_call(CallSet& list) const
{
   list.insert(m_gen_call);
}

}
