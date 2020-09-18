#ifndef OBJECTSTATE_H
#define OBJECTSTATE_H

#include "ft_common.hpp"
#include "ft_tracecall.hpp"

#include <GL/gl.h>
#include <vector>
#include <memory>
#include <unordered_set>
#include <unordered_map>

#include <iostream>

namespace frametrim {

class GlobalState;

struct pcall_less {
    bool operator () (PCall lhs, PCall rhs)
    {
        return lhs->no < rhs->no;
    }
};

struct call_hash {
    std::size_t operator () (const PCall& call) const noexcept {
        return std::hash<unsigned>{}(call->no);
    }
};

class ObjectState
{
public:
    using Pointer = std::shared_ptr<ObjectState>;

    ObjectState(GLint glID, PCall call);
    ObjectState(GLint glID);
    virtual ~ObjectState();

    unsigned id() const;

    //void append_gen_call(PCall call);

    void emit_calls_to_list(CallSet& list) const;

    void append_call(PTraceCall call);

    void set_state_call(PCall call, unsigned state_id_params);

    CallSet& dependent_calls();

protected:

    void reset_callset();

private:

    virtual bool is_active() const;

    virtual void do_emit_calls_to_list(CallSet& list) const;

    GLint m_glID;

    CallSet m_gen_calls;

    CallSet m_depenendet_calls;

    StateCallMap m_state_calls;

    CallSet m_calls;

    mutable bool m_emitting;
};

using PObjectState=ObjectState::Pointer;


template <typename T>
class TObjStateMap {

public:
    TObjStateMap (GlobalState *gs):
        m_global_state(gs),
        m_emitting(false){
    }

    TObjStateMap() : TObjStateMap (nullptr) {

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
    GlobalState& global_state() {
        assert(m_global_state);
        return *m_global_state;
    }

    void emit_all_states(CallSet& list) const {
        for(auto& s: m_states)
            if (s.second)
                s.second->emit_calls_to_list(list);
    }

private:
    virtual void do_emit_calls_to_list(CallSet& list) const = 0;

    std::unordered_map<unsigned, typename T::Pointer> m_states;

    GlobalState *m_global_state;
    mutable bool m_emitting;

};


}

#endif // OBJECTSTATE_H
