#ifndef TRACECALL_HPP
#define TRACECALL_HPP

#include "ftr_genobject.hpp"

#include "trace_model.hpp"
#include <queue>
#include <memory>
#include <bitset>


namespace frametrim_reverse {

using PCall=std::shared_ptr<trace::Call>;

class TraceCall {
public:
    using Pointer = std::shared_ptr<TraceCall>;

    enum Flags {
        required,
        single_state
    };

    TraceCall(unsigned callno, const std::string& name, bool is_state_call);

    TraceCall(const trace::Call& call);

    void set_flag(Flags f) {m_flags.set(f);}
    bool test_flag(Flags f) {return m_flags.test(f);}

    unsigned call_no() const { return m_trace_call_no;};
    const std::string& name() const { return m_name;}

    void add_object_to_set(ObjectSet& out_set) const;

private:
    virtual void add_owned_object(ObjectSet& out_set) const;
    virtual void add_dependend_objects(ObjectSet& out_set) const;

    unsigned m_trace_call_no;
    std::string m_name;
    std::bitset<8> m_flags;
};

using PTraceCall = TraceCall::Pointer;

using LightTrace = std::vector<PTraceCall>;


}

#endif // TRACECALL_HPP
