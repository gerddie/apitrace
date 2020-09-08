#include "ftr_programobj.hpp"
#include "ftr_tracecall.hpp"

#include "ftr_genobject_impl.hpp"

#include <GL/gl.h>
#include <GL/glext.h>

namespace frametrim_reverse {

using std::make_shared;

void
ShaderObject::source(const trace::Call& call)
{
    m_source_call = call.no;
}

void
ShaderObject::compile(const trace::Call& call)
{
    m_compile_call = call.no;
}


void
ShaderObject::collect_data_calls(CallIdSet& calls, unsigned call_before)
{
    assert(m_compile_call < call_before);
    assert(m_source_call < call_before);

    calls.insert(m_compile_call);
    calls.insert(m_source_call);
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

void
ProgramObject::attach_shader(const trace::Call& call, PGenObject shader)
{   
    m_attach_calls.push_back(call.no);
    m_attached_shaders.insert(shader);
}

void
ProgramObject::bind_attr_location(unsigned loc)
{
    m_bound_attributes.insert(loc);
}

void ProgramObject::collect_dependend_obj(Queue& objects)
{
    for(auto& i : m_attached_shaders) {
        if (!i->visited()) {
            objects.push(i);
            i->collect_objects(objects);
            i->set_visited();
        }
    }

    for(auto& i : m_bound_attr_buffers) {
        if (i.second && !i.second->visited()) {
            objects.push(i.second);
            i.second->collect_objects(objects);
            i.second->set_visited();
        }
    }
}

void ProgramObject::link(const trace::Call& call)
{
    m_link_call = call.no;
}

void
ProgramObject::collect_data_calls(CallIdSet& calls, unsigned call_before)
{
    calls.insert(m_link_call);
    collect_all_calls_before(calls, m_attach_calls, call_before);
    collect_all_calls_before(calls, m_data_calls, call_before);

    for (auto&& uniform_slot: m_uniforms_calls) {
        collect_last_call_before(calls, uniform_slot.second, call_before);
    }
}

void
ProgramObject::bind_attr_pointer(unsigned attr_id, PBufObject obj)
{
    m_bound_attr_buffers[attr_id] = obj;
}

void
ProgramObject::data_call(const trace::Call& call)
{
    m_data_calls.push_back(call.no);
}

void ProgramObject::uniform(const trace::Call& call)
{
    m_uniforms_calls[call.arg(0).toUInt()].push_back(call.no);
}

PTraceCall
ProgramObjectMap::attach_shader(trace::Call& call, const ShaderObjectMap& shaders)
{
    auto program = by_id(call.arg(0).toUInt());
    assert(program);
    auto shader = shaders.by_id_untyped(call.arg(1).toUInt());
    program->attach_shader(call, shader);
    return make_shared<TraceCallOnBoundObjWithDeps>(call, program, shader);
}

PTraceCall ProgramObjectMap::bind_attr_location(trace::Call& call)
{
    auto program = by_id(call.arg(0).toUInt());
    assert(program);
    program->bind_attr_location(call.arg(1).toUInt());
    return make_shared<TraceCallOnBoundObjWithDeps>(call, program, nullptr);
}

PTraceCall
ProgramObjectMap::vertex_attr_pointer(trace::Call& call, BufObjectMap& buffers)
{
    auto program_id = call.arg(0).toUInt();
    auto attr_id = call.arg(1).toUInt();
    auto attr_buffer = buffers.bound_to_target(GL_ARRAY_BUFFER);
    auto program = by_id(program_id);
    program->bind_attr_pointer(attr_id, attr_buffer);
    return make_shared<TraceCallOnBoundObjWithDeps>(call, program, attr_buffer);
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
    program->data_call(call);
    return make_shared<TraceCallOnBoundObj>(call, program);
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
    program->uniform(call);
    return make_shared<TraceCallOnBoundObj>(call, program);
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
    program->link(call);
    return make_shared<TraceCallOnBoundObj>(call, program);
}


template class CreateObjectMap<ProgramObject>;
template class GenBoundObjectMap<ProgramObject>;
template class GenObjectMap<ProgramObject>;

}