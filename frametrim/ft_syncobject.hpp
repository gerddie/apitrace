#ifndef SYNCOBJECT_HPP
#define SYNCOBJECT_HPP

#include "ft_objectstate.hpp"

namespace frametrim {

class SyncObject : public ObjectState {
public:
    using ObjectState::ObjectState;
    using Pointer = std::shared_ptr<SyncObject>;
private:
    ObjectType type() const override;
};

class SyncObjectMap : public TObjStateMap<ObjectState> {

public:
    PTraceCall create(const trace::Call& call);
    PTraceCall wait_or_destroy(const trace::Call& call);
private:
    void do_emit_calls_to_list(CallSet& list) const override;
};

}

#endif // SYNCOBJECT_HPP
