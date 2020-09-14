#include "ftr_texobject.hpp"
#include "ftr_tracecallext.hpp"
#include "ftr_boundobject.hpp"

#include <GL/gl.h>
#include <GL/glext.h>
#include <sstream>

namespace frametrim_reverse {

using std::make_shared;
using std::vector;

TexObject::TexObject(unsigned gl_id, PTraceCall gen_call):
    AttachableObject(gl_id, gen_call),
    m_dimensions(0),
    m_max_level(0)
{

}

PTraceCall TexObject::state(const trace::Call& call, unsigned nparam)
{
    auto c = make_shared<StateCall>(call, nparam);
    m_state_calls.push_front(std::make_pair(call.no, c));
    return c;
}

PTraceCall TexObject::sub_image(const trace::Call& call, PGenObject read_buffer)
{
    unsigned level = call.arg(1).toUInt();
    unsigned x = call.arg(2).toUInt();
    unsigned y = 0;
    unsigned z = 0;
    unsigned width = 1;
    unsigned height = 1;
    unsigned depth = 1;

    if (m_max_level <= level)
        m_max_level = level + 1;

    bool needs_readbuffer = false;

    switch (m_dimensions) {
    case 1: width = call.arg(3).toUInt();
        needs_readbuffer = !call.arg(6).toPointer();
        break;
    case 2:
        y = call.arg(3).toUInt();
        width = call.arg(4).toUInt();
        height = call.arg(5).toUInt();
        needs_readbuffer = !call.arg(8).toPointer();
        break;
    case 3:
        y = call.arg(3).toUInt();
        z = call.arg(4).toUInt();
        width = call.arg(5).toUInt();
        height = call.arg(6).toUInt();
        depth = call.arg(7).toUInt();
        needs_readbuffer = !call.arg(10).toPointer();
        break;
    }

    auto c = make_shared<TexSubImageCall>(call, level, x, y, z,
                                          width, height, depth,
                                          needs_readbuffer ? read_buffer : nullptr);
    m_data_calls.push_front(c);
    return c;
}

class TexRegionMerger {
public:

    bool covered(PTexSubImageCall region) {
        for(auto&& r : m_regions) {
            if (r->m_x <= region->m_x &&
                r->m_y <= region->m_y &&
                r->m_z <= region->m_z &&
                r->m_width >= region->m_width &&
                r->m_height >= region->m_height &&
                r->m_depth >= region->m_depth)
                return true;
        }
        m_regions.push_back(region);
        return false;
    }

    vector<PTexSubImageCall> m_regions;
};

void TexObject::collect_data_calls(CallIdSet& calls, unsigned call_before)
{
    vector<TexRegionMerger> regions(m_max_level, TexRegionMerger());

    for(auto&& c : m_data_calls) {
        if (c->call_no() >= call_before)
            continue;

        if (regions[c->m_level].covered(c))
            continue;
        call_before = c->call_no();
        calls.insert(c);
        collect_last_call_before(calls, m_bind_calls, c->call_no());
    }
    collect_bind_calls(calls, call_before);
    collect_allocation_call(calls);
}

unsigned TexObject::evaluate_size(const trace::Call& call)
{
    unsigned level = call.arg(1).toUInt();
    unsigned w = call.arg(3).toUInt();
    unsigned h = 1;

    m_dimensions  = 1;

    if (!strcmp(call.name(), "glTexImage2D")||
        !strcmp(call.name(), "glCompressedTexImage2D"))
        m_dimensions  = 2;
    if (!strcmp(call.name(), "glTexImage3D"))
        m_dimensions  = 3;

    if (m_dimensions > 1)
        h = call.arg(4).toUInt();

    set_size(level, w, h);
    return level;
}

void TexObject::collect_state_calls(CallIdSet& calls, unsigned call_before)
{
    std::unordered_set<std::string> states;
    for (auto&& c: m_state_calls) {
        if (c.first < call_before &&
                states.find(c.second->name()) == states.end()) {
            calls.insert(c.second);
            states.insert(c.second->name());
            collect_last_call_before(calls, m_bind_calls, c.second->call_no());
        }
    }
}

TexObjectMap::TexObjectMap():
    m_active_texture_unit(0)
{

}

PTraceCall
TexObjectMap::active_texture(const trace::Call& call)
{
    m_active_texture_unit = call.arg(0).toUInt() - GL_TEXTURE0;
    return make_shared<TraceCall>(call);
}

PTraceCall
TexObjectMap::allocation(const trace::Call& call)
{
    auto texture = bound_to_call_target(call);
    return texture->allocate(call);
}

PTraceCall
TexObjectMap::bind_multitex(const trace::Call& call)
{
    auto unit = call.arg(0).toUInt() - GL_TEXTURE0;
    auto target =  call.arg(1).toUInt();
    auto id = call.arg(2).toUInt();

    auto target_id = compose_target_id_with_unit(target, unit);

    auto obj = bind_target(target_id, id);
    if (obj) {
        auto c = make_shared<TraceCallOnBoundObj>(call, obj);
        obj->bind(c);
        return c;
    }
    return make_shared<TraceCall>(call);
}

PTraceCall
TexObjectMap::state(const trace::Call& call, unsigned num_param)
{
    auto texture = bound_to_call_target(call);
    return texture->state(call, num_param);
}

unsigned TexObjectMap::target_id_from_call(const trace::Call& call) const
{
    unsigned target = call.arg(0).toUInt();
    return compose_target_id_with_unit(target, m_active_texture_unit);
}

PTraceCall TexObjectMap::sub_image(const trace::Call& call, PGenObject read_buffer)
{
    auto texture = bound_to_call_target(call);
    return texture->sub_image(call, read_buffer);
}

unsigned TexObjectMap::active_unit() const
{
    return m_active_texture_unit;
}

unsigned TexObjectMap::compose_target_id_with_unit(unsigned target,
                                                   unsigned unit) const
{
    switch (target) {
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        target = GL_TEXTURE_CUBE_MAP;
        break;
    case GL_PROXY_TEXTURE_1D:
        target = GL_TEXTURE_1D;
        break;
    case GL_PROXY_TEXTURE_2D:
        target = GL_TEXTURE_2D;
        break;
    case GL_PROXY_TEXTURE_3D:
        target = GL_TEXTURE_3D;
        break;
    default:
        ;
    }
    return (target << 8) | unit;
}

}

