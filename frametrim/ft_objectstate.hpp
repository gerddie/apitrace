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
    bt_sync_object,
    bt_texture,
    bt_vertex_array,
    bt_vertex_pointer,
    bt_last
};


enum EStateCaches {
    sc_vertex_attr_pointer,
    sc_va_enables,
    sc_states,
    sc_enables,
    sc_matrix_states,
    sc_legacy_programs,
    sc_programs,
    sc_textures,
    sc_fbo,
    sc_buffers,
    sc_shaders,
    sc_renderbuffers,
    sc_samplers,
    sc_sync_object,
    sc_vertex_arrays,
    sc_vertex_attrib_pointers,
    sc_vertex_attribs,
    sc_last
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
    PTraceCall append_call(const trace::Call& call);

    PTraceCall set_state_call(const trace::Call& call, unsigned state_id_params);

    void flush_state_cache(ObjectState& fbo) const;

    unsigned global_id() const;

    unsigned cache_id() const;

    PCallSet get_state_cache() const;

    bool state_cache_dirty() const;

    const char *type_name() const;
protected:
    virtual void post_set_state_call(PTraceCall call) {(void)call;}

    virtual unsigned get_cache_id_helper() const;

    unsigned fence_id() const;

    void reset_callset();

    void dirty_cache();

    virtual PTraceCall gen_call() const  {return nullptr; }

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
    mutable unsigned m_callset_submit_fence;
    mutable unsigned m_callset_fence;
    mutable PCallSet m_state_cache;
};

template <typename T>
class TObjStateMap {

public:
    TObjStateMap ():
        m_emitting(false){
    }

    typename T::Pointer get_by_id(uint64_t id, bool required = true) {
        auto iter = m_states.find(id);
        if (iter == m_states.end()) {
            assert((!required || !id) && "Expected id not found \n");
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
        emit_unbind_calls(list);
        m_emitting = false;
    }

    void flush_state_caches(ObjectState& fbo) const {
        for (auto&& [key, state]: m_states) {
            if (state)
                state->flush_state_cache(fbo);
        }
    }

    PCallSet get_state_caches() const {
        bool dirty_cache = false;
        for (auto&& [key, state]: m_states) {
            if (state && state->state_cache_dirty()) {
                dirty_cache = true;
                break;
            }
        }

        if (dirty_cache || !m_state_cache) {
            m_state_cache = std::make_shared<CallSet>();
            for (auto&& [key, state]: m_states) {
                if (state)
                    m_state_cache->insert(key, state->get_state_cache());
            }
        }
        return m_state_cache;
    }

protected:
    void emit_all_states(CallSet& list) const {
        for(auto& s: m_states)
            if (s.second)
                s.second->emit_calls_to_list(list);
    }

private:
    virtual void do_emit_calls_to_list(CallSet& list) const = 0;
    virtual void emit_unbind_calls(CallSet& list) const {(void) list;}

    std::unordered_map<unsigned, typename T::Pointer> m_states;

    mutable bool m_emitting;
    mutable PCallSet m_state_cache;

};


}

#endif // OBJECTSTATE_H
