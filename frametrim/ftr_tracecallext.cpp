#include "ftr_tracecallext.hpp"

#include <sstream>
#include <iostream>

namespace frametrim_reverse {

StateCall::StateCall(const trace::Call& call, unsigned num_param_ids):
    TraceCall(call, combined_name(call, num_param_ids), true)
{

}

std::string
StateCall::combined_name(const trace::Call& call,
                         unsigned num_param_ids)
{
    std::stringstream s;

    s << call.name();
    for (auto i = 0u; i < num_param_ids; ++i)
        s << "_" << call.arg(i).toUInt();
    return s.str();
}

StateEnableCall::StateEnableCall(const trace::Call& call, const char *basename):
    TraceCall(call, combined_name(call, basename), true)
{

}

std::string
StateEnableCall::combined_name(const trace::Call& call, const char *basename)
{
    std::stringstream s;
    s << basename << "_" << call.arg(0).toUInt();
    return s.str();
}

TraceCallOnBoundObj::TraceCallOnBoundObj(const trace::Call& call, PGenObject obj):
    TraceCall(call),
    m_object(obj)
{

}

void TraceCallOnBoundObj::add_owned_object(ObjectSet& out_set) const
{
    std::cerr << "Bound object: " << m_object->id();

    if (!m_object->visited()) {
        std::cerr << " added";
        out_set.push(m_object);
        m_object->collect_objects(out_set, call_no());
    }
    std::cerr << "\n";
}

TraceCallOnBoundObjWithDeps::TraceCallOnBoundObjWithDeps(const trace::Call& call,
                                                         PGenObject obj,
                                                         PGenObject dep):
    TraceCallOnBoundObj(call, obj),
    m_dependency(dep)
{
}

void TraceCallOnBoundObjWithDeps::add_dependend_objects(ObjectSet& out_set) const
{
    if (!m_dependency->visited()) {
        out_set.push(m_dependency);
        m_dependency->collect_objects(out_set, call_no());
    }
}

}