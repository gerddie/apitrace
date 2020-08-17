#ifndef GENOBJECTSTATE_HPP
#define GENOBJECTSTATE_HPP

#include "ft_objectstate.hpp"

namespace frametrim {

class GenObjectState : public ObjectState
{
public:
   GenObjectState(GLint glID, PCall gen_call);
protected:

   void emit_gen_call(CallSet& list) const;

private:
   PCall m_gen_call;

};

}

#endif // GENOBJECTSTATE_HPP
