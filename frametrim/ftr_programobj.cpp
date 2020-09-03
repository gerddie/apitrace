#include "ftr_programobj.hpp"
#include "ftr_tracecall.hpp"

#include "ftr_genobject_impl.hpp"

namespace frametrim_reverse {

using std::make_shared;

void
ProgramObject::attach_shader(unsigned shader_id)
{
    m_attached_shaders.insert(shader_id);
}

void
ProgramObject::bind_attr_location(unsigned loc)
{
    m_bound_attributes.insert(loc);
}

PTraceCall
ProgramObjectMap::attach_shader(trace::Call& call)
{
    auto program = by_id(call.arg(0).toUInt());
    assert(program);
    program->attach_shader(call.arg(1).toUInt());
    return make_shared<TraceCallOnBoundObjWithDeps>(call, program);
}

PTraceCall ProgramObjectMap::bind_attr_location(trace::Call& call)
{
    auto program = by_id(call.arg(0).toUInt());
    assert(program);
    program->bind_attr_location(call.arg(1).toUInt());
    return make_shared<TraceCallOnBoundObjWithDeps>(call, program);
}

unsigned
ProgramObjectMap::target_id_from_call(trace::Call& call) const
{
    (void)call;
    /* There is only one bind target */
    return 0;
}

template class CreateObjectMap<ProgramObject>;
template class GenBoundObjectMap<ProgramObject>;
template class GenObjectMap<ProgramObject>;

}