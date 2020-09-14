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
    bt_vertex_array,
};

class GlobalStateObject : public TraceObject
{
public:
    using Pointer = std::shared_ptr<GlobalStateObject>;
    void record_bind(BindType type, PGenObject obj,
                     unsigned id, unsigned tex_unit, unsigned callno);

private:
    void collect_owned_obj(ObjectSet& required_objects,
                           const TraceCallRange& range) override;
    unsigned buffer_offset(BindType type, unsigned target, unsigned active_unit);

    std::unordered_map<unsigned, BindTimeline> m_bind_timelines;
};

using PGlobalStateObject = GlobalStateObject::Pointer;

}

#endif // GLOBALSTATEOBJECT_HPP
