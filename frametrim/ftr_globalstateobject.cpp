#include "ftr_globalstateobject.hpp"
#include "ftr_tracecall.hpp"

#include <GL/gl.h>
#include <GL/glext.h>
#include <iostream>


namespace frametrim_reverse {

GlobalStateObject::GlobalStateObject():
    m_bind_dirty(false)
{

}

void GlobalStateObject::record_bind(BindType type, PGenObject obj,
                                    unsigned id, unsigned tex_unit, unsigned callno)
{
    unsigned index = buffer_offset(type, id, tex_unit);
    auto& record = m_bind_timelines[index];
    record.push(callno, obj);
    m_bind_dirty = true;
    m_curently_bound[index] = obj;
}


PObjectVector
GlobalStateObject::currently_bound_objects_of_type(std::bitset<16> typemask)
{
    if (m_bind_dirty) {
        m_curently_bound_shadow = std::make_shared<std::vector<PTraceObject>>();
        for(auto&& bindings : m_curently_bound) {
            if (typemask.test(bindings.first / 128) && bindings.second)
                m_curently_bound_shadow->push_back(bindings.second);
        }
    }
    return m_curently_bound_shadow;
}

void GlobalStateObject::collect_objects_of_type(Queue& objects, unsigned call,
                                                std::bitset<16> typemask)
{
    const TraceCallRange range(call - 1, call + 1);
    for(auto&& timeline : m_bind_timelines) {
        if (typemask.test(timeline.first / 128))
            timeline.second.collect_active_in_call_range(objects, range);
    }
}

void GlobalStateObject::prepend_call(PTraceCall call)
{
    m_trace.push_front(call);
}

unsigned GlobalStateObject::get_required_calls(CallSet& required_calls) const
{
    unsigned first_required_call = std::numeric_limits<unsigned>::max();
    /* record all frames from the target frame set */
    for(auto& i : m_trace) {
        if (i->test_flag(TraceCall::required)) {
            i->add_object_calls(required_calls);
            required_calls.insert(i);
            first_required_call = i->call_no();
        }
    }
    return first_required_call;
}

void
GlobalStateObject::get_repeatable_states_from_beginning(CallSet &required_calls,
                                                        unsigned before) const
{
    std::unordered_map<std::string, std::string> param_map;

    /* Traces start at the highest number so start at the end
     * to record the earliest call */
    for (auto i = m_trace.rbegin(); i != m_trace.rend(); ++i) {
        if ((*i)->call_no() >= before)
            return;
        if ((*i)->test_flag(TraceCall::repeatable_state)) {
            resolve_repeatable_state_calls(*i, required_calls, param_map);
        }
    }
}

void
GlobalStateObject::get_last_states_before(CallSet &required_calls,
                                          unsigned before) const
{
    StateCallMap state_calls;


    for (auto&& c : m_trace) {
        /*  required calls are already in the output callset */
        if (c->call_no() >= before)
            continue;

        if (c->test_flag(TraceCall::single_state))
            resolve_state_calls(c, required_calls, before, state_calls);
    }
}


void
GlobalStateObject::resolve_state_calls(PTraceCall call,
                                       CallSet& callset /* inout */,
                                       unsigned next_required_call,
                                       StateCallMap &map) const
{
    /* Add the call if the last required call happended before
     * the passed function was called the last time.
     * The _last_before_callid_ value is initialized to the
     * maximum when the object is created. */
    auto& last_call = map[call->name()];
    if (call->call_no() < next_required_call &&
        last_call.last_before_callid < call->call_no()) {
        last_call.last_before_callid = call->call_no();
        callset.insert(call);
    }
}

void GlobalStateObject::resolve_repeatable_state_calls(PTraceCall call,
                                                       CallSet& callset /* inout */,
                                                       ParamMap &param_map) const
{
    auto& last_state_param_set = param_map[call->name()];
    if (last_state_param_set != call->name_with_params()) {
        param_map[call->name()] = call->name_with_params();
        callset.insert(call);
    }
}


unsigned GlobalStateObject::buffer_offset(BindType type, unsigned target,
                                          unsigned active_unit)
{
    unsigned base = type * 128;
    switch (type) {
    case bt_buffer:
        switch (target) {
        case GL_ARRAY_BUFFER: return base + 1;
        case GL_ATOMIC_COUNTER_BUFFER: 	return base + 2;
        case GL_COPY_READ_BUFFER: return base + 3;
        case GL_COPY_WRITE_BUFFER: return base + 4;
        case GL_DISPATCH_INDIRECT_BUFFER: return base + 5;
        case GL_DRAW_INDIRECT_BUFFER: return base + 6;
        case GL_ELEMENT_ARRAY_BUFFER: return base + 7;
        case GL_PIXEL_PACK_BUFFER: return base + 8;
        case GL_PIXEL_UNPACK_BUFFER: return base + 9;
        case GL_QUERY_BUFFER: return base + 10;
        case GL_SHADER_STORAGE_BUFFER: return base + 11;
        case GL_TEXTURE_BUFFER: return base + 12;
        case GL_TRANSFORM_FEEDBACK_BUFFER: return base + 13;
        case GL_UNIFORM_BUFFER: return base + 14;
        default:
            assert(0 && "unknown buffer bind point");
        }
    case bt_framebuffer:
        switch (target) {
        case GL_READ_FRAMEBUFFER: return base + 1;
        case GL_DRAW_FRAMEBUFFER: return base + 2;
        default:
            /* Since GL_FRAMEBUFFER may set both buffers we must handle this elsewhere */
            assert(0 && "GL_FRAMEBUFFER must be handled before calling buffer_offset");
        }
    case bt_texture: {
        base = active_unit * 11  + base;
        switch (target) {
        case GL_TEXTURE_1D: return base;
        case GL_TEXTURE_2D: return base + 1;
        case GL_TEXTURE_3D: return base + 2;
        case GL_TEXTURE_1D_ARRAY: return base + 3;
        case GL_TEXTURE_2D_ARRAY: return base + 4;
        case GL_TEXTURE_RECTANGLE: return base + 5;
        case GL_TEXTURE_CUBE_MAP: return base + 6;
        case GL_TEXTURE_CUBE_MAP_ARRAY: return base + 7;
        case GL_TEXTURE_BUFFER: return base + 8;
        case GL_TEXTURE_2D_MULTISAMPLE: return base + 9;
        case GL_TEXTURE_2D_MULTISAMPLE_ARRAY: return base + 10;
        default:
            assert(0 && "unknown texture bind point");
        }
    }
    case bt_sampler:
        return base + target;
    case bt_legacy_program:
            switch (target) {
            case GL_VERTEX_PROGRAM_ARB:
                return base + 1;
            case GL_FRAGMENT_PROGRAM_ARB:
                return base + 2;
            default:
                assert(0 && "unknown legacy shader bind point");
            }
    default:
        return base;
    }
    assert(0 && "Unsupported bind point");

}



}
