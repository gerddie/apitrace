#ifndef TRACECALL_HPP
#define TRACECALL_HPP

#include "trace_model.hpp"

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <bitset>

namespace frametrim {

enum ECallFlags {
    tc_required,
    tc_last
};

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

    void set_flag(ECallFlags flag) { m_flags.set(flag);}
    bool test_flag(ECallFlags flag) { return m_flags.test(flag);}
private:

    unsigned m_trace_call_no;
    unsigned m_recorded_at;
    std::string m_name;
    std::string m_name_with_params;

    std::bitset<tc_last> m_flags;
};
using PTraceCall = TraceCall::Pointer;

using StateCallMap=std::unordered_map<std::string, PTraceCall>;

inline PTraceCall trace2call(const trace::Call& call) {
    return std::make_shared<TraceCall>(call);
}

struct CallHash {
    std::size_t operator () (const PTraceCall& call) const {
        return std::hash<unsigned>{}(call->call_no());
    }
};

class CallSet {
public:
    using const_iterator = std::unordered_set<PTraceCall, CallHash>::const_iterator;

    void insert(PTraceCall call);
    void insert(const CallSet& set);
    void insert(const StateCallMap& map);
    void clear();
    bool empty() const;
    size_t size() const {return m_calls.size(); }
    const_iterator begin() const;
    const_iterator end() const;
protected:
    void insert_into_set(PTraceCall call) {m_calls.insert(call);}
private:
    virtual void do_insert(PTraceCall call);

    std::unordered_set<PTraceCall, CallHash> m_calls;
};

class CallSetWithCycleCounter : public CallSet {
public:
    CallSetWithCycleCounter();
    void set_cycle(unsigned cycle);
private:
    virtual void do_insert(PTraceCall call);
    unsigned m_store_cycle;
};


}

#endif // TRACECALL_HPP
