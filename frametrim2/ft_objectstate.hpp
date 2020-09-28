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

class FramebufferState;

enum ObjectType {
    bt_buffer,
    bt_display_list,
    bt_framebuffer,
    bt_program,
    bt_legacy_program,
    bt_matrix,
    bt_renderbuffer,
    bt_shader,
    bt_sampler,
    bt_texture,
    bt_vertex_array,
    bt_last
};

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

    void flush_state_cache(ObjectState& fbo) const;

    unsigned global_id() const;

protected:
    virtual void post_set_state_call(PTraceCall call) {(void)call;}
    void reset_callset();

    void dirty_cache() {m_callset_dirty = true;}

private:

    virtual ObjectType type() const = 0;

    virtual bool is_active() const;

    virtual void do_emit_calls_to_list(CallSet& list) const;

    virtual void pass_state_cache(unsigned object_id, PCallSet cache);

    virtual void emit_dependend_caches(CallSet& list) const;

    GLint m_glID;

    CallSet m_gen_calls;

    StateCallMap m_state_calls;

    CallSet m_calls;

    ObjectType m_type;

    mutable bool m_emitting;
    mutable bool m_callset_dirty;
    mutable PCallSet m_state_cache;
};

template <typename T>
class TObjStateMap {

public:
    TObjStateMap ():
        m_emitting(false){
    }

    typename T::Pointer get_by_id(uint64_t id) {
        if (!id)
            return nullptr;

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
