
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

TraceCall::TraceCall(const trace::Call& call, const std::string& name):
    m_trace_call_no(call.no),
    m_recorded_at(0),
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

TraceCall::TraceCall(const trace::Call& call, unsigned nsel):
    TraceCall(call, name_with_paramsel(call, nsel))
{

}

TraceCall::TraceCall(const trace::Call& call):
    TraceCall(call, call.name())
{
}

std::string
TraceCall::name_with_paramsel(const trace::Call &call, unsigned nsel)
{
    std::stringstream s;
    s << call.name();

    trace::StreamVisitor sv(s);
    for(unsigned i = 0; i < nsel; ++i ) {
        s << "_";
        call.args[i].value->visit(sv);
    }
    return s.str();
}

bool TraceCall::is_recorded_at(unsigned reference_call) const
{
    return m_recorded_at >= reference_call;
}

void TraceCall::record_at(unsigned reference_call)
{
    m_recorded_at = reference_call;
}

void TraceCall::emit_required_calls(CallSet& out_list)
{
    if (m_required_call) {
        out_list.insert(m_required_call);
        m_required_call->emit_required_calls(out_list);
    }

    emit_required_callsets(out_list);
}

void TraceCall::set_required_call(Pointer call)
{
    m_required_call = call;
}

void TraceCall::emit_required_callsets(CallSet& out_list)
{
    (void)out_list;
}

void CallSet::insert(PTraceCall call)
{
    if (!call)
        return;
    do_insert(call);
    call->emit_required_calls(*this);
}

void CallSet::insert(const CallSet& set)
{
    for(auto&& c : set.m_calls)
        insert(c);
    for(auto&& c : set.m_subsets)
        insert(c.first, c.second);
}

void CallSet::insert(const StateCallMap& map)
{
    for(auto&& c : map)
        insert(c.second);
}

void CallSet::do_insert(PTraceCall call)
{
    insert_into_set(call);
}

void CallSet::clear()
{
    m_calls.clear();
    m_flags.reset();
}

bool CallSet::empty() const
{
    return m_calls.empty();
}

void CallSet::resolve()
{
    for(auto&& [k, s]: m_subsets) {
        if (s) {
            for (auto&& c: *s)
                insert(c);
        }
    }
    m_subsets.clear();
}

void CallSet::deep_resolve()
{
    for(auto&& [k, s]: m_subsets) {
        if (s) {
            s->deep_resolve();
            for (auto&& c: *s) {
                insert(c);
                c->emit_required_calls(*this);
            }
        }
    }
    m_subsets.clear();
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

void CallSet::insert(unsigned id, Pointer subset)
{
    m_subsets[id] = subset;
}

void TraceDrawCall::append_callset(PCallSet depends)
{
    m_depends.push_back(depends);
}

void TraceDrawCall::emit_required_callsets(CallSet& out_list)
{
    for (auto&& d : m_depends) {
        if (d)
            out_list.insert(*d);
    }
}

CallSetWithCycleCounter::CallSetWithCycleCounter():
    m_store_cycle(0)
{

}

void CallSetWithCycleCounter::set_cycle(unsigned cycle)
{
    m_store_cycle = cycle;
}

void CallSetWithCycleCounter::do_insert(PTraceCall call)
{
    if (!call->is_recorded_at(m_store_cycle)) {
        insert_into_set(call);
        call->record_at(m_store_cycle);
    }
}


}
