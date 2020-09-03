#ifndef TRACECALL_HPP
#define TRACECALL_HPP

#include "ftr_genobject.hpp"

#include <memory>

#include "trace_model.hpp"
#include <queue>


namespace frametrim_reverse {

using PCall=std::shared_ptr<trace::Call>;

using ObjectSet = std::queue<PGenObject>;

class TraceCall {
public:
    using Pointer = std::shared_ptr<TraceCall>;

    TraceCall(const trace::Call& call);

    void set_required();

    bool required() const {return m_required;}
    unsigned call_no() const { return m_trace_call_no;};

    void add_object_to_set(ObjectSet& out_set) const;

private:
    virtual void add_owned_object(ObjectSet& out_set) const;
    virtual void add_dependend_objects(ObjectSet& out_set) const;

    unsigned m_trace_call_no;
    bool m_required;
};

using PTraceCall = TraceCall::Pointer;

using LightTrace = std::vector<PTraceCall>;


class TraceCallOnBoundObj : public TraceCall {
public:
    TraceCallOnBoundObj(const trace::Call& call, PGenObject obj);

private:
    void add_owned_object(ObjectSet& out_set) const override;

    PGenObject m_object;
};

/* Currentyl a placeholder, needed for objects that depend on
 * other objects */
class TraceCallOnBoundObjWithDeps : public TraceCallOnBoundObj {
public:
    using TraceCallOnBoundObj::TraceCallOnBoundObj;
};


}

#endif // TRACECALL_HPP
