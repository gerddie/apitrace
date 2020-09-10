#include "ftr_programobj.hpp"
#include "ftr_tracecall.hpp"

#include "ftr_genobject_impl.hpp"

#include <GL/gl.h>
#include <GL/glext.h>

namespace frametrim_reverse {

using std::make_shared;

PTraceCall
ShaderObject::source(const trace::Call& call)
{
    m_source_call = make_shared<TraceCall>(call);
    return m_source_call;
}

PTraceCall
ShaderObject::compile(const trace::Call& call)
{
    m_compile_call = make_shared<TraceCall>(call);
    return m_compile_call;
}


void
ShaderObject::collect_data_calls(CallIdSet& calls, unsigned call_before)
{
    assert(m_compile_call->call_no() < call_before);
    assert(m_source_call->call_no() < call_before);

    calls.insert(*m_compile_call);
    calls.insert(*m_source_call);
}

PTraceCall
ShaderObjectMap::compile(const trace::Call& call)
{
    auto sh = by_id(call.arg(0).toUInt());
    assert(sh);
    sh->compile(call);
    return make_shared<TraceCall>(call);
}

PTraceCall
ShaderObjectMap::source(const trace::Call& call)
{
    auto sh = by_id(call.arg(0).toUInt());
    assert(sh);
    sh->source(call);
    return make_shared<TraceCall>(call);
}

PTraceCall
ProgramObject::attach_shader(const trace::Call& call, PGenObject shader)
{   

    auto c = make_shared<TraceCallOnBoundObj>(call, shader);
    m_attach_calls.push_front(c);
    m_attached_shaders.insert(shader);
    return c;
}

void
ProgramObject::bind_attr_location(unsigned loc)
{
    m_bound_attributes.insert(loc);
}

void ProgramObject::collect_dependend_obj(Queue& objects, unsigned at_call)
{
    for(auto& i : m_attached_shaders) {
        if (!i->visited()) {
            objects.push(i);
            i->collect_objects(objects, at_call);
        }
    }

    for(auto& i : m_bound_attr_buffers) {
        if (i.second && !i.second->visited()) {
            objects.push(i.second);
            i.second->collect_objects(objects, at_call);
        }
    }
}

PTraceCall ProgramObject::link(const trace::Call& call)
{
    m_link_call = make_shared<TraceCall>(call);
    return m_link_call;
}

void
ProgramObject::collect_data_calls(CallIdSet& calls, unsigned call_before)
{
    calls.insert(*m_link_call);
    collect_all_calls_before(calls, m_attach_calls, call_before);
    collect_all_calls_before(calls, m_data_calls, call_before);

    unsigned needs_bind_before = call_before;
    for (auto&& uniform_slot: m_uniforms_calls) {
        unsigned call_no = collect_last_call_before(calls, uniform_slot.second, call_before);
        if (call_no < needs_bind_before)
            needs_bind_before = call_no;
    }

    for (auto&& va : m_va_pointer_calls)
        if (!va.second.empty())
            calls.insert(*va.second.front());
    collect_bind_calls(calls, needs_bind_before);
}

PTraceCall ProgramObject::bind_attr_pointer(const trace::Call& call, unsigned attr_id, PBufObject obj)
{
    m_bound_attr_buffers[attr_id] = obj;
    auto c = make_shared<TraceCall>(call);
    m_va_pointer_calls[attr_id].push_front(c);
    return c;
}

PTraceCall
ProgramObject::data_call(const trace::Call& call)
{
    auto c = make_shared<TraceCall>(call);
    m_data_calls.push_front(c);
    return c;
}

PTraceCall
ProgramObject::uniform(const trace::Call& call)
{
    auto c = make_shared<TraceCall>(call);
    m_uniforms_calls[call.arg(0).toUInt()].push_back(c);
    return c;
}

PTraceCall
ProgramObjectMap::attach_shader(trace::Call& call, const ShaderObjectMap& shaders)
{
    auto program = by_id(call.arg(0).toUInt());
    assert(program);
    auto shader = shaders.by_id_untyped(call.arg(1).toUInt());
    return program->attach_shader(call, shader);
}

PTraceCall ProgramObjectMap::bind_attr_location(trace::Call& call)
{
    auto program = by_id(call.arg(0).toUInt());
    assert(program);
    program->bind_attr_location(call.arg(1).toUInt());
    return make_shared<TraceCallOnBoundObj>(call, program);
}

PTraceCall
ProgramObjectMap::vertex_attr_pointer(trace::Call& call, BufObjectMap& buffers)
{
    auto attr_id = call.arg(0).toUInt();
    auto attr_buffer = buffers.bound_to_target(GL_ARRAY_BUFFER);
    auto program = bound_to_target(0);
    return program->bind_attr_pointer(call, attr_id, attr_buffer);
}

unsigned
ProgramObjectMap::target_id_from_call(const trace::Call& call) const
{
    (void)call;
    /* There is only one bind target */
    return 0;
}

PTraceCall
ProgramObjectMap::data(trace::Call& call)
{
    auto program = bound_to_target(0);
    if (!program) {
        std::cerr << "Call " << call.no << " ("<< call.name()
                  << ") no program with ID" << call.arg(0).toUInt() <<"\n";
        return NULL;
    }
    return program->data_call(call);
}

PTraceCall
ProgramObjectMap::uniform(trace::Call& call)
{
    auto program = bound_to_target(0);
    if (!program) {
        std::cerr << "Call " << call.no << " ("<< call.name()
                  << ") no program with ID" << call.arg(0).toUInt() <<"\n";
        return NULL;
    }
    return program->uniform(call);
}

PTraceCall
ProgramObjectMap::link(trace::Call& call)
{
    auto program = by_id(call.arg(0).toUInt());
    if (!program) {
        std::cerr << "Call " << call.no << " ("<< call.name()
                  << ") no program with ID" << call.arg(0).toUInt() <<"\n";
        return NULL;
    }
    return program->link(call);
}

template class CreateObjectMap<ProgramObject>;
template class GenBoundObjectMap<ProgramObject>;
template class GenObjectMap<ProgramObject>;

}