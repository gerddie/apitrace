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

class CallSet;

class TraceCall {
public:
    using Pointer = std::shared_ptr<TraceCall>;

    TraceCall(const trace::Call& call, const std::string& name);

    TraceCall(const trace::Call& call, unsigned nparam_sel);

    TraceCall(const trace::Call& call);

    unsigned call_no() const { return m_trace_call_no;};
    const std::string& name() const { return m_name;}
    const std::string& name_with_params() const { return m_name_with_params;}

    bool is_recorded_at(unsigned reference_call) const;
    void record_at(unsigned reference_call);

    void set_flag(ECallFlags flag) { m_flags.set(flag);}
    bool test_flag(ECallFlags flag) { return m_flags.test(flag);}

    void set_required_call(Pointer call);

    void emit_required_calls(CallSet& out_list);
private:

    static std::string name_with_paramsel(const trace::Call& call, unsigned nsel);

    virtual void emit_required_callsets(CallSet& out_list);

    unsigned m_trace_call_no;
    unsigned m_recorded_at;
    std::string m_name;
    std::string m_name_with_params;

    std::bitset<tc_last> m_flags;

    Pointer m_required_call;
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
    using Pointer = std::shared_ptr<CallSet>;

    using const_iterator = std::unordered_set<PTraceCall, CallHash>::const_iterator;

    enum Flag {
        attach_calls,
        last_flag
    };

    void insert(PTraceCall call);
    void insert(const CallSet& set);
    void insert(const StateCallMap& map);
    void clear();
    bool empty() const;
    size_t size() const {return m_calls.size(); }
    const_iterator begin() const;
    const_iterator end() const;

    void set(Flag f) {m_flags.set(f);}
    bool has(Flag f) {return m_flags.test(f);}

    void resolve();
    void insert(unsigned id, Pointer subset);
protected:
    void insert_into_set(PTraceCall call) {m_calls.insert(call);}
private:
    virtual void do_insert(PTraceCall call);

    std::unordered_set<PTraceCall, CallHash> m_calls;
    std::bitset<last_flag> m_flags;
    std::unordered_map<unsigned, Pointer> m_subsets;

};
using PCallSet = std::shared_ptr<CallSet>;

class TraceDrawCall : public TraceCall {
public:
    using TraceCall::TraceCall;

    void insert(PCallSet depends);
private:
    void emit_required_callsets(CallSet& out_list) override;

    PCallSet m_depends;
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
