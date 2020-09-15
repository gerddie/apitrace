#ifndef GENOBJECT_H
#define GENOBJECT_H

#include "ft_common.hpp"

#include <unordered_map>
#include <queue>
#include <memory>
#include <list>
#include <bitset>

namespace frametrim_reverse {

class CallIdSet;
class TraceCall;
using PTraceCall = std::shared_ptr<TraceCall>;
using TraceCallRange = std::pair<unsigned, unsigned>;

class TraceObject {
public:
    TraceObject();
    using Pointer = std::shared_ptr<TraceObject>;
    using Queue = std::queue<Pointer>;

    bool visited(unsigned callno) const { return m_visited <= callno;}
    void set_visited(unsigned callno) {m_visited = callno;};

    void collect_calls(CallIdSet& calls, unsigned call_before);

    virtual void collect_objects_of_type(Queue& objects, unsigned call,
                                         std::bitset<16> typemask);
protected:
    unsigned collect_last_call_before(CallIdSet& calls,
                                  const std::list<PTraceCall>& call_list,
                                  unsigned call_before);

    void collect_all_calls_before(CallIdSet& calls,
                                  const std::list<PTraceCall>& call_list,
                                  unsigned call_before);

    virtual void collect_generate_call(CallIdSet& calls);
    virtual void collect_allocation_call(CallIdSet& calls);
    virtual void collect_data_calls(CallIdSet& calls, unsigned call_before);
    virtual void collect_bind_calls(CallIdSet& calls, unsigned call_before);
    virtual void collect_state_calls(CallIdSet& calls, unsigned call_before);
private:
    unsigned m_visited;
};
using PTraceObject = TraceObject::Pointer;
using PObjectVector = std::shared_ptr<std::vector<PTraceObject>>;

class GenObject : public TraceObject {
public:
    using Pointer = std::shared_ptr<GenObject>;

    GenObject(unsigned gl_id, PTraceCall gen_call);
    unsigned id() const { return m_id;};

private:
    void collect_generate_call(CallIdSet& calls) override;

    unsigned m_id;
    PTraceCall m_gen_call;
};

using ObjectSet = GenObject::Queue;
using PGenObject = GenObject::Pointer;



class BoundObject : public GenObject {
public:
    using GenObject::GenObject;
    using Pointer=std::shared_ptr<BoundObject>;

    void bind(PTraceCall call) {
        m_bind_calls.push_front(call);
    }

protected:
    void collect_bind_calls(CallIdSet& calls, unsigned call_before) override;

    std::list<PTraceCall> m_bind_calls;
};

struct BindTimePoint {
    PTraceObject obj;
    unsigned bind_call_no;
    unsigned unbind_call_no;

    BindTimePoint(PTraceObject o, unsigned callno):
        obj(o), bind_call_no(callno),
        unbind_call_no(std::numeric_limits<unsigned>::max()) {}
};

class BindTimeline {
public:
    PTraceObject push(unsigned callno, PTraceObject obj);
    PTraceObject unbind_last(unsigned callno);
    PTraceObject active_in_call_range(const TraceCallRange &call_range) const;
    PTraceObject active_at_call(unsigned no) const;
    void collect_active_in_call_range(ObjectSet& objects, const TraceCallRange &call_range) const;
    void collect_currently_active(ObjectSet& objects) const;
private:
    std::list<BindTimePoint> m_timeline;
};

}

#endif // GENOBJECT_H
