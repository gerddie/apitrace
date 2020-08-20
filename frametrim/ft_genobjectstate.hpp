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

    enum EAttachmentType {
        texture,
        renderbuffer
    };

    SizedObjectState(GLint glID, PCall gen_call, EAttachmentType at);

    unsigned width(unsigned level = 0) const;
    unsigned height(unsigned level = 0) const;

    friend bool operator == (const SizedObjectState& lhs,
                             const SizedObjectState& rhs);

protected:

    void set_size(unsigned level, unsigned w, unsigned h);

private:
    std::vector<std::pair<unsigned, unsigned>> m_size;

    EAttachmentType m_attachment_type;

};

using PSizedObjectState = std::shared_ptr<SizedObjectState>;

bool operator == (const SizedObjectState& lhs,
                  const SizedObjectState& rhs);


}

#endif // GENOBJECTSTATE_HPP
