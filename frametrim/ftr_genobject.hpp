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

class TraceObject {
public:
    TraceObject();
    using Pointer = std::shared_ptr<TraceObject>;
    using Queue = std::queue<Pointer>;

    bool visited() const { return m_visited;}
    void set_visited() {m_visited = true;};

    void collect_objects(Queue& objects);
    void collect_calls(CallIdSet& calls, unsigned call_before);

protected:
    void collect_last_call_before(CallIdSet& calls,
                                  const std::vector<unsigned>& call_list,
                                  unsigned call_before);
    void collect_all_calls_before(CallIdSet& calls,
                                  const std::vector<unsigned>& call_list,
                                  unsigned call_before);

private:
    virtual void collect_generate_call(CallIdSet& calls);
    virtual void collect_allocation_call(CallIdSet& calls);
    virtual void collect_data_calls(CallIdSet& calls, unsigned call_before);
    virtual void collect_bind_calls(CallIdSet& calls, unsigned call_before);
    virtual void collect_state_calls(CallIdSet& calls, unsigned call_before);

    virtual void collect_owned_obj(Queue& objects);
    virtual void collect_dependend_obj(Queue& objects);

    bool m_visited;
};
using PTraceObject = TraceObject::Pointer;

class GenObject : public TraceObject {
public:
    using Pointer = std::shared_ptr<GenObject>;

    GenObject(unsigned gl_id, unsigned gen_call);
    unsigned id() const { return m_id;};

private:
    void collect_generate_call(CallIdSet& calls) override;

    unsigned m_id;
    unsigned m_gen_call;
};

using ObjectSet = GenObject::Queue;
using PGenObject = GenObject::Pointer;

class BoundObject : public GenObject {
public:
    using GenObject::GenObject;
    using Pointer=std::shared_ptr<BoundObject>;

    void bind(unsigned call_no) {
        m_bind_calls.push_back(call_no);
    }

protected:
    void collect_bind_calls(CallIdSet& calls, unsigned call_before) override;

    std::vector<unsigned> m_bind_calls;

};

}

#endif // GENOBJECT_H
