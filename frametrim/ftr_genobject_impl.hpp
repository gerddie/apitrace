#ifndef FTR_GENOBJECT_IMPL_HPP
#define FTR_GENOBJECT_IMPL_HPP

#include <ftr_genobject.hpp>
#include "ftr_tracecall.hpp"
#include <iostream>

namespace frametrim_reverse {

using std::make_shared;

template <typename T>
void
GenBoundObjectMap<T>::add(typename T::Pointer obj)
{
    m_obj_table[obj->id()] = obj;
}

template <typename T>
void
GenBoundObjectMap<T>::erase(unsigned id)
{
    m_obj_table.erase(id);
}

template <typename T>
PTraceCall
GenBoundObjectMap<T>::bind(trace::Call& call, unsigned id_index)
{
    auto target = target_id_from_call(call);
    auto id = call.arg(id_index).toUInt();
    auto obj = bind_target(target, id);
    return make_shared<TraceCallOnBoundObj>(call, obj);
}

template <typename T>
unsigned
GenBoundObjectMap<T>::target_id_from_call(trace::Call& call) const
{
    return call.arg(0).toUInt();
}

template <typename T>
typename T::Pointer
GenBoundObjectMap<T>::bind_target(unsigned target, unsigned id)
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
PGenObject
GenBoundObjectMap<T>::bound_to_call_target_untyped(trace::Call& call)
{
    return this->bound_to_call_target(call);
}

template <typename T>
typename T::Pointer
GenBoundObjectMap<T>::bound_to_call_target(trace::Call& call)
{
    return m_bound_to_target[this->target_id_from_call(call)];
}

template <typename T>
PGenObject
GenBoundObjectMap<T>::by_id_untyped(unsigned id)
{
    return this->by_id(id);
}

template <typename T>
typename T::Pointer
GenBoundObjectMap<T>::by_id(unsigned id)
{
    return m_obj_table[id];
}

template <typename T>
PTraceCall
GenObjectMap<T>::generate(trace::Call& call)
{
    const auto ids = call.arg(1).toArray();
    for (auto& v : ids->values) {
        auto id = v->toUInt();
        this->add(std::make_shared<T>(id));
    }
    return make_shared<TraceCall>(call);
}

template <typename T>
PTraceCall
GenObjectMap<T>::destroy(trace::Call& call)
{
    const auto ids = call.arg(1).toArray();
    for (auto& v : ids->values)
        this->erase(v->toUInt());
    return make_shared<TraceCall>(call);
}

template <typename T>
PTraceCall
CreateObjectMap<T>::create(trace::Call &call)
{
    const auto id = call.ret->toUInt();
    auto obj = std::make_shared<T>(id);
    this->add(obj);
    return make_shared<TraceCallOnBoundObj>(call, obj);
}

template <typename T>
PTraceCall
CreateObjectMap<T>::destroy(trace::Call& call)
{
    const auto id = call.arg(0).toUInt();
    this->erase(id);
    return make_shared<TraceCall>(call);
}



}

#endif // FTR_GENOBJECT_IMPL_HPP
