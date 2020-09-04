#ifndef TRACECALL_HPP
#define TRACECALL_HPP

#include <memory>

#include "ftr_genobject.hpp"

#include "trace_model.hpp"
#include <queue>


namespace frametrim_reverse {

using PCall=std::shared_ptr<trace::Call>;

class TraceCall {
public:
    using Pointer = std::shared_ptr<TraceCall>;

    TraceCall(unsigned callno, const std::string& name);

    TraceCall(const trace::Call& call);

    void set_required();

    bool required() const {return m_required;}
    unsigned call_no() const { return m_trace_call_no;};
    const std::string& name() const { return m_name;}

    void add_object_to_set(ObjectSet& out_set) const;

private:
    virtual void add_owned_object(ObjectSet& out_set) const;
    virtual void add_dependend_objects(ObjectSet& out_set) const;

    unsigned m_trace_call_no;
    std::string m_name;
    bool m_required;
};

using PTraceCall = TraceCall::Pointer;

using LightTrace = std::vector<PTraceCall>;


}

#endif // TRACECALL_HPP
