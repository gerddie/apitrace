#include "ftr_tracecall.hpp"

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

namespace frametrim_reverse {

unsigned TraceCall::m_current_draw_framebuffer = 0;

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
    m_fbo_id = m_current_draw_framebuffer;
}

TraceCall::TraceCall(const trace::Call &call):
    TraceCall(call, call.name(), false)
{
}

void TraceCall::add_dependend_object(PTraceObject obj)
{
    if (obj)
        m_dependencies.push_back(obj);
}

void TraceCall::depends_on_call(Pointer call)
{
    m_depends_on.push_back(call);
}

void TraceCall::bind_fbo(unsigned id)
{
    m_current_draw_framebuffer = id;

    // Override value set in contructor
    m_fbo_id = m_current_draw_framebuffer;
    set_flag(draw_framebuffer_bind);
}

void TraceCall::add_object_set(PObjectVector entry_dependencies)
{
    m_entry_dependencies = entry_dependencies;
}

void TraceCall::collect_dependent_objects_in_set(ObjectSet& out_set) const
{
    for (auto&& o : m_dependencies)
        out_set.insert(o);

    if (m_entry_dependencies) {
        for (auto&& o : *m_entry_dependencies)
            out_set.insert(o);
    }
}

void TraceCall::add_object_calls(CallSet& out_calls) const
{
    for(auto&& c : m_depends_on)
        out_calls.insert(c);

    for (auto&& o : m_dependencies)
        o->collect_calls(out_calls, call_no());

    if (m_entry_dependencies) {
        for (auto&& o : *m_entry_dependencies)
            o->collect_calls(out_calls, call_no());
    }
}

void CallSet::insert(PTraceCall call)
{
    if (!call)
        return;
    if (!call->test_flag(TraceCall::recorded)) {
        m_calls.insert(call);
        call->set_flag(TraceCall::recorded);
        call->add_object_calls(*this);
    }
}

void CallSet::merge(const CallSet& set)
{
    m_calls.insert(set.begin(), set.end());
}

std::unordered_set<PTraceCall>::const_iterator
CallSet::begin() const
{
    return m_calls.begin();
}

std::unordered_set<PTraceCall>::const_iterator
CallSet::end() const
{
    return m_calls.end();
}

}
