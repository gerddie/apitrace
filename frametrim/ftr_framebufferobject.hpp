#ifndef FRAMEBUFFEROBJECT_HPP
#define FRAMEBUFFEROBJECT_HPP

#include "ftr_texobject.hpp"

namespace frametrim_reverse {

class FramebufferObject : public GenObject {
public:
    FramebufferObject(unsigned gl_id, unsigned gen_call);

    using Pointer = std::shared_ptr<FramebufferObject>;

    void attach(unsigned index, PAttachableObject obj, unsigned layer);

    void viewport(const trace::Call& call);
    void set_state(PTraceCall call);
    void clear(const trace::Call& call);
    void draw(PTraceCall call);

private:
    void collect_data_calls(CallIdSet& calls, unsigned call_before) override;
    void collect_dependend_obj(Queue& objects) override;

    std::vector<PAttachableObject> m_attachments;
    PGenObject m_blit_source;
    std::list<PTraceCall> m_draw_calls;
    std::list<PTraceCall> m_state_calls;

    unsigned m_viewport_x, m_viewport_y;
    unsigned m_viewport_width, m_viewport_height;

    unsigned m_width, m_height;
};
using PFramebufferObject = FramebufferObject::Pointer;

class RenderbufferObject : public AttachableObject {
public:
    using AttachableObject::AttachableObject;
    using Pointer = std::shared_ptr<RenderbufferObject>;
private:
    unsigned evaluate_size(const trace::Call& call) override;
};
using PRenderbufferObject = RenderbufferObject::Pointer;


class RenderbufferObjectMap : public GenObjectMap<RenderbufferObject> {
public:
    PTraceCall storage(const trace::Call& call);
};

class FramebufferObjectMap : public GenObjectMap<FramebufferObject> {
public:
    PTraceCall blit(const trace::Call& call);
    PTraceCall bind(const trace::Call& call);
    PTraceCall attach_renderbuffer(const trace::Call& call, RenderbufferObjectMap& rb_map);
    PTraceCall attach_texture(const trace::Call& call, TexObjectMap& tex_map,
                              unsigned tex_param_idx, unsigned level_param_idx);
    PTraceCall attach_texture3d(const trace::Call& call, TexObjectMap& tex_map);

    PFramebufferObject draw_buffer() const;
    PFramebufferObject read_buffer() const;

private:
    PFramebufferObject bound_to_call_target(const trace::Call& call) const;

    PFramebufferObject m_draw_buffer;
    PFramebufferObject m_read_buffer;
};

}

#endif // FRAMEBUFFEROBJECT_HPP
