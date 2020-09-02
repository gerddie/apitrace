#include "ftr_genobject.hpp"
#include "ftr_tracecall.hpp"
#include <iostream>

namespace frametrim_reverse {

using std::make_shared;

GenObject::GenObject(unsigned id):m_id(id)
{

}

unsigned GenObject::id() const
{
    return m_id;
}

PTraceCall  GenObjectMap::generate(trace::Call& call)
{
    const auto ids = call.arg(1).toArray();
    for (auto& v : ids->values) {
        auto id = v->toUInt();
        m_obj_table[id] = std::make_shared<GenObject>(id);;
    }
    return make_shared<TraceCall>(call);
}

PTraceCall GenObjectMap::destroy(trace::Call& call)
{
    const auto ids = call.arg(1).toArray();
    for (auto& v : ids->values)
        m_obj_table.erase(v->toUInt());
    return make_shared<TraceCall>(call);
}

PTraceCall GenObjectMap::bind(trace::Call& call, unsigned id_index)
{
    auto target = target_id_from_call(call);
    auto id = call.arg(id_index).toUInt();
    auto obj = bind_target(target, id);
    return make_shared<TraceCallOnBoundObj>(call, obj);
}

unsigned GenObjectMap::target_id_from_call(trace::Call& call) const
{
    return call.arg(0).toUInt();
}

PGenObject GenObjectMap::bind_target(unsigned target, unsigned id)
{
    if (id > 0) {
        if (!m_bound_to_target[target] ||
                m_bound_to_target[target]->id() != id) {

            auto obj = m_obj_table[id];
            if (!obj) {
                std::cerr << "Object " << id << " not found while binding\n";
                return nullptr;
            }

            m_bound_to_target[target] = obj;
        }
    } else {
         m_bound_to_target[target] = nullptr;
    }
    return m_bound_to_target[target];
}

}