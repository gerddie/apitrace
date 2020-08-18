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

using PGenObjectState = std::shared_ptr<GenObjectState>;


class SizedObjectState : public GenObjectState
{
public:
   SizedObjectState(GLint glID, PCall gen_call);

   unsigned width(unsigned level = 0) const;
   unsigned height(unsigned level = 0) const;

protected:

   void set_size(unsigned level, unsigned w, unsigned h);

private:
   std::vector<std::pair<unsigned, unsigned>> m_size;

};

using PSizedObjectState = std::shared_ptr<SizedObjectState>;


}

#endif // GENOBJECTSTATE_HPP
