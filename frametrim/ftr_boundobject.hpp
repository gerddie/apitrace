#ifndef BOUNDOBJECTMAP_HPP
#define BOUNDOBJECTMAP_HPP

#include "ftr_tracecallext.hpp"
#include "ftr_genobject.hpp"

#include <iostream>

namespace frametrim_reverse {

class BoundObjectMap {
public:
    virtual PGenObject bound_to_call_target_untyped(const trace::Call& call) const = 0;
    virtual PGenObject by_id_untyped(unsigned id) const = 0;
    virtual void collect_currently_bound_objects(GenObject::Queue& objects) const = 0;
};

template <typename T>
class GenBoundObjectMap : public BoundObjectMap {
public:
    typename T::Pointer bound_to_call_target(const trace::Call& call) const;
    typename T::Pointer bound_to_target(unsigned target) const;
    typename T::Pointer by_id(unsigned id) const;
    PGenObject bind(const trace::Call& call, unsigned id_index);
    PGenObject bound_to_call_target_untyped(const trace::Call& call) const override;
    PGenObject by_id_untyped(unsigned id) const override;
    void collect_currently_bound_objects(GenObject::Queue& objects) const override;
protected:
    typename T::Pointer bind_target(unsigned target, unsigned id);
    void add(typename T::Pointer obj);
    void erase(unsigned id);
private:
    virtual unsigned target_id_from_call(const trace::Call& call) const;

    std::unordered_map<unsigned, typename T::Pointer> m_obj_table;
    std::unordered_map<unsigned, typename T::Pointer> m_bound_to_target;
};

template <typename T>
class GenObjectMap : public GenBoundObjectMap<T> {
public:
    PTraceCall generate(const trace::Call& call);
    PTraceCall destroy(const trace::Call& call);
};

template <typename T>
class CreateObjectMap : public GenBoundObjectMap<T> {
public:
    PTraceCall create(const trace::Call& call);
    PTraceCall destroy(const trace::Call& call);
};

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
PGenObject
GenBoundObjectMap<T>::bind(const trace::Call& call, unsigned id_index)
{
    auto target = target_id_from_call(call);
    auto id = call.arg(id_index).toUInt();
    auto obj = bind_target(target, id);
    if (obj) {
        obj->bind(call.no);
        return obj;
    } else {
        if (id)
            std::cerr << "Call " << call.no
                      << " " << call.name()
                      << " object " << id << "not found\n";
        return nullptr;
    }
}

template <typename T>
unsigned
GenBoundObjectMap<T>::target_id_from_call(const trace::Call& call) const
{
    return call.arg(0).toUInt();
}

template <typename T>
void
GenBoundObjectMap<T>::collect_currently_bound_objects(GenObject::Queue& objects) const
{
    /* This breaks when we use fbos and results of draws before the target frame */
    for(auto&& o: m_bound_to_target) {
        if (o.second)
            objects.push(o.second);
    }
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
GenBoundObjectMap<T>::bound_to_call_target_untyped(const trace::Call& call) const
{
    return this->bound_to_call_target(call);
}

template <typename T>
typename T::Pointer
GenBoundObjectMap<T>::bound_to_target(unsigned target) const
{
    return m_bound_to_target.at(target);
}

template <typename T>
typename T::Pointer
GenBoundObjectMap<T>::bound_to_call_target(const trace::Call& call) const
{
    auto i = m_bound_to_target.find(this->target_id_from_call(call));
    return i != m_bound_to_target.end() ? i->second : nullptr;
}

template <typename T>
PGenObject
GenBoundObjectMap<T>::by_id_untyped(unsigned id) const
{
    return this->by_id(id);
}

template <typename T>
typename T::Pointer
GenBoundObjectMap<T>::by_id(unsigned id) const
{
    auto obj = m_obj_table.find(id);
    if (obj != m_obj_table.end())
        return obj->second;
    else
        return nullptr;
}

template <typename T>
PTraceCall
GenObjectMap<T>::generate(const trace::Call& call)
{
    const auto ids = call.arg(1).toArray();
    auto c = std::make_shared<TraceCall>(call);
    for (auto& v : ids->values) {
        auto id = v->toUInt();
        this->add(std::make_shared<T>(id, c));
    }
    return c;
}

template <typename T>
PTraceCall
GenObjectMap<T>::destroy(const trace::Call& call)
{
    const auto ids = call.arg(1).toArray();
    for (auto& v : ids->values)
        this->erase(v->toUInt());
    return std::make_shared<TraceCall>(call);
}

template <typename T>
PTraceCall
CreateObjectMap<T>::create(const trace::Call &call)
{
    const auto id = call.ret->toUInt();
    auto c = std::make_shared<TraceCall>(call);
    auto obj = std::make_shared<T>(id, c);
    this->add(obj);
    return c;
}

template <typename T>
PTraceCall
CreateObjectMap<T>::destroy(const trace::Call& call)
{
    const auto id = call.arg(0).toUInt();
    this->erase(id);
    return std::make_shared<TraceCall>(call);
}

}

#endif // BOUNDOBJECTMAP_HPP
