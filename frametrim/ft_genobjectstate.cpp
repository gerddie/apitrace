

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

SizedObjectState::SizedObjectState(GLint glID, PCall gen_call, EAttachmentType at):
   GenObjectState(glID, gen_call),
   m_size(16), // this should be enough
   m_attachment_type(at)
{
}

unsigned SizedObjectState::width(unsigned level) const
{
   return m_size[level].first;
}

unsigned SizedObjectState::height(unsigned level) const
{
   return m_size[level].second;
}

void SizedObjectState::set_size(unsigned level, unsigned w, unsigned h)
{
   assert(level < m_size.size());
   m_size[level] = std::make_pair(w, h);
}

bool operator == (const SizedObjectState& lhs, const SizedObjectState& rhs)
{
   return lhs.id() == rhs.id() &&
         lhs.m_attachment_type == rhs.m_attachment_type;
}

}
