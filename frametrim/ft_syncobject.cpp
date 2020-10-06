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

    auto fence = get_by_id(id, false);

    auto c = trace2call(call);
    if (!fence) {
        auto obj = std::make_shared<SyncObject>(id, c);
        this->set(id, obj);
    } else {
        fence->append_call(c);
    }
    return c;
}

PTraceCall
SyncObjectMap::wait_or_destroy(const trace::Call& call)
{
    auto id = call.arg(0).toUInt();
    if (!id)
        return trace2call(call);

    auto fence = get_by_id(id, false);

    if (!fence) {
        std::cerr << call.no << " " << call.name() << ": no fence object "
                  << call.arg(0).toUInt() << "\n";
        assert(0);
    }

    return fence->append_call(call);
}

void SyncObjectMap::do_emit_calls_to_list(CallSet& list) const
{
    emit_all_states(list);
}

}


