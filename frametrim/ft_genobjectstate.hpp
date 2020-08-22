#ifndef GENOBJECTSTATE_HPP
#define GENOBJECTSTATE_HPP

#include "ft_objectstate.hpp"

#include <unordered_map>

namespace frametrim {

class GenObjectState : public ObjectState
{
public:     
    using Pointer = std::shared_ptr<GenObjectState>;

    GenObjectState(GLint glID, PCall gen_call);
protected:

    void emit_gen_call(CallSet& list) const;

private:
    PCall m_gen_call;

};

using PGenObjectState = GenObjectState::Pointer;


template <typename T>
class TStateMap  {

public:
    void generate(PCall call) {
        const auto ids = (call->arg(1)).toArray();
        for (auto& v : ids->values) {
            auto obj = std::make_shared<T>(v->toUInt(), call);
            m_states[v->toUInt()] = obj;
        }
    }

    typename T::Pointer get_by_id(uint64_t id) {
        assert(id > 0);

        auto iter = m_states.find(id);
        if (iter == m_states.end()) {
            std::cerr << "Expected id " << id << " not found \n";
            return nullptr;
        }
        return iter->second;
    }

    void destroy(PCall call) {
        const auto ids = (call->arg(1)).toArray();
        for (auto& v : ids->values) {
            auto state = this->get_by_id(v->toUInt());
            /* We erase the state here, but it might be referenced
             * elsewhere, so we have to record the call with the state */
            if (state)
                state->append_call(call);
            m_states.erase(v->toUInt());
        }
    }

private:
    std::unordered_map<unsigned, typename T::Pointer> m_states;
};



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
