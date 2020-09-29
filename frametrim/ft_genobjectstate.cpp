

#include "ft_genobjectstate.hpp"

namespace frametrim {

GenObjectState::GenObjectState(GLint glID, PTraceCall gen_call):
    ObjectWithBindState(glID, gen_call),
    m_gen_call(gen_call)
{
}

void GenObjectState::emit_gen_call(CallSet& list) const
{
    if (m_gen_call)
        list.insert(m_gen_call);
}

SizedObjectState::SizedObjectState(GLint glID, PTraceCall gen_call, EAttachmentType at):
    GenObjectState(glID, gen_call),
    m_size(16), // this should be enough
    m_attachment_type(at),
    m_attach_count(0)
{
}

void SizedObjectState::unattach()
{
    if (is_attached())
        --m_attach_count;
    else
        std::cerr << "Trying to un-attach " << id()
                  << " but it is not attached\n";
}

void SizedObjectState::attach()
{
    ++m_attach_count;
}

bool SizedObjectState::is_attached() const
{
    return m_attach_count > 0;
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
