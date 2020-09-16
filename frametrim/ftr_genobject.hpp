#ifndef GENOBJECT_H
#define GENOBJECT_H

#include "ftr_traceobject.hpp"

#include <unordered_map>
#include <queue>
#include <memory>
#include <list>
#include <bitset>

namespace frametrim_reverse {

class GenObject : public TraceObject {
public:
    using Pointer = std::shared_ptr<GenObject>;

    GenObject(unsigned gl_id, PTraceCall gen_call);
    unsigned id() const { return m_id;};

private:
    void collect_generate_call(CallSet& calls) override;

    unsigned m_id;
    PTraceCall m_gen_call;
};

using ObjectSet = GenObject::Queue;
using PGenObject = GenObject::Pointer;

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
