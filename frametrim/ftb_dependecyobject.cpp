#include "ftb_dependecyobject.hpp"

#include <cstring>

#include <GL/gl.h>
#include <GL/glext.h>

namespace frametrim {


void DependecyObject::add_call(PTraceCall call)
{
    m_calls.push_back(call);
}

void DependecyObject::add_depenency(Pointer dep)
{
    m_dependencies.push_back(dep);
}

void DependecyObject::append_calls(CallSet& out_list)
{
    for (auto&& n : m_calls)
        out_list.insert(n);

    for (auto&& o : m_dependencies)
        o->append_calls(out_list);
}


PTraceCall DependecyObjectMap::Generate(const trace::Call& call)
{
    auto c = trace2call(call);
    const auto ids = (call.arg(1)).toArray();
    for (auto& v : ids->values) {
        auto obj = std::make_shared<DependecyObject>();
        obj->add_call(c);
        add_object(v->toUInt(), obj);
    }
    return c;
}

PTraceCall DependecyObjectMap::Destroy(const trace::Call& call)
{
    auto c = trace2call(call);
    const auto ids = (call.arg(1)).toArray();
    for (auto& v : ids->values) {
        m_objects[v->toUInt()]->add_call(c);
    }
    return c;
}

PTraceCall DependecyObjectMap::Create(const trace::Call& call)
{
    auto obj = std::make_shared<DependecyObject>();
    add_object(call.ret->toUInt(), obj);
    auto c = trace2call(call);
    obj->add_call(c);
    return c;
}

PTraceCall DependecyObjectMap::Delete(const trace::Call& call)
{
    auto obj = m_objects[call.arg(0).toUInt()];
    auto c = trace2call(call);
    obj->add_call(c);
    return c;
}


void DependecyObjectMap::add_object(unsigned id, DependecyObject::Pointer obj)
{
    m_objects[id] = obj;
}

PTraceCall DependecyObjectMap::Bind(const trace::Call& call, unsigned obj_id_param)
{
    unsigned id = call.arg(obj_id_param).toUInt();
    unsigned bindpoint = get_bindpoint_from_call(call);

    bind_target(id, bindpoint);

    auto c = trace2call(call);
    m_calls.push_back(c);
    return c;
}

void DependecyObjectMap::bind_target(unsigned id, unsigned bindpoint)
{
    if (id) {
        m_bound_object[bindpoint] = m_objects[id];
    } else {
        m_bound_object[bindpoint] = nullptr;
    }
}

void DependecyObjectMap::bind(unsigned id, unsigned bindpoint)
{
    m_bound_object[bindpoint] = m_objects[id];
}

DependecyObject::Pointer
DependecyObjectMap::bound_to(unsigned bindpoint)
{
    return m_objects[bindpoint];
}

PTraceCall DependecyObjectMap::CallOnBoundObject(const trace::Call& call)
{
    unsigned bindpoint = get_bindpoint_from_call(call);
    assert(m_bound_object[bindpoint]);
    auto c = trace2call(call);
    m_bound_object[bindpoint]->add_call(c);
    return c;
}

PTraceCall DependecyObjectMap::CallOnObjectBoundTo(const trace::Call& call, unsigned bindpoint)
{
    assert(m_bound_object[bindpoint]);
    auto c = trace2call(call);
    m_bound_object[bindpoint]->add_call(c);
    return c;
}

PTraceCall DependecyObjectMap::CallOnNamedObject(const trace::Call& call)
{
    auto obj = m_objects[call.arg(0).toUInt()];
    assert(obj);
    auto c = trace2call(call);
    obj->add_call(c);
    return c;
}

PTraceCall DependecyObjectMap::CallOnBoundObjectWithDep(const trace::Call& call,
                                                        int dep_obj_param,
                                                        DependecyObjectMap& other_objects)
{
    unsigned bindpoint = get_bindpoint_from_call(call);
    assert(m_bound_object[bindpoint]);
    auto c = trace2call(call);
    unsigned obj_id = call.arg(dep_obj_param).toUInt();
    if (obj_id) {
        auto obj = other_objects.get_by_id(obj_id);
        assert(obj);
        m_bound_object[bindpoint]->add_depenency(obj);
    }
    return c;
}

PTraceCall DependecyObjectMap::CallOnNamedObjectWithDep(const trace::Call& call,
                                                        int dep_obj_param,
                                                        DependecyObjectMap& other_objects)
{
    auto obj = m_objects[call.arg(0).toUInt()];
    auto c = trace2call(call);
    unsigned dep_obj_id = call.arg(dep_obj_param).toUInt();
    if (dep_obj_id) {
        auto dep_obj = other_objects.get_by_id(dep_obj_id);
        assert(dep_obj);
        obj->add_depenency(dep_obj);
    }
    return c;
}

void DependecyObjectMap::add_call(PTraceCall call)
{
    m_calls.push_back(call);
}


DependecyObject::Pointer DependecyObjectMap::get_by_id(unsigned id) const
{
    auto i = m_objects.find(id);
    return i !=  m_objects.end() ? i->second : nullptr;
}

void DependecyObjectMap::emit_bound_objects(CallSet& out_calls)
{
    for (auto&& [key, obj]: m_bound_object) {
        if (obj)
            obj->append_calls(out_calls);
    }
}

unsigned
DependecyObjectWithSingleBindPointMap::get_bindpoint_from_call(const trace::Call& call) const
{
    return 0;
}

unsigned
DependecyObjectWithDefaultBindPointMap::get_bindpoint_from_call(const trace::Call& call) const
{
    return call.arg(0).toUInt();
}

unsigned BufferObjectMap::get_bindpoint_from_call(const trace::Call& call) const
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
    assert(0 && "Unknown buffer binding target");
    return 0;
}

TextureObjectMap::TextureObjectMap():
    m_active_texture(0)
{

}

PTraceCall TextureObjectMap::ActiveTexture(const trace::Call& call)
{
    m_active_texture = call.arg(0).toUInt() - GL_TEXTURE0;
    auto c = trace2call(call);
    add_call(c);
    return c;
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
    auto default_fb = std::make_shared<DependecyObject>();
    add_object(0, default_fb);
    bind(GL_DRAW_FRAMEBUFFER, 0);
    bind(GL_READ_FRAMEBUFFER, 0);
}

unsigned FramebufferObjectMap::get_bindpoint_from_call(const trace::Call& call) const
{
    return call.arg(0).toUInt();
}

void FramebufferObjectMap::bind_target(unsigned id, unsigned bindpoint)
{
    if (bindpoint == GL_FRAMEBUFFER ||
        bindpoint == GL_DRAW_FRAMEBUFFER)
        bind(GL_DRAW_FRAMEBUFFER, id);

    if (bindpoint == GL_FRAMEBUFFER ||
        bindpoint == GL_READ_FRAMEBUFFER)
        bind(GL_READ_FRAMEBUFFER, id);
}

PTraceCall FramebufferObjectMap::Blit(const trace::Call& call)
{
    auto c = trace2call(call);
    auto dest = bound_to(GL_DRAW_FRAMEBUFFER);
    auto src = bound_to(GL_READ_FRAMEBUFFER);
    dest->add_depenency(src);
    dest->add_call(c);
    return c;
}

}