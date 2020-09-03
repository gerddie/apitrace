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

    GenObject(unsigned gl_id);
    unsigned id() const { return m_id;};
    bool visited() const { return m_visited;}
    void set_visited() {m_visited = true;};

    void record(CallIdSet& calls, Queue& objects);
private:

    virtual void record_owned_obj(CallIdSet& calls, Queue& objects);
    virtual void record_dependend_obj(CallIdSet& calls, Queue& objects);

    unsigned m_id;
    bool m_visited;
};

using ObjectSet = GenObject::Queue;
using PGenObject = GenObject::Pointer;

}

#endif // GENOBJECT_H
