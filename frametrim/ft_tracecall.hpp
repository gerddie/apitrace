#ifndef TRACECALL_HPP
#define TRACECALL_HPP

#include "trace_model.hpp"

#include <memory>
#include <unordered_set>

namespace frametrim {

class TraceCall {
public:
    using Pointer = std::shared_ptr<TraceCall>;

    TraceCall(const trace::Call &call, const std::string& name);

    TraceCall(const trace::Call& call);

    unsigned call_no() const { return m_trace_call_no;};
    const std::string& name() const { return m_name;}
    const std::string& name_with_params() const { return m_name_with_params;}

    bool is_recorded_at(unsigned reference_call) const;
    void record_at(unsigned reference_call);

private:
    unsigned m_recorded_at;

    unsigned m_trace_call_no;
    std::string m_name;
    std::string m_name_with_params;
};
using PTraceCall = TraceCall::Pointer;


class CallSet {
public:
    CallSet();
    void set_reference_call_no(unsigned callno);
    void insert(PTraceCall call);
    void merge(const CallSet& set);
    size_t size() const {return m_calls.size(); }
    std::unordered_set<PTraceCall>::const_iterator begin() const;
    std::unordered_set<PTraceCall>::const_iterator end() const;

private:
    std::unordered_set<PTraceCall> m_calls;
    unsigned m_reference_call_no;
};

}

#endif // TRACECALL_HPP
