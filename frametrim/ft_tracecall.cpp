
#include "ft_tracecall.hpp"

#include <sstream>
#include <iostream>


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

namespace frametrim {

TraceCall::TraceCall(const trace::Call &call, const std::string& name):
    m_trace_call_no(call.no),
    m_name(name)
{
    std::stringstream s;
    s << call.name();

    trace::StreamVisitor sv(s);
    for(auto&& a: call.args) {
        s << "_";  a.value->visit(sv);
    }

    m_name_with_params = s.str();
}

TraceCall::TraceCall(const trace::Call& call):
    TraceCall(call, call.name())
{
}

bool TraceCall::is_recorded_at(unsigned reference_call) const
{
    return false && m_recorded_at <= reference_call;
}

void TraceCall::record_at(unsigned reference_call)
{
    m_recorded_at = reference_call;
}

CallSet::CallSet():
    m_reference_call_no(0)
{

}

void CallSet::set_reference_call_no(unsigned callno)
{
    m_reference_call_no = callno;
}

void CallSet::insert(PTraceCall call)
{
    if (!call)
        return;

    if (!call->is_recorded_at(m_reference_call_no)) {
        m_calls.insert(call);
        call->record_at(m_reference_call_no);
    }
}

void CallSet::insert(const CallSet& set)
{
    for(auto&& c : set)
        insert(c);
}

void CallSet::insert(const StateCallMap& map)
{
    for(auto&& c : map)
        insert(c.second);
}

void CallSet::clear()
{
    m_calls.clear();
}

bool CallSet::empty() const
{
    return m_calls.empty();
}

CallSet::const_iterator
CallSet::begin() const
{
    return m_calls.begin();
}

CallSet::const_iterator
CallSet::end() const
{
    return m_calls.end();
}

}
