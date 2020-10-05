#include "ft_syncobject.hpp"

#include <memory>
namespace frametrim {

ObjectType
SyncObject::type() const
{
    return bt_sync_object;
}

PTraceCall
SyncObjectMap::create(const trace::Call& call)
{
    uint64_t id = call.ret->toUInt();
    auto c = trace2call(call);
    auto obj = std::make_shared<SyncObject>(id, c);
    this->set(id, obj);
    return c;
}

PTraceCall
SyncObjectMap::wait(const trace::Call& call)
{
    auto fence = get_by_id(call.arg(0).toUInt());
    assert(fence);
    return fence->append_call(call);
}

PTraceCall
SyncObjectMap::destroy(const trace::Call& call)
{
    auto fence = get_by_id(call.arg(0).toUInt());
    assert(fence);
    return fence->append_call(call);
}

void SyncObjectMap::do_emit_calls_to_list(CallSet& list) const
{
    emit_all_states(list);
}

}


