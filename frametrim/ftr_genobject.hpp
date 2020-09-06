#ifndef GENOBJECT_H
#define GENOBJECT_H

#include "ft_common.hpp"

#include <unordered_map>
#include <queue>
#include <memory>

namespace frametrim_reverse {

using frametrim::CallIdSet;

class TraceCall;
using PTraceCall = std::shared_ptr<TraceCall>;

class GenObject
{
public:
    using Pointer = std::shared_ptr<GenObject>;
    using Queue = std::queue<Pointer>;

    GenObject(unsigned gl_id, unsigned gen_call);
    unsigned id() const { return m_id;};
    bool visited() const { return m_visited;}
    void set_visited() {m_visited = true;};

    void collect_objects(Queue& objects);
    void collect_calls(CallIdSet& calls, unsigned call_before);
private:
    virtual void collect_allocation_call(CallIdSet& calls);
    virtual void collect_data_calls(CallIdSet& calls, unsigned call_before);
    virtual void collect_state_calls(CallIdSet& calls, unsigned call_before);

    virtual void collect_owned_obj(Queue& objects);
    virtual void collect_dependend_obj(Queue& objects);

    unsigned m_id;
    unsigned m_gen_call;
    bool m_visited;
};

using ObjectSet = GenObject::Queue;
using PGenObject = GenObject::Pointer;

}

#endif // GENOBJECT_H
