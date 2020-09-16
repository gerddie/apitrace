#ifndef TRACEOBJECT_HPP
#define TRACEOBJECT_HPP

#include <memory>
#include <queue>
#include <bitset>
#include <list>

using TypeFlags = std::bitset<16>;


namespace frametrim_reverse {
class CallSet;
class TraceCall;
using PTraceCall = std::shared_ptr<TraceCall>;
using ReverseCallList = std::list<PTraceCall>;

using TraceCallRange = std::pair<unsigned, unsigned>;

class TraceObject {
public:
    TraceObject();
    using Pointer = std::shared_ptr<TraceObject>;
    using Queue = std::queue<Pointer>;

    bool visited(unsigned callno) const { return m_visited <= callno;}
    void set_visited(unsigned callno) {if (m_visited > callno) m_visited = callno;};

    void collect_calls(CallSet& calls, unsigned call_before);

    virtual void collect_objects_of_type(Queue& objects, unsigned call,
                                         TypeFlags typemask);
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
};
using PTraceObject = TraceObject::Pointer;
using PObjectVector = std::shared_ptr<std::vector<PTraceObject>>;


}

#endif // TRACEOBJECT_HPP
