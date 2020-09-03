#ifndef GENOBJECT_H
#define GENOBJECT_H

#include "ft_common.hpp"

#include <unordered_map>
#include <memory>

namespace frametrim_reverse {

class TraceCall;
using PTraceCall = std::shared_ptr<TraceCall>;

class GenObject
{
public:
    using Pointer = std::shared_ptr<GenObject>;
    GenObject(unsigned gl_id);
    unsigned id() const;
private:
    unsigned m_id;
};

using PGenObject = GenObject::Pointer;

class BoundObjectMap {
public:
    virtual PGenObject bound_to_call_target_untyped(trace::Call& call) = 0;
    virtual PGenObject by_id(unsigned id) = 0;
};

template <typename T>
class GenObjectMap : public BoundObjectMap {
public:
    typename T::Pointer bound_to_call_target(trace::Call& call);

    PTraceCall generate(trace::Call &call);
    PTraceCall destroy(trace::Call& call);
    PTraceCall bind(trace::Call& call, unsigned id_index);
    PGenObject bound_to_call_target_untyped(trace::Call& call) override;
    PGenObject by_id(unsigned id) override;
protected:
    typename T::Pointer bind_target(unsigned target, unsigned id);
private:
    virtual unsigned target_id_from_call(trace::Call& call) const;

    std::unordered_map<unsigned, typename T::Pointer> m_obj_table;

    std::unordered_map<unsigned, typename T::Pointer> m_bound_to_target;
};

}

#endif // GENOBJECT_H
