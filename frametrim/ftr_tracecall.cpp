#include "ftr_tracecall.hpp"

#include <sstream>


namespace trace {

class StreamVisitor : public Visitor
{
public:
    StreamVisitor(std::stringstream& ss): s(ss) {}
    void visit(Null *) override { s << "(null)";}
    void visit(Bool *v) override { s << v->value;}
    void visit(SInt *v) override { s << v->value;}
    void visit(UInt *v) override { s << v->value;}
    void visit(Float *v) override { s << v->value;}
    void visit(Double *v) override { s << v->value;}
    void visit(String *v) override { s << v->value;}
    void visit(WString *v) override { s << v->value;}
    void visit(Enum *v) override { s << v->value;}
    void visit(Bitmask *v) override { s << v->value;}
    void visit(Struct *v) override { s << v;}
    void visit(Array *v) override { s << v;}
    void visit(Blob *v) override { s << v;}
    void visit(Pointer *v) override { s << v->value;}
    void visit(Repr *v) override { s << v;}
protected:
    inline void _visit(Value *value) {
        if (value) {
            value->visit(*this);
        }
    }
    std::stringstream& s;
};

}

namespace frametrim_reverse {

TraceCall::TraceCall(const trace::Call &call, const std::string& name,
                     bool is_state_call):
    m_trace_call_no(call.no),
    m_name(name)
{
    if (is_state_call)
        m_flags.set(single_state);

    std::stringstream s;
    s << call.name();

    trace::StreamVisitor sv(s);
    for(auto&& a: call.args) {
        s << "_";  a.value->visit(sv);
    }

    m_name_with_params = s.str();
}

TraceCall::TraceCall(const trace::Call &call):
    TraceCall(call, call.name(), false)
{
}

void TraceCall::add_object_to_set(ObjectSet& out_set) const
{
    add_dependend_objects(out_set);
}

void TraceCall::add_dependend_objects(ObjectSet& out_set) const
{
    (void)out_set;
}

void TraceCall::add_object_calls(CallIdSet& out_calls) const
{
    add_dependend_object_calls(out_calls);
}

void TraceCall::add_dependend_object_calls(CallIdSet& out_calls) const
{
    (void)out_calls;
}

void CallIdSet::insert(PTraceCall call)
{
    if (!call->test_flag(TraceCall::recorded)) {
        m_calls.insert(call);
        call->set_flag(TraceCall::recorded);
    }
}

void CallIdSet::merge(const CallIdSet& set)
{
    m_calls.insert(set.begin(), set.end());
}

std::unordered_set<PTraceCall>::const_iterator
CallIdSet::begin() const
{
    return m_calls.begin();
}

std::unordered_set<PTraceCall>::const_iterator
CallIdSet::end() const
{
    return m_calls.end();
}

}
