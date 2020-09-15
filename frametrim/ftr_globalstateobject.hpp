#ifndef GLOBALSTATEOBJECT_HPP
#define GLOBALSTATEOBJECT_HPP

#include "ftr_genobject.hpp"

namespace frametrim_reverse {

enum BindType {
    bt_buffer,
    bt_framebuffer,
    bt_program,
    bt_legacy_program,
    bt_renderbuffer,
    bt_sampler,
    bt_texture,
    bt_vertex_array
};

struct StateCallRecord {
    unsigned last_before_callid;
    StateCallRecord() :
        last_before_callid(std::numeric_limits<unsigned>::min()){
    }
};

using StateCallMap = std::unordered_map<std::string, StateCallRecord>;

class GlobalStateObject : public TraceObject
{
public:
    using Pointer = std::shared_ptr<GlobalStateObject>;

    GlobalStateObject();

    void record_bind(BindType type, PGenObject obj,
                     unsigned id, unsigned tex_unit, unsigned callno);

    void resolve_state_calls(PTraceCall call,
                             CallIdSet& callset /* inout */,
                             unsigned next_required_call);

    void resolve_repeatable_state_calls(PTraceCall call,
                                        CallIdSet& callset /* inout */);

    PObjectVector currently_bound_objects_of_type(std::bitset<16> typemask);

    void collect_objects_of_type(Queue& objects, unsigned call,
                                 std::bitset<16> typemask) override;

private:
    unsigned buffer_offset(BindType type, unsigned target, unsigned active_unit);

    std::unordered_map<unsigned, BindTimeline> m_bind_timelines;

    StateCallMap m_state_calls;
    std::unordered_map<std::string, std::string> m_state_call_param_map;

    std::unordered_map<unsigned, PTraceObject> m_curently_bound;

    mutable bool m_bind_dirty;
    PObjectVector m_curently_bound_shadow;
};

using PGlobalStateObject = GlobalStateObject::Pointer;

}

#endif // GLOBALSTATEOBJECT_HPP
