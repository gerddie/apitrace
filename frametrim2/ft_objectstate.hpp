#ifndef OBJECTSTATE_H
#define OBJECTSTATE_H

#include "ft_tracecall.hpp"

#include <GL/gl.h>
#include <vector>
#include <memory>
#include <unordered_set>
#include <unordered_map>

#include <iostream>

namespace frametrim {

class ObjectState
{
public:
    using Pointer = std::shared_ptr<ObjectState>;

    ObjectState(GLint glID, PTraceCall call);

    ObjectState(GLint glID);

    virtual ~ObjectState();

    unsigned id() const;

    void emit_calls_to_list(CallSet& list) const;

    PTraceCall append_call(PTraceCall call);

    PTraceCall set_state_call(const trace::Call& call, unsigned state_id_params);

protected:

    void reset_callset();

private:

    virtual bool is_active() const;

    virtual void do_emit_calls_to_list(CallSet& list) const;

    GLint m_glID;

    CallSet m_gen_calls;

    StateCallMap m_state_calls;

    CallSet m_calls;

    mutable bool m_emitting;
};


template <typename T>
class TObjStateMap {

public:
    TObjStateMap ():
        m_emitting(false){
    }

    typename T::Pointer get_by_id(uint64_t id) {
        assert(id > 0);

        auto iter = m_states.find(id);
        if (iter == m_states.end()) {
            assert(0 && "Expected id not found \n");
            return nullptr;
        }
        return iter->second;
    }

    void set(uint64_t id, typename T::Pointer obj) {
        m_states[id] = obj;
    }

    void clear(uint64_t id) {
        m_states[id] = nullptr;
    }

    void emit_calls_to_list(CallSet& list) const {
        if (m_emitting)
            return;
        m_emitting = true;
        do_emit_calls_to_list(list);
        m_emitting = false;
    }

protected:
    void emit_all_states(CallSet& list) const {
        for(auto& s: m_states)
            if (s.second)
                s.second->emit_calls_to_list(list);
    }

private:
    virtual void do_emit_calls_to_list(CallSet& list) const = 0;

    std::unordered_map<unsigned, typename T::Pointer> m_states;

    mutable bool m_emitting;

};


}

#endif // OBJECTSTATE_H
