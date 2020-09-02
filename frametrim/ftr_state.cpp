#include "ftr_state.hpp"
#include "ftr_tracecall.hpp"
#include "ftr_genobject.hpp"
#include "ftr_bufobject.hpp"
#include "ftr_texobject.hpp"


#include <algorithm>
#include <functional>

namespace frametrim_reverse {

using ftr_callback = std::function<PTraceCall(trace::Call&)>;
using std::bind;
using std::placeholders::_1;
using std::placeholders::_2;

struct TraceMirrorImpl {

    void process(const trace::Call& call, bool required);

    PTraceCall call_on_bound_obj(trace::Call &call, BoundObjectMap& map);

    void register_buffer_calls();
    void register_texture_calls();
    void register_program_calls();
    void register_framebuffer_calls();

    BufObjectMap m_buffers;
    TexObjectMap m_textures;

    using CallTable = std::multimap<const char *, ftr_callback, frametrim::string_part_less>;
    CallTable m_call_table;

    LightTrace trace;
};



TraceMirror::TraceMirror()
{
    impl = new TraceMirrorImpl;
}


TraceMirror::~TraceMirror()
{
    delete impl;
}

void TraceMirror::process(const trace::Call& call, bool required)
{
    impl->process(call, required);
}

void TraceMirrorImpl::process(const trace::Call& call, bool required)
{

}

PTraceCall TraceMirrorImpl::call_on_bound_obj(trace::Call &call, BoundObjectMap& map)
{
    auto bound_obj = map.bound_to_call_target_untyped(call);
    return PTraceCall(new TraceCallOnBoundObj(call, bound_obj));
}

#define MAP(name, call) m_call_table.insert(std::make_pair(#name, bind(&TraceMirrorImpl::call, this, _1)))
#define MAP_DATA(name, call, data) m_call_table.insert(std::make_pair(#name, bind(&TraceMirrorImpl::call, this, _1, data)))

#define MAP_GENOBJ(name, obj, call) \
    m_call_table.insert(std::make_pair(#name, bind(&call, &obj, _1)))
#define MAP_GENOBJ_DATA(name, obj, call, data) \
    m_call_table.insert(std::make_pair(#name, bind(&call, &obj, _1, data)))

void TraceMirrorImpl::register_buffer_calls()
{
    MAP_GENOBJ(glGenBuffers, m_buffers, BufObjectMap::generate);
    MAP_GENOBJ(glDeleteBuffers, m_buffers, BufObjectMap::destroy);
    MAP_GENOBJ_DATA(glBindBuffer, m_buffers, BufObjectMap::bind, 1);

    MAP_GENOBJ(glBufferData, m_buffers, BufObjectMap::data);
    MAP_DATA(glBufferSubData, call_on_bound_obj, m_buffers);
    MAP_GENOBJ(glMapBuffer, m_buffers, BufObjectMap::map);
    MAP_GENOBJ(glMapBufferRange, m_buffers, BufObjectMap::map_range);
    MAP_GENOBJ(glUnmapBuffer, m_buffers, BufObjectMap::unmap);
    MAP_GENOBJ(memcpy, m_buffers, BufObjectMap::memcopy);
}

void TraceMirrorImpl::register_texture_calls()
{
    MAP_GENOBJ(glGenTextures, m_textures, TexObjectMap::generate);
    MAP_GENOBJ(glDeleteTextures, m_textures, TexObjectMap::destroy);
    MAP_GENOBJ_DATA(glBindTexture, m_textures, TexObjectMap::bind, 1);

    MAP_GENOBJ(glActiveTexture, m_textures, TexObjectMap::active_texture);
    MAP_GENOBJ(glClientActiveTexture, m_textures, TexObjectMap::active_texture);

    MAP_GENOBJ(glBindMultiTexture, m_textures, TexObjectMap::bind_multitex);

    MAP_DATA(glCompressedTexImage2D, call_on_bound_obj, m_textures);
    MAP_DATA(glGenerateMipmap, call_on_bound_obj, m_textures);
    MAP_DATA(glTexImage1D, call_on_bound_obj, m_textures);
    MAP_DATA(glTexImage2D, call_on_bound_obj, m_textures);
    MAP_DATA(glTexImage3D, call_on_bound_obj, m_textures);
    MAP_DATA(glTexSubImage1D, call_on_bound_obj, m_textures);
    MAP_DATA(glTexSubImage2D, call_on_bound_obj, m_textures);
    MAP_DATA(glTexSubImage3D, call_on_bound_obj, m_textures);
    MAP_DATA(glCopyTexSubImage2D, call_on_bound_obj, m_textures);
    MAP_DATA(glTexParameter, call_on_bound_obj, m_textures);
/*
    MAP_GENOBJ(glBindSampler, m_samplers, SamplerStateMap::bind);
    MAP_GENOBJ(glGenSamplers, m_samplers, SamplerStateMap::generate);
    MAP_GENOBJ(glDeleteSamplers, m_samplers, SamplerStateMap::destroy);
    MAP_GENOBJ_DATA(glSamplerParameter, m_samplers, SamplerStateMap::set_state, 2);
*/
}

void TraceMirrorImpl::register_program_calls()
{

}

void TraceMirrorImpl::register_framebuffer_calls()
{

}

}
