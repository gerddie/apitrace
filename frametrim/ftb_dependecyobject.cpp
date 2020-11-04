#include "ftb_dependecyobject.hpp"

namespace frametrim {


void DependecyObject::add_call(PTraceCall call)
{
    m_calls.push_back(call);
}

void DependecyObject::add_depenency(Pointer dep)
{
    m_dependencies.push_back(dep);
}

void DependecyObject::append_calls(CallSet& out_list)
{
    for (auto&& n : m_calls)
        out_list.insert(n);

    for (auto&& o : m_dependencies)
        o->append_calls(out_list);
}


PTraceCall DependecyObjectMap::Generate(const trace::Call& call)
{
    auto c = trace2call(call);
    const auto ids = (call.arg(1)).toArray();
    for (auto& v : ids->values) {
        auto obj = std::make_shared<DependecyObject>(v->toUInt());
        obj->add_call(c);
        m_objects[v->toUInt()] = obj;
    }
    return c;
}

PTraceCall DependecyObjectMap::Bind(const trace::Call& call)
{
    unsigned id = get_id_from_call(call);
    unsigned bindpoint = get_bindpoint_from_call(call);

    if (id) {
        m_bound_object[bindpoint] = m_objects[id];
    } else {
        m_bound_object[bindpoint] = nullptr;
    }

    auto c = trace2call(call);
    m_calls.push_back(c);
    return c;
}

PTraceCall DependecyObjectMap::CallOnBoundObject(const trace::Call& call)
{
    unsigned bindpoint = get_bindpoint_from_call(call);
    assert(m_bound_object[bindpoint]);
    auto c = trace2call(call);
    m_bound_object[bindpoint]->add_call(c);
    return c;
}

PTraceCall DependecyObjectMap::CallOnBoundObjectWithDep(const trace::Call& call,
                                                        int dep_obj_param,
                                                        DependecyObjectMap& other_objects)
{
    unsigned bindpoint = get_bindpoint_from_call(call);
    assert(m_bound_object[bindpoint]);
    auto c = trace2call(call);
    unsigned obj_id = call.arg(dep_obj_param).toUInt();
    if (obj_id) {
        auto obj = other_objects.get_by_id(obj_id);
        assert(obj);
        m_bound_object[bindpoint]->add_depenency(obj);
    }
    return c;
}

DependecyObject::Pointer DependecyObjectMap::get_by_id(unsigned id) const
{
    auto i = m_objects.find(id);
    return i !=  m_objects.end() ? i->second : nullptr;
}

}