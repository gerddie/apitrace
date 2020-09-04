#include "ftr_tracecallext.hpp"

#include <sstream>

namespace frametrim_reverse {

StateCall::StateCall(const trace::Call& call, unsigned num_param_ids):
    TraceCall(call.no, combined_name(call, num_param_ids))
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

TraceCallOnBoundObj::TraceCallOnBoundObj(const trace::Call& call, PGenObject obj):
    TraceCall(call),
    m_object(obj)
{

}

void TraceCallOnBoundObj::add_owned_object(ObjectSet& out_set) const
{
    if (!m_object->visited()) {
        out_set.push(m_object);
        m_object->collect_objects(out_set);
        m_object->set_visited();
    }
}

}