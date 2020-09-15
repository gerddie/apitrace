#ifndef TRACECALL_HPP
#define TRACECALL_HPP

#include "ftr_genobject.hpp"

#include "trace_model.hpp"
#include <queue>
#include <memory>
#include <bitset>

namespace frametrim_reverse {

using PCall=std::shared_ptr<trace::Call>;
using TraceCallRange = std::pair<unsigned, unsigned>;

class TraceCall {
public:
    using Pointer = std::shared_ptr<TraceCall>;

    enum Flags {
        required,
        recorded,
        single_state,
        repeatable_state,
        full_viewport_redraw,
        framebuffer_op,
        matrix_reset,
        matrix_select,
        matrix_data,
        buffer_data_reset,
        buffer_map,
        buffer_unmap,
        last_flag
    };

    TraceCall(const trace::Call &call, const std::string& name, bool is_state_call);

    TraceCall(const trace::Call& call);

    void set_flag(Flags f) {m_flags.set(f);}
    bool test_flag(Flags f) {return m_flags.test(f);}

    unsigned call_no() const { return m_trace_call_no;};
    const std::string& name() const { return m_name;}
    const std::string& name_with_params() const { return m_name_with_params;}

    void add_object_to_set(ObjectSet& out_set) const;
    void add_object_calls(CallIdSet& out_calls) const;

private:
    virtual void add_dependend_objects(ObjectSet& out_set) const;
    virtual void add_dependend_object_calls(CallIdSet& out_calls) const;

    unsigned m_trace_call_no;
    std::string m_name;
    std::string m_name_with_params;
    std::bitset<last_flag> m_flags;
};
using PTraceCall = TraceCall::Pointer;

class CallIdSet {
public:
    void insert(PTraceCall call);
    void merge(const CallIdSet& set);
    size_t size() const {return m_calls.size(); }
    std::unordered_set<PTraceCall>::const_iterator begin() const;
    std::unordered_set<PTraceCall>::const_iterator end() const;

private:
    std::unordered_set<PTraceCall> m_calls;
};



using LightTrace = std::vector<PTraceCall>;


}

#endif // TRACECALL_HPP
