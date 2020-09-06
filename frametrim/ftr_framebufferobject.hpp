#ifndef FRAMEBUFFEROBJECT_HPP
#define FRAMEBUFFEROBJECT_HPP

#include "ftr_texobject.hpp"

namespace frametrim_reverse {

class FramebufferObject : public GenObject {
public:
    using GenObject::GenObject;

    using Pointer = std::shared_ptr<FramebufferObject>;

    void attach(unsigned index, PAttachableObject obj);

private:
    PAttachableObject m_attachments[10];

    PGenObject m_blit_source;
};
using PFramebufferObject = FramebufferObject::Pointer;

class RenderbufferObject : public AttachableObject {
public:
    using AttachableObject::AttachableObject;
    using Pointer = std::shared_ptr<RenderbufferObject>;
private:
    void evaluate_size(const trace::Call& call) override;
};
using PRenderbufferObject = RenderbufferObject::Pointer;


class RenderbufferObjectMap : public GenObjectMap<RenderbufferObject> {
public:
    PTraceCall storage(const trace::Call& call);
};

class FramebufferObjectMap : public GenObjectMap<FramebufferObject> {
public:
    PTraceCall blit(const trace::Call& call);
    PTraceCall attach_renderbuffer(const trace::Call& call, RenderbufferObjectMap& rb_map);
    PTraceCall attach_texture(const trace::Call& call, TexObjectMap& tex_map);
private:

};

}

#endif // FRAMEBUFFEROBJECT_HPP
