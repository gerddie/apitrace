#ifndef TRACECALL_HPP
#define TRACECALL_HPP

#include <memory>

#include "trace_model.hpp"

namespace frametrim_reverse {

using PCall=std::shared_ptr<trace::Call>;

class TraceCall {
public:
    using Pointer = std::shared_ptr<TraceCall>;

    TraceCall(PCall call);

private:
    unsigned m_trace_call_no;
};

using PTraceCall = TraceCall::Pointer;


using PlightTrace = std::vector<PTraceCall>;

template <typename BindObject>
class TraceCallWithBinding : public TraceCall {
public:
    TraceCallWithBinding(PCall call, typename BindObject::Pointer obj);

private:
    typename BindObject::Pointer m_obj;
};

}

#endif // TRACECALL_HPP
