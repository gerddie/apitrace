#include "ftr_programobj.hpp"
#include "ftr_tracecall.hpp"

#include "ftr_genobject_impl.hpp"

namespace frametrim_reverse {

using std::make_shared;

void
ProgramObject::attach_shader(PGenObject shader)
{
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
}

PTraceCall
ProgramObjectMap::attach_shader(trace::Call& call, const ShaderObjectMap& shaders)
{
    auto program = by_id(call.arg(0).toUInt());
    assert(program);
    auto shader = shaders.by_id_untyped(call.arg(1).toUInt());
    program->attach_shader(shader);
    return make_shared<TraceCallOnBoundObjWithDeps>(call, program, shader);
}

PTraceCall ProgramObjectMap::bind_attr_location(trace::Call& call)
{
    auto program = by_id(call.arg(0).toUInt());
    assert(program);
    program->bind_attr_location(call.arg(1).toUInt());
    return make_shared<TraceCallOnBoundObjWithDeps>(call, program, nullptr);
}

unsigned
ProgramObjectMap::target_id_from_call(const trace::Call& call) const
{
    (void)call;
    /* There is only one bind target */
    return 0;
}

template class CreateObjectMap<ProgramObject>;
template class GenBoundObjectMap<ProgramObject>;
template class GenObjectMap<ProgramObject>;

}