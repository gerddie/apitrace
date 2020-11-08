#include "ftb_dependecyobject.hpp"

#include <cstring>

#include <GL/gl.h>
#include <GL/glext.h>

namespace frametrim {


UsedObject::UsedObject(unsigned id):
    m_id(id),
    m_emitted(true)
{

}

unsigned
UsedObject::id() const
{
    return m_id;
}

bool
UsedObject::emitted() const
{
    return m_emitted;
}

void
UsedObject::add_call(PTraceCall call)
{
    m_calls.push_back(call);
    m_emitted = false;
}

void
UsedObject::set_call(PTraceCall call)
{
    m_calls.clear();
    add_call(call);
}


void
UsedObject::add_depenency(Pointer dep)
{
    m_dependencies.push_back(dep);
    m_emitted = false;
}

void UsedObject::set_depenency(Pointer dep)
{
    m_dependencies.clear();
    add_depenency(dep);
}

void
UsedObject::emit_calls_to(CallSet& out_list)
{
    if (!m_emitted) {
        m_emitted = true;
        for (auto&& n : m_calls)
            out_list.insert(n);

        for (auto&& o : m_dependencies)
            o->emit_calls_to(out_list);
    }
}

void
DependecyObjectMap::Generate(const trace::Call& call)
{
    auto c = trace2call(call);
    const auto ids = (call.arg(1)).toArray();
    for (auto& v : ids->values) {
        auto obj = std::make_shared<UsedObject>(v->toUInt());
        obj->add_call(c);
        add_object(v->toUInt(), obj);
    }
}

void DependecyObjectMap::Destroy(const trace::Call& call)
{
    auto c = trace2call(call);
    const auto ids = (call.arg(1)).toArray();
    for (auto& v : ids->values) {
        assert(m_objects[v->toUInt()]->id() == v->toUInt());
        m_objects[v->toUInt()]->add_call(c);
    }
}

void DependecyObjectMap::Create(const trace::Call& call)
{
    auto obj = std::make_shared<UsedObject>(call.ret->toUInt());
    add_object(call.ret->toUInt(), obj);
    auto c = trace2call(call);
    obj->add_call(c);    
}

void DependecyObjectMap::Delete(const trace::Call& call)
{
    auto obj = m_objects[call.arg(0).toUInt()];
    auto c = trace2call(call);
    obj->add_call(c);
}


void DependecyObjectMap::add_object(unsigned id, UsedObject::Pointer obj)
{
    m_objects[id] = obj;
}

UsedObject::Pointer
DependecyObjectMap::Bind(const trace::Call& call, unsigned obj_id_param)
{
    unsigned id = call.arg(obj_id_param).toUInt();
    unsigned bindpoint = get_bindpoint_from_call(call);
    return bind_target(id, bindpoint);
}

UsedObject::Pointer
DependecyObjectMap::bind_target(unsigned id, unsigned bindpoint)
{
    if (id) {
        m_bound_object[bindpoint] = m_objects[id];
    } else {
        m_bound_object[bindpoint] = nullptr;
    }
    return m_bound_object[bindpoint];
}

UsedObject::Pointer
DependecyObjectMap::bind( unsigned bindpoint, unsigned id)
{
    return m_bound_object[bindpoint] = m_objects[id];
}

UsedObject::Pointer
DependecyObjectMap::bound_to(unsigned target, unsigned index)
{
    unsigned bindpoint = get_bindpoint(target, index);
    return at_binding(bindpoint);
}

DependecyObjectMap::ObjectMap::iterator
DependecyObjectMap::begin()
{
    return m_bound_object.begin();
}

DependecyObjectMap::ObjectMap::iterator
DependecyObjectMap::end()
{
    return m_bound_object.end();
}

UsedObject::Pointer DependecyObjectMap::at_binding(unsigned index)
{
    return m_bound_object[index];
}

unsigned DependecyObjectMap::get_bindpoint(unsigned target, unsigned index) const
{
    (void)index;
    return target;
}

void
DependecyObjectMap::CallOnBoundObject(const trace::Call& call)
{
    unsigned bindpoint = get_bindpoint_from_call(call);
    if (!m_bound_object[bindpoint])  {
        std::cerr << "no object bound to " << bindpoint << " in call " << call.no << ": " << call.name() << "\n";
        return;
    }

    m_bound_object[bindpoint]->add_call(trace2call(call));
}

UsedObject::Pointer
DependecyObjectMap::BindWithCreate(const trace::Call& call, unsigned obj_id_param)
{
    unsigned bindpoint = get_bindpoint_from_call(call);
    unsigned id = call.arg(obj_id_param).toUInt();
    if (!m_objects[id])  {
        m_objects[id] = std::make_shared<UsedObject>(id);
    }
    return bind_target(id, bindpoint);
}

void
DependecyObjectMap::CallOnObjectBoundTo(const trace::Call& call, unsigned bindpoint)
{
    auto obj = bound_to(bindpoint);

    assert(obj);

    obj->add_call(trace2call(call));
}

void
DependecyObjectMap::CallOnNamedObject(const trace::Call& call)
{
    auto obj = m_objects[call.arg(0).toUInt()];
    if (!obj) {
        if (call.arg(0).toUInt()) {
            std::cerr << "Named object " << call.arg(0).toUInt()
                      << " doesn't exists in call " << call.no << ": "
                      << call.name() << "\n";
            assert(0);
        }
    } else
        obj->add_call(trace2call(call));
}

UsedObject::Pointer
DependecyObjectMap::CallOnBoundObjectWithDep(const trace::Call& call,
                                             DependecyObjectMap& other_objects,
                                             int dep_obj_param,
                                             bool reverse_dep_too)
{
    unsigned bindpoint = get_bindpoint_from_call(call);
    if (!m_bound_object[bindpoint]) {
        std::cerr << "No object bound in call " << call.no << ":" << call.name() << "\n";
        assert(0);
    }

    UsedObject::Pointer obj = nullptr;
    unsigned obj_id = call.arg(dep_obj_param).toUInt();
    if (obj_id) {
        obj = other_objects.get_by_id(obj_id);
        assert(obj);
        m_bound_object[bindpoint]->add_depenency(obj);
        if (reverse_dep_too)
            obj->add_depenency(m_bound_object[bindpoint]);
    }
    m_bound_object[bindpoint]->add_call(trace2call(call));
    return obj;
}

void
DependecyObjectMap::CallOnBoundObjectWithDepBoundTo(const trace::Call& call,
                                                    DependecyObjectMap& other_objects,
                                                    int bindingpoint)
{
    unsigned bindpoint = get_bindpoint_from_call(call);
    if (!m_bound_object[bindpoint]) {
        std::cerr << "No object bound in call " << call.no << ":" << call.name() << "\n";
        assert(0);
    }
    m_bound_object[bindpoint]->add_call(trace2call(call));

    UsedObject::Pointer obj = nullptr;
    auto dep = other_objects.bound_to(bindingpoint);
    if (dep) {
        m_bound_object[bindpoint]->add_depenency(dep);
    }
}

void
DependecyObjectMap::CallOnNamedObjectWithDep(const trace::Call& call,
                                             DependecyObjectMap& other_objects,
                                             int dep_obj_param,
                                             bool reverse_dep_too)
{
    auto obj = m_objects[call.arg(0).toUInt()];

    if (!obj) {
        if (!call.arg(0).toUInt())
            return;

        std::cerr << "Call:" << call.no << ":" << call.name()
                  << " Object " << call.arg(0).toUInt() << "not found\n";
        assert(0);
    }



    unsigned dep_obj_id = call.arg(dep_obj_param).toUInt();
    if (dep_obj_id) {
        auto dep_obj = other_objects.get_by_id(dep_obj_id);
        assert(dep_obj);
        obj->add_depenency(dep_obj);
        if (reverse_dep_too)
            dep_obj->add_depenency(obj);
    }
    obj->add_call(trace2call(call));
}

void DependecyObjectMap::add_call(PTraceCall call)
{
    m_calls.push_back(call);
}


UsedObject::Pointer
DependecyObjectMap::get_by_id(unsigned id) const
{
    auto i = m_objects.find(id);
    return i !=  m_objects.end() ? i->second : nullptr;
}

void
DependecyObjectMap::emit_bound_objects(CallSet& out_calls)
{
    for (auto&& [key, obj]: m_bound_object) {
        if (obj)
            obj->emit_calls_to(out_calls);
    }
    for(auto&& c : m_calls)
        out_calls.insert(c);
}

void DependecyObjectMap::emit_bound_objects_ext(CallSet& out_calls)
{
    (void)out_calls;
}

unsigned
DependecyObjectWithSingleBindPointMap::get_bindpoint_from_call(const trace::Call& call) const
{
    (void)call;
    return 0;
}

unsigned
DependecyObjectWithDefaultBindPointMap::get_bindpoint_from_call(const trace::Call& call) const
{
    return call.arg(0).toUInt();
}

UsedObject::Pointer
BufferObjectMap::bound_to_target(unsigned target, unsigned index)
{
    return bound_to(target, index);
}

enum BufTypes {
    bt_array = 1,
    bt_atomic_counter,
    bt_copy_read,
    bt_copy_write,
    bt_dispatch_indirect,
    bt_draw_indirect,
    bt_element_array,
    bt_pixel_pack,
    bt_pixel_unpack,
    bt_query,
    bt_ssbo,
    bt_texture,
    bt_tf,
    bt_uniform,
    bt_last,
};

unsigned
BufferObjectMap::get_bindpoint(unsigned target, unsigned index) const
{
    switch (target) {
    case GL_ARRAY_BUFFER:
        return bt_array;
    case GL_ATOMIC_COUNTER_BUFFER:
        return bt_atomic_counter + bt_last* index;
    case GL_COPY_READ_BUFFER:
        return bt_copy_read;
    case GL_COPY_WRITE_BUFFER:
        return bt_copy_write;
    case GL_DISPATCH_INDIRECT_BUFFER:
        return bt_dispatch_indirect;
    case GL_DRAW_INDIRECT_BUFFER:
        return bt_draw_indirect;
    case GL_ELEMENT_ARRAY_BUFFER:
        return bt_element_array;
    case GL_PIXEL_PACK_BUFFER:
        return bt_pixel_pack;
    case GL_PIXEL_UNPACK_BUFFER:
        return bt_pixel_unpack;
    case GL_QUERY_BUFFER:
        return bt_query;
    case GL_SHADER_STORAGE_BUFFER:
        return bt_ssbo + bt_last * index;
    case GL_TEXTURE_BUFFER:
        return bt_texture;
    case GL_TRANSFORM_FEEDBACK_BUFFER:
        return bt_tf  + bt_last * index;
    case GL_UNIFORM_BUFFER:
        return bt_uniform + bt_last * index;
    }
    std::cerr << "Unknown buffer binding target=" << target << "\n";
    assert(0);
    return 0;
}

unsigned
BufferObjectMap::get_bindpoint_from_call(const trace::Call& call) const
{
    unsigned target = call.arg(0).toUInt();
    unsigned index = 0;
    if (!strcmp(call.name(), "glBindBufferRange")) {
        index = call.arg(1).toUInt();
    }
    return get_bindpoint(target, index);
}

void
BufferObjectMap::data(const trace::Call& call)
{
    unsigned target = get_bindpoint_from_call(call);
    auto buf = at_binding(target);
    if (buf) {
        m_buffer_sizes[buf->id()] = call.arg(1).toUInt();
        buf->add_call(trace2call(call));
    }
}

void
BufferObjectMap::map(const trace::Call& call)
{
    unsigned target = get_bindpoint_from_call(call);
    auto buf = at_binding(target);
    if (buf) {
        m_mapped_buffers[target][buf->id()] = buf;
        uint64_t begin = call.ret->toUInt();
        uint64_t end = begin + m_buffer_sizes[buf->id()];
        m_buffer_mappings[buf->id()] = std::make_pair(begin, end);
        buf->add_call(trace2call(call));
    }
}

void
BufferObjectMap::map_range(const trace::Call& call)
{
    unsigned target = get_bindpoint_from_call(call);
    auto buf = at_binding(target);
    if (buf) {
        m_mapped_buffers[target][buf->id()] = buf;
        uint64_t begin = call.ret->toUInt();
        uint64_t end = begin + call.arg(2).toUInt();
        m_buffer_mappings[buf->id()] = std::make_pair(begin, end);
        buf->add_call(trace2call(call));
    }
}

void
BufferObjectMap::unmap(const trace::Call& call)
{
    unsigned target = get_bindpoint_from_call(call);
    auto buf = at_binding(target);
    if (buf) {
        m_mapped_buffers[target].erase(buf->id());
        m_buffer_mappings[buf->id()] = std::make_pair(0, 0);
        buf->add_call(trace2call(call));
    }
}

void
BufferObjectMap::memcopy(const trace::Call& call)
{
    uint64_t start = call.arg(0).toUInt();
    unsigned buf_id = 0;
    for(auto&& [id, range ]: m_buffer_mappings) {
        if (range.first <= start && start < range.second) {
            buf_id = id;
            break;
        }
    }
    if (!buf_id) {
        std::cerr << "Found no mapping for memcopy to " << start << " in call " << call.no << ": " << call.name() << "\n";
        assert(0);
    }
    auto buf = get_by_id(buf_id);
    buf->add_call(trace2call(call));
}

void BufferObjectMap::add_ssbo_dependencies(UsedObject::Pointer dep)
{
    for(auto && [key, buf]: *this) {
        if (buf && ((key % bt_last) == bt_ssbo)) {
            buf->add_depenency(dep);
            dep->add_depenency(buf);
        }
    }
}

void
VertexAttribObjectMap::BindAVO(const trace::Call& call, BufferObjectMap& buffers)
{
    unsigned id = call.arg(0).toUInt();
    auto obj = get_by_id(id);
    if (!obj) {
        obj = std::make_shared<UsedObject>(id);
        add_object(id, obj);
    }
    bind(id, id);
    obj->set_call(trace2call(call));

    auto buf = buffers.bound_to_target(GL_ARRAY_BUFFER);
    if (buf) {
        obj->set_depenency(buf);
    }
}

void VertexAttribObjectMap::BindVAOBuf(const trace::Call& call, BufferObjectMap& buffers,
                                       CallSet &out_list, bool emit_dependencies)
{
    unsigned id = call.arg(0).toUInt();
    auto obj = get_by_id(id);
    if (!obj) {
        obj = std::make_shared<UsedObject>(id);
        add_object(id, obj);
    }
    bind(id, id);
    obj->add_call(trace2call(call));

    auto buf = buffers.get_by_id(call.arg(1).toUInt());
    assert(buf || (call.arg(1).toUInt() == 0));
    if (buf) {
        obj->add_depenency(buf);
        if (emit_dependencies) {
            buf->emit_calls_to(out_list);
        }
    }
}

TextureObjectMap::TextureObjectMap():
    m_active_texture(0)
{

}

void
TextureObjectMap::ActiveTexture(const trace::Call& call)
{
    m_active_texture = call.arg(0).toUInt() - GL_TEXTURE0;
    add_call(trace2call(call));
}

enum TexTypes {
    gl_texture_buffer,
    gl_texture_1d,
    gl_texture_2d,
    gl_texture_3d,
    gl_texture_cube,
    gl_texture_1d_array,
    gl_texture_2d_array,
    gl_texture_cube_array,
    gl_texture_2dms,
    gl_texture_2dms_array,
    gl_texture_rectangle,
    gl_texture_last
};

UsedObject::Pointer
TextureObjectMap::BindMultitex(const trace::Call& call)
{
    unsigned unit = call.arg(0).toUInt() - GL_TEXTURE0;
    unsigned target = call.arg(1).toUInt();
    unsigned id = call.arg(2).toUInt();
    unsigned bindpoint  = get_bindpoint_from_target_and_unit(target, unit);
    return bind(bindpoint, id);
}

void TextureObjectMap::Copy(const trace::Call& call)
{
    unsigned src_id = call.arg(0).toUInt();
    unsigned dst_id = call.arg(6).toUInt();

    auto src = get_by_id(src_id);
    auto dst = get_by_id(dst_id);
    assert(src && dst);
    dst->add_call(trace2call(call));
    dst->add_depenency(src);
}

void TextureObjectMap::BindToImageUnit(const trace::Call& call)
{
    unsigned unit = call.arg(0).toUInt();
    unsigned id = call.arg(1).toUInt();

    auto c = trace2call(call);
    if (id) {
        auto tex = get_by_id(id);
        assert(tex);
        tex->add_call(c);
        m_bound_images[unit] = tex;
    } else {
        add_call(c);
        m_bound_images[unit] = nullptr;
    }
}

unsigned
TextureObjectMap::get_bindpoint_from_call(const trace::Call& call) const
{
    unsigned target = call.arg(0).toUInt();
    return get_bindpoint_from_target_and_unit(target, m_active_texture);
}

void TextureObjectMap::emit_bound_objects_ext(CallSet& out_calls)
{
    for(auto [unit, tex]: m_bound_images)
        if (tex)
            tex->emit_calls_to(out_calls);
}

void TextureObjectMap::add_image_dependencies(UsedObject::Pointer dep)
{
    for (auto&& [key, tex]: m_bound_images) {
        if (tex) {
            tex->add_depenency(dep);
            dep->add_depenency(tex);
        }
    }
}

unsigned
TextureObjectMap::get_bindpoint_from_target_and_unit(unsigned target, unsigned unit) const
{
    switch (target) {
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
    case GL_TEXTURE_CUBE_MAP:
        target = gl_texture_cube;
        break;
    case GL_PROXY_TEXTURE_1D:
    case GL_TEXTURE_1D:
        target = gl_texture_1d;
        break;
    case GL_PROXY_TEXTURE_2D:
    case GL_TEXTURE_2D:
        target = gl_texture_2d;
        break;
    case GL_PROXY_TEXTURE_3D:
    case GL_TEXTURE_3D:
        target = gl_texture_3d;
        break;
    case GL_TEXTURE_CUBE_MAP_ARRAY:
        target = gl_texture_cube_array;
        break;
    case GL_TEXTURE_1D_ARRAY:
        target = gl_texture_1d_array;
        break;
    case GL_TEXTURE_2D_ARRAY:
        target = gl_texture_2d_array;
        break;
    case GL_TEXTURE_2D_MULTISAMPLE:
        target = gl_texture_2dms;
        break;
    case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
        target = gl_texture_2dms_array;
        break;
    case GL_TEXTURE_BUFFER:
        target = gl_texture_buffer;
        break;
    case GL_TEXTURE_RECTANGLE:
        target = gl_texture_rectangle;
        break;
    default:
        std::cerr << "target=" << target << "not supported\n";
        assert(0);
        ;
    }
    return target + unit * gl_texture_last;
}

FramebufferObjectMap::FramebufferObjectMap()
{
    auto default_fb = std::make_shared<UsedObject>(0);
    add_object(0, default_fb);
    bind(GL_DRAW_FRAMEBUFFER, 0);
    bind(GL_READ_FRAMEBUFFER, 0);
}

unsigned
FramebufferObjectMap::get_bindpoint_from_call(const trace::Call& call) const
{
    switch (call.arg(0).toUInt()) {
    case GL_FRAMEBUFFER:
    case GL_DRAW_FRAMEBUFFER:
        return GL_DRAW_FRAMEBUFFER;
    case GL_READ_FRAMEBUFFER:
        return GL_READ_FRAMEBUFFER;
    }
    return 0;
}

UsedObject::Pointer
FramebufferObjectMap::bind_target(unsigned id, unsigned bindpoint)
{
    UsedObject::Pointer obj = nullptr;
    if (bindpoint == GL_FRAMEBUFFER ||
        bindpoint == GL_DRAW_FRAMEBUFFER) {
        bind(GL_FRAMEBUFFER, id);
        obj = bind(GL_DRAW_FRAMEBUFFER, id);
    }

    if (bindpoint == GL_FRAMEBUFFER ||
        bindpoint == GL_READ_FRAMEBUFFER)
        obj = bind(GL_READ_FRAMEBUFFER, id);

    return obj;
}

void
FramebufferObjectMap::Blit(const trace::Call& call)
{
    auto dest = bound_to(GL_DRAW_FRAMEBUFFER);
    auto src = bound_to(GL_READ_FRAMEBUFFER);
    assert(dest);
    assert(src);
    dest->add_depenency(src);
    dest->add_call(trace2call(call));
}

void
FramebufferObjectMap::ReadBuffer(const trace::Call& call)
{
    auto fbo = bound_to(GL_READ_FRAMEBUFFER);
    assert(call.arg(0).toUInt() != GL_BACK || fbo->id() == 0);
    CallOnObjectBoundTo(call, GL_READ_FRAMEBUFFER);
}

void FramebufferObjectMap::DrawFromBuffer(const trace::Call& call, BufferObjectMap& buffers)
{
    auto fbo = bound_to(GL_DRAW_FRAMEBUFFER);
    if (fbo->id()) {
        auto buf = buffers.bound_to_target(GL_ELEMENT_ARRAY_BUFFER);
        if (buf)
            fbo->add_depenency(buf);
        fbo->add_call(trace2call(call));
    }
}

}