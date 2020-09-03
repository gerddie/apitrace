#include "ftr_tracecallext.hpp"

namespace frametrim_reverse {

TraceCallOnBoundObj::TraceCallOnBoundObj(const trace::Call& call, PGenObject obj):
    TraceCall(call),
    m_object(obj)
{

}

void TraceCallOnBoundObj::add_owned_object(ObjectSet& out_set) const
{
    if (!m_object->visited()) {
        out_set.push(m_object);
        m_object->set_visited();
    }
}

}