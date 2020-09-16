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

using PGenObject = GenObject::Pointer;

template <typename Pointer>
struct BindTimePoint {
    Pointer obj;
    unsigned bind_call_no;
    unsigned unbind_call_no;

    BindTimePoint(Pointer o, unsigned callno):
        obj(o), bind_call_no(callno),
        unbind_call_no(std::numeric_limits<unsigned>::max()) {}
};

template <typename Pointer>
class BindTimeline {
public:
    Pointer push(unsigned callno, Pointer obj);
    Pointer unbind_last(unsigned callno);
    Pointer active_at_call(unsigned no) const;
    void collect_currently_active(ObjectVector& objects) const;
private:
    std::list<BindTimePoint<Pointer>> m_timeline;
};

template <typename Pointer>
Pointer BindTimeline<Pointer>::push(unsigned callno, Pointer obj)
{
    Pointer last = nullptr;
    if (!m_timeline.empty()) {
        m_timeline.front().unbind_call_no = callno;
        last = m_timeline.front().obj;
    }

    m_timeline.push_front(BindTimePoint<Pointer>(obj, callno));
    return last;
}

template <typename Pointer>
Pointer BindTimeline<Pointer>::unbind_last(unsigned callno)
{
    if (!m_timeline.empty()) {
        m_timeline.front().unbind_call_no = callno;
        return m_timeline.front().obj;
    }
    return nullptr;
}

template <typename Pointer>
Pointer BindTimeline<Pointer>::active_at_call(unsigned no) const
{
    for (auto&& tp: m_timeline) {
        if (tp.bind_call_no <= no  &&
            tp.unbind_call_no > no)
            return tp.obj;
    }
    return nullptr;
}

template <typename Pointer>
void BindTimeline<Pointer>::collect_currently_active(ObjectVector& objects) const
{
    if (!m_timeline.empty() && m_timeline.front().obj)
        objects.push_back(m_timeline.front().obj);
}

}

#endif // GENOBJECT_H
