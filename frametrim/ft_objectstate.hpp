#ifndef OBJECTSTATE_H
#define OBJECTSTATE_H

#include "trace_model.hpp"

#include <GL/gl.h>
#include <vector>
#include <memory>
#include <set>
#include <unordered_map>

#include <iostream>

namespace frametrim {

using PCall=std::shared_ptr<trace::Call>;

using StateCallMap=std::unordered_map<std::string, PCall>;


struct pcall_less {
    bool operator () (PCall lhs, PCall rhs)
    {
        return lhs->no < rhs->no;
    }
};

class CallSet {

public:

    CallSet(bool debug=false):m_debug(debug) {}

    bool m_debug;

    using iterator = std::set<PCall>::iterator;
    using const_iterator = std::set<PCall>::const_iterator;

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

private:
    std::set<PCall> m_calls;
};


class ObjectState
{
public:
    ObjectState(GLint glID);
    virtual ~ObjectState();

    unsigned id() const;

    void append_gen_call(PCall call);

    void emit_calls_to_list(CallSet& list) const;

    void append_call(PCall call);

    void set_state_call(PCall call, unsigned state_id_params);

protected:

    void reset_callset();

private:

    virtual void do_emit_calls_to_list(CallSet& list) const;

    GLint m_glID;

    CallSet m_gen_calls;

    StateCallMap m_state_calls;

    CallSet m_calls;

    mutable bool m_emitting;
};

using PObjectState=std::shared_ptr<ObjectState>;

}

#endif // OBJECTSTATE_H
