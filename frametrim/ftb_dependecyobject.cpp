#include "ftb_dependecyobject.hpp"

#include <cstring>

#include <GL/gl.h>
#include <GL/glext.h>

namespace frametrim {


DependecyObject::DependecyObject(unsigned id):
    m_id(id)
{

}

unsigned
DependecyObject::id() const
{
    return m_id;
}

void
DependecyObject::add_call(PTraceCall call)
{
    m_calls.push_back(call);
}

void
DependecyObject::add_depenency(Pointer dep)
{
    m_dependencies.push_back(dep);
}

void
DependecyObject::append_calls(CallSet& out_list)
{
    for (auto&& n : m_calls)
        out_list.insert(n);

    for (auto&& o : m_dependencies)
        o->append_calls(out_list);
}

void
DependecyObjectMap::Generate(const trace::Call& call)
{
    auto c = trace2call(call);
    const auto ids = (call.arg(1)).toArray();
    for (auto& v : ids->values) {
        auto obj = std::make_shared<DependecyObject>(v->toUInt());
        obj->add_call(c);
        add_object(v->toUInt(), obj);
    }
}

void DependecyObjectMap::Destroy(const trace::Call& call)
{
    auto c = trace2call(call);
    const auto ids = (call.arg(1)).toArray();
    for (auto& v : ids->values) {
        m_objects[v->toUInt()]->add_call(c);
    }
}

void DependecyObjectMap::Create(const trace::Call& call)
{
    auto obj = std::make_shared<DependecyObject>(call.ret->toUInt());
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


void DependecyObjectMap::add_object(unsigned id, DependecyObject::Pointer obj)
{
    m_objects[id] = obj;
}

DependecyObject::Pointer
DependecyObjectMap::Bind(const trace::Call& call, unsigned obj_id_param)
{
    unsigned id = call.arg(obj_id_param).toUInt();
    unsigned bindpoint = get_bindpoint_from_call(call);
    return bind_target(id, bindpoint);
}

DependecyObject::Pointer
DependecyObjectMap::bind_target(unsigned id, unsigned bindpoint)
{
    if (id) {
        m_bound_object[bindpoint] = m_objects[id];
    } else {
        m_bound_object[bindpoint] = nullptr;
    }
    return m_bound_object[bindpoint];
}

DependecyObject::Pointer
DependecyObjectMap::bind( unsigned bindpoint, unsigned id)
{
    return m_bound_object[bindpoint] = m_objects[id];
}

DependecyObject::Pointer
DependecyObjectMap::bound_to(unsigned bindpoint)
{
    return m_bound_object[bindpoint];
}

void
DependecyObjectMap::CallOnBoundObject(const trace::Call& call)
{
    unsigned bindpoint = get_bindpoint_from_call(call);
    if (!m_bound_object[bindpoint])  {
        std::cerr << "required object not bound in call " << call.no << ": " << call.name() << "\n";
        assert(0);
    }

    m_bound_object[bindpoint]->add_call(trace2call(call));
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
        std::cerr << "Named object doesn't exists in call " << call.no << ": "
                  << call.name() << "\n";
        assert(obj);
    }
    obj->add_call(trace2call(call));
}

void
DependecyObjectMap::CallOnBoundObjectWithDep(const trace::Call& call,
                                             int dep_obj_param,
                                             DependecyObjectMap& other_objects)
{
    unsigned bindpoint = get_bindpoint_from_call(call);
    if (!m_bound_object[bindpoint]) {
        std::cerr << "No object bound in call " << call.no << ":" << call.name() << "\n";
        assert(0);
    }

    unsigned obj_id = call.arg(dep_obj_param).toUInt();
    if (obj_id) {
        auto obj = other_objects.get_by_id(obj_id);
        assert(obj);
        m_bound_object[bindpoint]->add_depenency(obj);
    }
    m_bound_object[bindpoint]->add_call(trace2call(call));
}

void
DependecyObjectMap::CallOnNamedObjectWithDep(const trace::Call& call,
                                             int dep_obj_param,
                                             DependecyObjectMap& other_objects)
{
    auto obj = m_objects[call.arg(0).toUInt()];
    unsigned dep_obj_id = call.arg(dep_obj_param).toUInt();
    if (dep_obj_id) {
        auto dep_obj = other_objects.get_by_id(dep_obj_id);
        assert(dep_obj);
        obj->add_depenency(dep_obj);
    }
    obj->add_call(trace2call(call));
}

void DependecyObjectMap::add_call(PTraceCall call)
{
    m_calls.push_back(call);
}


DependecyObject::Pointer
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
            obj->append_calls(out_calls);
    }
    for(auto&& c : m_calls)
        out_calls.insert(c);
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

unsigned
BufferObjectMap::get_bindpoint_from_call(const trace::Call& call) const
{
    unsigned target = call.arg(0).toUInt();
    unsigned index = 0;
    if (!strcmp(call.name(), "glBindBufferRange")) {
        index = call.arg(1).toUInt();
    }

    switch (target) {
    case GL_ARRAY_BUFFER:
        return 1;
    case GL_ATOMIC_COUNTER_BUFFER:
        return 2 + 16 * index;
    case GL_COPY_READ_BUFFER:
        return 3;
    case GL_COPY_WRITE_BUFFER:
        return 4;
    case GL_DISPATCH_INDIRECT_BUFFER:
        return 5;
    case GL_DRAW_INDIRECT_BUFFER:
        return 6;
    case GL_ELEMENT_ARRAY_BUFFER:
        return 7;
    case GL_PIXEL_PACK_BUFFER:
        return 8;
    case GL_PIXEL_UNPACK_BUFFER:
        return 9;
    case GL_QUERY_BUFFER:
        return 10;
    case GL_SHADER_STORAGE_BUFFER:
        return 11 + 16 * index;
    case GL_TEXTURE_BUFFER:
        return 12;
    case GL_TRANSFORM_FEEDBACK_BUFFER:
        return 13  + 16 * index;
    case GL_UNIFORM_BUFFER:
        return 14 + 16 * index;
    }
    std::cerr << "Call " << call.no << ":" << call.name() << "Unknown buffer binding target\n";
    assert(0);
    return 0;
}

void
BufferObjectMap::data(const trace::Call& call)
{
    unsigned target = get_bindpoint_from_call(call);
    auto buf = bound_to(target);
    if (buf) {
        m_buffer_sizes[buf->id()] = call.arg(1).toUInt();
        buf->add_call(trace2call(call));
    }
}

void
BufferObjectMap::map(const trace::Call& call)
{
    unsigned target = get_bindpoint_from_call(call);
    auto buf = bound_to(target);
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
    auto buf = bound_to(target);
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
    auto buf = bound_to(target);
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
    gl_texture_last
};

unsigned
TextureObjectMap::get_bindpoint_from_call(const trace::Call& call) const
{
    unsigned target = call.arg(0).toUInt();
    return get_bindpoint_from_target_and_unit(target, m_active_texture);
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
    default:
        std::cerr << "target=" << target << "not supported\n";
        assert(0);
        ;
    }
    return target + unit * gl_texture_last;
}

FramebufferObjectMap::FramebufferObjectMap()
{
    auto default_fb = std::make_shared<DependecyObject>(0);
    add_object(0, default_fb);
    bind(GL_DRAW_FRAMEBUFFER, 0);
    bind(GL_READ_FRAMEBUFFER, 0);
}

unsigned
FramebufferObjectMap::get_bindpoint_from_call(const trace::Call& call) const
{
    return call.arg(0).toUInt();
}

DependecyObject::Pointer
FramebufferObjectMap::bind_target(unsigned id, unsigned bindpoint)
{
    DependecyObject::Pointer obj = nullptr;
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

}