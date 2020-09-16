#ifndef GLOBALSTATEOBJECT_HPP
#define GLOBALSTATEOBJECT_HPP

#include "ftr_genobject.hpp"
#include "ftr_tracecall.hpp"

#include "trace_model.hpp"

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

    PObjectVector currently_bound_objects_of_type(std::bitset<16> typemask);

    void collect_objects_of_type(ObjectVector& objects, unsigned call,
                                 std::bitset<16> typemask) override;

    void prepend_call(PTraceCall call);

    unsigned get_required_calls(CallSet &required_calls) const;

    void get_repeatable_states_from_beginning(CallSet &required_calls,
                                              unsigned before) const;

    void get_last_states_before(CallSet &required_calls,
                                unsigned before) const;

    PTraceCall enable_disable(const trace::Call& call);

    PTraceCall state_call(const trace::Call& call, unsigned num_param_selectors);
    PTraceCall repeatable_state_call(const trace::Call& call, unsigned num_param_selectors);

private:
    using ParamMap = std::unordered_map<std::string, std::string>;

    void resolve_repeatable_state_calls(PTraceCall call,
                                        CallSet& callset /* inout */,
                                        ParamMap& param_map) const;


    unsigned buffer_offset(BindType type, unsigned target, unsigned active_unit);

    std::list<PTraceCall> m_trace;

    std::unordered_map<unsigned, BindTimeline> m_bind_timelines;
    std::unordered_map<unsigned, PTraceObject> m_curently_bound;

    mutable bool m_bind_dirty;
    PObjectVector m_curently_bound_shadow;

    std::unordered_map<unsigned, ReverseCallList> m_enable_calls;
    std::unordered_map<std::string, ReverseCallList> m_state_calls;

    std::unordered_map<std::string, std::vector<PTraceCall>> m_repeatable_state_calls;
};
using PGlobalStateObject = GlobalStateObject::Pointer;

}

#endif // GLOBALSTATEOBJECT_HPP
