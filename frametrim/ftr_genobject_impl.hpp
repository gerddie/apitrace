#ifndef FTR_GENOBJECT_IMPL_HPP
#define FTR_GENOBJECT_IMPL_HPP

#include <ftr_genobject.hpp>
#include "ftr_tracecall.hpp"
#include <iostream>

namespace frametrim_reverse {

using std::make_shared;

template <typename T>
PTraceCall  GenObjectMap<T>::generate(trace::Call& call)
{
    const auto ids = call.arg(1).toArray();
    for (auto& v : ids->values) {
        auto id = v->toUInt();
        m_obj_table[id] = std::make_shared<T>(id);;
    }
    return make_shared<TraceCall>(call);
}

template <typename T>
PTraceCall GenObjectMap<T>::destroy(trace::Call& call)
{
    const auto ids = call.arg(1).toArray();
    for (auto& v : ids->values)
        m_obj_table.erase(v->toUInt());
    return make_shared<TraceCall>(call);
}

template <typename T>
PTraceCall GenObjectMap<T>::bind(trace::Call& call, unsigned id_index)
{
    auto target = target_id_from_call(call);
    auto id = call.arg(id_index).toUInt();
    auto obj = bind_target(target, id);
    return make_shared<TraceCallOnBoundObj>(call, obj);
}

template <typename T>
unsigned GenObjectMap<T>::target_id_from_call(trace::Call& call) const
{
    return call.arg(0).toUInt();
}

template <typename T>
typename T::Pointer
GenObjectMap<T>::bind_target(unsigned target, unsigned id)
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

template <typename T>
PGenObject GenObjectMap<T>::bound_to_call_target_untyped(trace::Call& call)
{
    return this->bound_to_call_target(call);
}

template <typename T>
typename T::Pointer
GenObjectMap<T>::bound_to_call_target(trace::Call& call)
{
    return m_bound_to_target[this->target_id_from_call(call)];
}

}

#endif // FTR_GENOBJECT_IMPL_HPP
