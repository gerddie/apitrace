#ifndef TRACEOBJECT_HPP
#define TRACEOBJECT_HPP

#include "ftr_objectvisitor.hpp"

#include <memory>
#include <queue>
#include <bitset>
#include <list>
#include <unordered_set>

using TypeFlags = std::bitset<16>;


namespace frametrim_reverse {
class CallSet;
class TraceCall;
using PTraceCall = std::shared_ptr<TraceCall>;

class ReverseCallList {
public:
    using const_iterator = std::list<PTraceCall>::const_iterator;
    using iterator = std::list<PTraceCall>::iterator;

    void push_front(PTraceCall call);
    PTraceCall last_before(unsigned call_no) const;
    PTraceCall last() const;
    const_iterator begin() const {return m_calls.begin();}
    const_iterator end() const {return m_calls.end();}

private:
    std::list<PTraceCall> m_calls;
};

class TraceObject {
public:
    TraceObject();
    using Pointer = std::shared_ptr<TraceObject>;
    using Vector = std::vector<Pointer>;

    bool visited(unsigned callno) const { return m_visited <= callno;}
    void set_visited(unsigned callno) {if (m_visited > callno) m_visited = callno;};

    void collect_calls(CallSet& calls, unsigned call_before);

    virtual void collect_objects_of_type(Vector& objects, unsigned call,
                                         TypeFlags typemask);

    virtual void accept(ObjectVisitor& visitor) = 0;
protected:
    unsigned collect_last_call_before(CallSet& calls,
                                      const ReverseCallList& call_list,
                                      unsigned call_before);

    void collect_all_calls_before(CallSet& calls,
                                  const ReverseCallList& call_list,
                                  unsigned call_before);

    virtual void collect_generate_call(CallSet& calls);
    virtual void collect_allocation_call(CallSet& calls);
    virtual void collect_data_calls(CallSet& calls, unsigned call_before);
    virtual void collect_bind_calls(CallSet& calls, unsigned call_before);
    virtual void collect_state_calls(CallSet& calls, unsigned call_before);
private:
    unsigned m_visited;
    bool m_visiting;
};

using PTraceObject = TraceObject::Pointer;
using ObjectVector = TraceObject::Vector;
using PObjectVector = std::shared_ptr<ObjectVector>;
using ObjectSet = std::unordered_set<PTraceObject>;

}

#endif // TRACEOBJECT_HPP
