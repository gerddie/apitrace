#ifndef OBJECTSTATE_H
#define OBJECTSTATE_H

#include "trace_model.hpp"


#include <GL/gl.h>
#include <vector>
#include <memory>
#include <unordered_set>
#include <unordered_map>

#include <iostream>

namespace frametrim {

class GlobalState;

using PCall=std::shared_ptr<trace::Call>;

using StateCallMap=std::unordered_map<std::string, PCall>;


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

class CallSet {

public:

    CallSet(bool debug=false):m_debug(debug) {}

    bool m_debug;

    using Container = std::unordered_set<PCall, call_hash>;
    using iterator = Container::iterator;
    using const_iterator = Container::const_iterator;

    void insert(PCall call) {
        if (m_debug)
            std::cerr << "Insert: " << call->no << " " << call->name() << "\n";
        m_calls.insert(call);
    }

    void insert(const CallSet& calls) {
        if (m_debug) {
            for (auto& c: calls)
                std::cerr << "Insert: " << c->no << " " << c->name() << "\n";
        }
        m_calls.insert(calls.begin(), calls.end());
    }

    void insert(const StateCallMap& calls) {
        if (m_debug) {
            for (auto& c: calls)
                std::cerr << "Insert: " << c.second->no
                          << " " << c.second->name() << "\n";
        }
        for (auto& c: calls)
            m_calls.insert(c.second);
    }

    iterator begin() {
        return m_calls.begin();
    }

    const_iterator begin() const {
        return m_calls.begin();
    }

    iterator end() {
        return m_calls.end();
    }

    const_iterator end() const {
        return m_calls.end();
    }

    void clear() {
        m_calls.clear();
    }

    bool empty() const {
        return m_calls.empty();
    }

    size_t size() const {
        return m_calls.size();
    }

private:
    Container m_calls;
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

    void append_call(PCall call);

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
