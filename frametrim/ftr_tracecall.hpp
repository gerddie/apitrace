#ifndef TRACECALL_HPP
#define TRACECALL_HPP

#include "ftr_genobject.hpp"

#include <memory>

#include "trace_model.hpp"



namespace frametrim_reverse {

using PCall=std::shared_ptr<trace::Call>;

class TraceCall {
public:
    using Pointer = std::shared_ptr<TraceCall>;

    TraceCall(const trace::Call& call);

    void set_required();

private:
    unsigned m_trace_call_no;
    bool m_required;
};

using PTraceCall = TraceCall::Pointer;

using LightTrace = std::vector<PTraceCall>;


class TraceCallOnBoundObj : public TraceCall {
public:
    TraceCallOnBoundObj(const trace::Call& call, PGenObject obj);

private:
    PGenObject m_object;
};

}

#endif // TRACECALL_HPP
