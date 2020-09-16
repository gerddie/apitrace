
#include "ftr_framebufferobject.hpp"
#include "ftr_globalstateobject.hpp"
#include "ftr_matrixobject.hpp"
#include "ftr_programobj.hpp"
#include "ftr_state.hpp"
#include "ftr_texobject.hpp"
#include "ftr_tracecall.hpp"
#include "ftr_vertexattribarray.hpp"

#include <algorithm>
#include <functional>
#include <set>
#include <list>
#include <iostream>

#include <GL/gl.h>
#include <GL/glext.h>

namespace frametrim_reverse {

using ftr_callback = std::function<PTraceCall(trace::Call&)>;
using std::bind;
using std::make_shared;
using std::placeholders::_1;
using std::placeholders::_2;

struct TraceMirrorImpl {
    TraceMirrorImpl();

    void process(trace::Call &call, bool required);

    PTraceCall call_on_named_obj(trace::Call &call, BoundObjectMap& map);
    PTraceCall record_call(trace::Call &call);
    PTraceCall record_state_call(trace::Call &call, unsigned num_name_params,
                                 TraceCall::Flags calltype);
    PTraceCall record_enable_call(trace::Call &call, const char *basename);

    PTraceCall record_draw(trace::Call &call);
    PTraceCall record_draw_buffer(trace::Call &call);
    /* These calls need to be redirected to the currently bound draw framebuffer */
    PTraceCall record_draw_with_buffer(trace::Call &call);
    PTraceCall record_viewport_call(trace::Call &call);
    PTraceCall record_clear_call(trace::Call &call);
    PTraceCall record_framebuffer_state(trace::Call &call);

    PTraceCall bind_fbo(trace::Call &call);
    PTraceCall bind_texture(trace::Call &call);
    PTraceCall bind_buffer(trace::Call &call);
    PTraceCall bind_program(trace::Call &call);
    PTraceCall bind_legacy_program(trace::Call &call);
    PTraceCall bind_renderbuffer(trace::Call &call);
    PTraceCall bind_sampler(trace::Call &call);
    PTraceCall bind_vertex_array(trace::Call &call);

    CallSet resolve();

    void register_state_calls();
    void register_buffer_calls();
    void register_texture_calls();
    void register_program_calls();
    void register_framebuffer_calls();
    void register_glx_state_calls();
    void register_legacy_calls();

    void register_draw_related_calls();

    PTraceCall bind_tracecall(trace::Call &call, PGenObject obj);

    BufObjectMap m_buffers;
    ProgramObjectMap m_programs;
    ShaderObjectMap m_shaders;
    TexObjectMap m_textures;
    FramebufferObjectMap m_fbo;
    RenderbufferObjectMap m_renderbuffers;
    MatrixObjectMap m_matrix_states;
    LegacyProgramObjectMap m_legacy_programs;
    VertexAttribArrayMap m_vertex_attrib_arrays;
    std::shared_ptr<GlobalStateObject> m_global_state;

    PFramebufferObject m_current_draw_buffer;
    PFramebufferObject m_current_read_buffer;

    VertexArrayMap m_va;

    using SamplerObjectMap = GenObjectMap<BoundObject>;
    SamplerObjectMap m_samplers;

    using CallTable = std::multimap<const char *, ftr_callback, frametrim::string_part_less>;
    CallTable m_call_table;

    std::set<std::string> m_unhandled_calls;
    PTraceCall m_last_fbo_bind_call;

};

TraceMirror::TraceMirror()
{
    impl = new TraceMirrorImpl;
}

TraceMirror::~TraceMirror()
{
    delete impl;
}

std::vector<unsigned> TraceMirror::trace() const
{
    CallSet calls = impl->resolve();

    std::unordered_set<unsigned> make_sure_its_singular;

    for(auto&& c: calls)
        make_sure_its_singular.insert(c->call_no());

    std::vector<unsigned> retval(make_sure_its_singular.begin(), make_sure_its_singular.end());
    std::sort(retval.begin(), retval.end());

    return retval;
}

TraceMirrorImpl::TraceMirrorImpl()
{
    m_global_state = make_shared<GlobalStateObject>();
    register_state_calls();
    register_buffer_calls();
    register_texture_calls();
    register_program_calls();
    register_framebuffer_calls();
    register_glx_state_calls();
    register_legacy_calls();
    register_draw_related_calls();
}

void TraceMirror::process(trace::Call &call, bool required)
{
    impl->process(call, required);
}

void TraceMirrorImpl::process(trace::Call& call, bool required)
{
    using frametrim::equal_chars;
    auto cb_range = m_call_table.equal_range(call.name());
    PTraceCall c;
    if (cb_range.first != m_call_table.end() &&
            std::distance(cb_range.first, cb_range.second) > 0) {

        CallTable::const_iterator cb = cb_range.first;
        CallTable::const_iterator i = cb_range.first;
        ++i;

        unsigned max_equal = equal_chars(cb->first, call.name());

        while (i != cb_range.second && i != m_call_table.end()) {
            auto n = equal_chars(i->first, call.name());
            if (n > max_equal) {
                max_equal = n;
                cb = i;
            }
            ++i;
        }
        c = cb->second(call);

    } else {
        if (m_unhandled_calls.find(call.name()) == m_unhandled_calls.end()) {
            std::cerr << "Call " << call.no
                      << " " << call.name() << " not handled\n";
            m_unhandled_calls.insert(call.name());
        }
        /* Add it to the trace anyway, there is a chance that it doesn't
         * need special handling */
        c = std::make_shared<TraceCall>(call);
    }
    if (c) {
        if (required)
            c->set_flag(TraceCall::required);
        m_global_state->prepend_call(c);
    }
}

PTraceCall
TraceMirrorImpl::bind_fbo(trace::Call &call)
{
    auto retval = m_fbo.bind(call);

    unsigned target = call.arg(0).toUInt();

    PFramebufferObject fbo;

    switch (target) {
    case GL_FRAMEBUFFER:
        m_global_state->record_bind(bt_framebuffer, m_fbo.read_buffer(), GL_READ_FRAMEBUFFER, 0, call.no);
        /* fallthrough */
    case GL_DRAW_FRAMEBUFFER:
        m_global_state->record_bind(bt_framebuffer, m_fbo.draw_buffer(), GL_DRAW_FRAMEBUFFER, 0, call.no);
        fbo = m_fbo.draw_buffer();
        PTraceCall c;
        if (fbo) {
            if (!m_current_draw_buffer ||
                fbo->id() != m_current_draw_buffer->id()) {
                m_current_draw_buffer = fbo;
                auto cc = make_shared<TraceCallOnBoundObj>(call, fbo);
                cc->add_object_set(m_global_state->currently_bound_objects_of_type(std::bitset<16>(0xffff)));
                c = cc;
            }
        } else {
            c = make_shared<TraceCall>(call);
        }
        m_last_fbo_bind_call = c;
        return c;
    }

    if (target == GL_FRAMEBUFFER) {



    } else if (target == GL_DRAW_FRAMEBUFFER) {
        m_global_state->record_bind(bt_framebuffer, m_fbo.draw_buffer(), GL_DRAW_FRAMEBUFFER, 0, call.no);
        fbo = m_fbo.draw_buffer();
    } else {
        assert(target == GL_READ_FRAMEBUFFER);
        m_global_state->record_bind(bt_framebuffer, m_fbo.read_buffer(), GL_READ_FRAMEBUFFER, 0, call.no);
        fbo = m_current_read_buffer = m_fbo.read_buffer();
    }


    return bind_tracecall(call, fbo);
}

PTraceCall TraceMirrorImpl::bind_texture(trace::Call &call)
{
    auto texture = m_textures.bind(call, 1);
    auto target = call.arg(0).toUInt();

    m_global_state->record_bind(bt_texture, texture, target, m_textures.active_unit(), call.no);
    return bind_tracecall(call, texture);
}

PTraceCall TraceMirrorImpl::bind_buffer(trace::Call &call)
{
    auto buffer = m_buffers.bind(call, 1);
    auto target = call.arg(0).toUInt();
    m_global_state->record_bind(bt_buffer, buffer, target, 0, call.no);
    return bind_tracecall(call, buffer);
}

PTraceCall TraceMirrorImpl::bind_program(trace::Call &call)
{
    auto prog = m_programs.bind(call, 0);
    m_global_state->record_bind(bt_program, prog, 0, 0, call.no);
    return bind_tracecall(call, prog);
}

PTraceCall TraceMirrorImpl::bind_legacy_program(trace::Call &call)
{
    auto prog = m_legacy_programs.bind(call, 1);
    auto target = call.arg(0).toUInt();
    m_global_state->record_bind(bt_legacy_program, prog, target, 0, call.no);
    return bind_tracecall(call, prog);
}

PTraceCall TraceMirrorImpl::bind_renderbuffer(trace::Call &call)
{
    auto rb = m_renderbuffers.bind(call, 1);
    m_global_state->record_bind(bt_renderbuffer, rb, 0, 0, call.no);
    return bind_tracecall(call, rb);
}

PTraceCall TraceMirrorImpl::bind_sampler(trace::Call &call)
{
    auto sampler = m_samplers.bind(call, 1);
    auto target = call.arg(0).toUInt();
    m_global_state->record_bind(bt_sampler, sampler, target, 0, call.no);
    return bind_tracecall(call, sampler);
}

PTraceCall TraceMirrorImpl::bind_vertex_array(trace::Call &call)
{
    auto va = m_va.bind(call, 0);
    m_global_state->record_bind(bt_vertex_array, va, 0, 0, call.no);
    return bind_tracecall(call, va);
}

PTraceCall TraceMirrorImpl::bind_tracecall(trace::Call &call, PGenObject obj)
{
    return obj ? make_shared<TraceCallOnBoundObj>(call, obj):
                 make_shared<TraceCall>(call);
}

PTraceCall
TraceMirrorImpl::call_on_named_obj(trace::Call &call, BoundObjectMap& map)
{
    auto bound_obj = map.by_id_untyped(call.arg(0).toUInt());
    if (bound_obj)
        return make_shared<TraceCallOnBoundObj>(call, bound_obj);
    else
        return make_shared<TraceCall>(call);
}

PTraceCall
TraceMirrorImpl::record_call(trace::Call &call)
{
    return make_shared<TraceCall>(call);
}

PTraceCall
TraceMirrorImpl::record_state_call(trace::Call &call, unsigned num_name_params,
                                   TraceCall::Flags calltype)
{
    auto c = make_shared<StateCall>(call, num_name_params);
    c->set_flag(calltype);
    return c;
}

PTraceCall
TraceMirrorImpl::record_enable_call(trace::Call &call, const char *basename)
{
    return make_shared<StateEnableCall>(call, basename);
}

CallSet
TraceMirrorImpl::resolve()
{
    CallSet required_calls;

    unsigned first_target_frame_call = m_global_state->get_required_calls(required_calls);

    /* Now collect all calls from the beginning that refere to states that
     * can be changed repeatandly (glx and egl stuff) and where we must use
     * the eraliest calls possible
     * Note: in m_trace call numbera are counting up */

    m_global_state->get_repeatable_states_from_beginning(required_calls,
                                                         first_target_frame_call);

    ObjectSet required_objects;
    m_global_state->collect_objects_of_type(required_objects,
                                            first_target_frame_call,
                                            std::bitset<16>(0xffff));

    while (!required_objects.empty()) {
        auto obj = required_objects.front();
        required_objects.pop();
        /* This is just a fail save, there should be no visited objects
         * in the queue */
        obj->collect_calls(required_calls, first_target_frame_call);
    }

    /* At this point only state calls should remain to be recorded
     * So go in reverse to the list and add them. */
    m_global_state->get_last_states_before(required_calls, first_target_frame_call);

    return required_calls;
}

PTraceCall TraceMirrorImpl::record_draw_with_buffer(trace::Call &call)
{
    auto c = make_shared<TraceCallOnBoundObj>(call);

    std::bitset<16> typemask((1 << 16) - 1);
        typemask.flip(bt_framebuffer);

    auto bindings = m_global_state->currently_bound_objects_of_type(typemask);
    c->add_object_set(bindings);

    if (m_current_draw_buffer)
        m_current_draw_buffer->draw(c);
    return c;
}

PTraceCall TraceMirrorImpl::record_draw(trace::Call &call)
{
    auto c = make_shared<TraceCall>(call);
    if (m_current_draw_buffer)
        m_current_draw_buffer->draw(c);
    return c;
}

PTraceCall TraceMirrorImpl::record_draw_buffer(trace::Call &call)
{
    auto c = make_shared<TraceCall>(call);
    if (m_last_fbo_bind_call)
        c->depends_on_call(m_last_fbo_bind_call);

    if (m_current_draw_buffer)
        m_current_draw_buffer->draw(c);
    return c;
}

PTraceCall TraceMirrorImpl::record_viewport_call(trace::Call &call)
{
    auto c = m_current_draw_buffer ?
                m_current_draw_buffer->viewport(call):
                make_shared<TraceCall>(call);
    c->set_flag(TraceCall::single_state);
    return c;
}

PTraceCall TraceMirrorImpl::record_clear_call(trace::Call &call)
{
    return m_current_draw_buffer ?
                m_current_draw_buffer->clear(call):
                make_shared<TraceCall>(call);
}

PTraceCall TraceMirrorImpl::record_framebuffer_state(trace::Call &call)
{
    auto c = make_shared<TraceCall>(call);
    if (m_current_draw_buffer)
        m_current_draw_buffer->set_state(c);
    return c;
}

#define MAP(name, call) m_call_table.insert(std::make_pair(#name, bind(&TraceMirrorImpl::call, this, _1)))
#define MAP_DATA(name, call, data) m_call_table.insert(std::make_pair(#name, bind(&TraceMirrorImpl::call, this, _1, data)))

#define MAP_GENOBJ(name, obj, call) \
    m_call_table.insert(std::make_pair(#name, bind(&call, &obj, _1)))
#define MAP_GENOBJ_DATA(name, obj, call, data) \
    m_call_table.insert(std::make_pair(#name, bind(&call, &obj, _1, data)))

#define MAP_GENOBJ_DATAREF(name, obj, call, data) \
    m_call_table.insert(std::make_pair(#name, bind(&call, &obj, _1, std::ref(data))))

#define MAP_GENOBJ_DATAREF_2(name, obj, call, data, param1, param2) \
    m_call_table.insert(std::make_pair(#name, bind(&call, &obj, _1, \
                        std::ref(data), param1, param2)))

void TraceMirrorImpl::register_draw_related_calls()
{
    const std::vector<const char *> calls  = {
        "glDrawElements",
        "glDrawRangeElements",
        "glDrawRangeElementsBaseVertex",
    };

    for (auto n: calls) {
        m_call_table.insert(make_pair(n, bind(&TraceMirrorImpl::record_draw_with_buffer,
                                              this, _1)));
    }

    m_call_table.insert(make_pair("glClear", bind(&TraceMirrorImpl::record_clear_call,
                                                  this, _1)));
    m_call_table.insert(make_pair("glViewport", bind(&TraceMirrorImpl::record_viewport_call,
                                                  this, _1)));

    m_call_table.insert(make_pair("glDrawArrays",bind(&TraceMirrorImpl::record_draw,
                                                      this, _1)));
    m_call_table.insert(make_pair("glDrawBuffer",bind(&TraceMirrorImpl::record_draw_buffer,                                                      this, _1)));
}

void TraceMirrorImpl::register_state_calls()
{
    const std::vector<const char *> state_calls_0  = {
        "glAlphaFunc",
        "glBlendColor",
        "glBlendEquation",
        "glBlendFunc",
        "glClearColor",
        "glClearDepth",
        "glClearStencil",
        "glClientWaitSync",
        "glColorMask",
        "glColorPointer",
        "glCullFace",
        "glDepthFunc",
        "glDepthMask",
        "glDepthRange",
        "glFlush",
        "glFenceSync",
        "glFrontFace",
        "glFrustum",
        "glLineStipple",
        "glLineWidth",
        "glPixelZoom",
        "glPointSize",
        "glPolygonMode",
        "glPolygonOffset",
        "glPolygonStipple",
        "glShadeModel",
        "glScissor",
        "glStencilFuncSeparate",
        "glStencilMask",
        "glStencilOpSeparate",
        "glVertexPointer"
    };

    for (auto n: state_calls_0) {
        m_call_table.insert(std::make_pair(n, bind(&TraceMirrorImpl::record_state_call,
                                                   this, _1, 0, TraceCall::single_state)));
    }

    const std::vector<const char *> state_calls_1  = {
        "glClipPlane",
        "glColorMaskIndexedEXT",
        "glColorMaterial",
        "glDisableClientState",
        "glEnableClientState",
        "glLight",
        "glPixelStorei",
        "glPixelTransfer",
    };

    for (auto n: state_calls_1) {
        m_call_table.insert(std::make_pair(n, bind(&TraceMirrorImpl::record_state_call,
                                                   this, _1, 1, TraceCall::single_state)));
    }

    const std::vector<const char *> state_calls_2  = {
        "glMaterial",
        "glTexEnv",
    };

    for (auto n: state_calls_2) {
        m_call_table.insert(std::make_pair(n, bind(&TraceMirrorImpl::record_state_call,
                                                   this, _1, 2, TraceCall::single_state)));
    }

    const std::vector<const char *> state_calls = {
        "glCheckFramebufferStatus",
        "glDeleteVertexArrays",
        "glDetachShader",
        "glGetError",
        "glGetFloat",
        "glGetFramebufferAttachmentParameter",
        "glGetInfoLog",
        "glGetInteger",
        "glGetObjectParameter",
        "glGetProgram",
        "glGetShader",
        "glGetString",
        "glGetTexLevelParameter",
        "glGetTexImage",
        "glIsEnabled",
        "glXGetClientString",
        "glXGetCurrentContext",
        "glXGetCurrentDisplay",
        "glXGetCurrentDrawable",
        "glXGetFBConfigAttrib",
        "glXGetFBConfigs",
        "glXGetProcAddress",
        "glXGetSwapIntervalMESA",
        "glXGetVisualFromFBConfig",
        "glXSwapBuffers",
    };
    for (auto n: state_calls) {
        m_call_table.insert(std::make_pair(n, bind(&TraceMirrorImpl::record_call, this, _1)));
    }

    MAP_DATA(glEnable, record_enable_call, "Enable");
    MAP_DATA(glDisable, record_enable_call, "Enable");
    MAP_DATA(glEnableVertexAttribArray, record_enable_call, "EnableVA");
    MAP_DATA(glDisableVertexAttrbArray, record_enable_call, "EnableVA");
}

void TraceMirrorImpl::register_buffer_calls()
{
    MAP_GENOBJ(glGenBuffers, m_buffers, BufObjectMap::generate);
    MAP_GENOBJ(glDeleteBuffers, m_buffers, BufObjectMap::destroy);
    MAP(glBindBuffer, bind_buffer);

    MAP_GENOBJ(glBufferData, m_buffers, BufObjectMap::data);
    MAP_GENOBJ(glBufferSubData, m_buffers, BufObjectMap::sub_data);
    MAP_GENOBJ(glMapBuffer, m_buffers, BufObjectMap::map);
    MAP_GENOBJ(glMapBufferRange, m_buffers, BufObjectMap::map_range);
    MAP_GENOBJ(glUnmapBuffer, m_buffers, BufObjectMap::unmap);
    MAP_GENOBJ(memcpy, m_buffers, BufObjectMap::memcopy);

    MAP_GENOBJ(glGenVertexArrays, m_va, VertexArrayMap::generate);
    MAP(glBindVertexArray, bind_vertex_array);
    MAP_GENOBJ(glDeleteVertexArray, m_va, VertexArrayMap::destroy);
}

void TraceMirrorImpl::register_texture_calls()
{
    MAP_GENOBJ(glGenTextures, m_textures, TexObjectMap::generate);
    MAP_GENOBJ(glDeleteTextures, m_textures, TexObjectMap::destroy);
    MAP(glBindTexture, bind_texture);

    MAP_GENOBJ(glActiveTexture, m_textures, TexObjectMap::active_texture);
    MAP_GENOBJ(glClientActiveTexture, m_textures, TexObjectMap::active_texture);

    MAP_GENOBJ(glBindMultiTexture, m_textures, TexObjectMap::bind_multitex);

    MAP_GENOBJ(glCompressedTexImage2D, m_textures, TexObjectMap::allocation);
    MAP_GENOBJ_DATA(glGenerateMipmap, m_textures, TexObjectMap::state, 0);
    MAP_GENOBJ(glTexImage1D, m_textures, TexObjectMap::allocation);
    MAP_GENOBJ(glTexImage2D, m_textures, TexObjectMap::allocation);
    MAP_GENOBJ(glTexImage3D, m_textures, TexObjectMap::allocation);
    MAP_GENOBJ_DATA(glTexSubImage1D, m_textures, TexObjectMap::sub_image, nullptr);
    MAP_GENOBJ_DATA(glTexSubImage2D, m_textures, TexObjectMap::sub_image, nullptr);
    MAP_GENOBJ_DATA(glTexSubImage3D, m_textures, TexObjectMap::sub_image, nullptr);
    //MAP_DATA(glCopyTexSubImage2D, call_on_bound_obj, m_textures);
    MAP_GENOBJ_DATA(glTexParameter, m_textures, TexObjectMap::state, 2);

    MAP(glBindSampler, bind_sampler);
    MAP_GENOBJ(glGenSamplers, m_samplers, SamplerObjectMap::generate);
    MAP_GENOBJ(glDeleteSamplers, m_samplers, SamplerObjectMap::destroy);
    MAP_DATA(glSamplerParameter, call_on_named_obj, m_samplers);
}

void TraceMirrorImpl::register_program_calls()
{
    MAP_GENOBJ_DATAREF(glAttachObject, m_programs, ProgramObjectMap::attach_shader, m_shaders);
    MAP_GENOBJ_DATAREF(glAttachShader, m_programs, ProgramObjectMap::attach_shader, m_shaders);
    MAP_GENOBJ(glCreateProgram, m_programs, ProgramObjectMap::create);
    MAP_GENOBJ(glDeleteProgram, m_programs, ProgramObjectMap::destroy);
    MAP(glUseProgram, bind_program);

    MAP_GENOBJ(glBindAttribLocation, m_programs,
               ProgramObjectMap::bind_attr_location);

    MAP_GENOBJ_DATAREF(glVertexAttribPointer, m_vertex_attrib_arrays,
                       VertexAttribArrayMap::pointer, m_buffers);

    MAP_GENOBJ(glLinkProgram, m_programs, ProgramObjectMap::link);
    MAP_GENOBJ(glUniform, m_programs, ProgramObjectMap::uniform);

    MAP_DATA(glBindFragDataLocation, call_on_named_obj, m_programs);
    MAP_DATA(glProgramBinary, call_on_named_obj, m_programs);
    MAP_DATA(glProgramParameter, call_on_named_obj, m_programs);

    MAP_GENOBJ(glCreateShader, m_shaders, ShaderObjectMap::create);
    MAP_GENOBJ(glDeleteShader, m_shaders, ShaderObjectMap::destroy);

    MAP_GENOBJ(glCompileShader, m_shaders, ShaderObjectMap::compile);
    MAP_GENOBJ(glShaderSource, m_shaders, ShaderObjectMap::source);
}

void TraceMirrorImpl::register_framebuffer_calls()
{
    MAP(glBindRenderbuffer, bind_renderbuffer);
    MAP_GENOBJ(glDeleteRenderbuffers, m_renderbuffers, RenderbufferObjectMap::destroy);
    MAP_GENOBJ(glGenRenderbuffer, m_renderbuffers, RenderbufferObjectMap::generate);
    MAP_GENOBJ(glRenderbufferStorage, m_renderbuffers, RenderbufferObjectMap::storage);
    MAP_GENOBJ_DATAREF(glGenFramebuffer, m_fbo, FramebufferObjectMap::generate_with_gs, m_global_state);
    MAP_GENOBJ(glDeleteFramebuffers, m_fbo, FramebufferObjectMap::destroy);
    MAP(glBindFramebuffer, bind_fbo);

    MAP_GENOBJ(glBlitFramebuffer, m_fbo, FramebufferObjectMap::blit);
    MAP_GENOBJ_DATAREF_2(glFramebufferTexture, m_fbo,
                         FramebufferObjectMap::attach_texture, m_textures,
                         2, 3);
    MAP_GENOBJ_DATAREF_2(glFramebufferTexture1D, m_fbo,
                         FramebufferObjectMap::attach_texture, m_textures,
                         3, 4);
    MAP_GENOBJ_DATAREF_2(glFramebufferTexture2D, m_fbo,
                         FramebufferObjectMap::attach_texture, m_textures,
                         3, 4);
    MAP_GENOBJ_DATAREF(glFramebufferTexture3D, m_fbo,
                         FramebufferObjectMap::attach_texture3d, m_textures);

    MAP_GENOBJ_DATAREF(glFramebufferRenderbuffer, m_fbo,
                       FramebufferObjectMap::attach_renderbuffer, m_renderbuffers);

    /* MAP(glReadBuffer, ReadBuffer); */
}

void TraceMirrorImpl::register_glx_state_calls()
{
    auto required_func = bind(&TraceMirrorImpl::record_state_call,
                              this, _1, 0, TraceCall::repeatable_state);
    const std::vector<const char *> required_calls = {
        "glXChooseFBConfig",
        "glXChooseVisual",
        "glXCreateContext",
        "glXDestroyContext",
        "glXMakeCurrent",
        "glXQueryExtensionsString",
        "glXSwapIntervalMESA",
        "glXMakeCurrent",
    };
    for (auto& i : required_calls)
        m_call_table.insert(std::make_pair(i, required_func));
}

void TraceMirrorImpl::register_legacy_calls()
{
/*
    // draw calls
    MAP(glBegin, Begin);
    MAP(glColor2, Vertex);
    MAP(glColor3, Vertex);
    MAP(glColor4, Vertex);
    MAP(glEnd, End);
    MAP(glNormal, Vertex);
    MAP(glRect, Vertex);
    MAP(glTexCoord2, Vertex);
    MAP(glTexCoord3, Vertex);
    MAP(glTexCoord4, Vertex);
    MAP(glVertex3, Vertex);
    MAP(glVertex4, Vertex);
    MAP(glVertex2, Vertex);
    MAP(glVertex3, Vertex);
    MAP(glVertex4, Vertex);

    // display lists
    MAP(glCallList, CallList);
    MAP(glDeleteLists, DeleteLists);
    MAP(glEndList, EndList);
    MAP(glGenLists, GenLists);
    MAP(glNewList, NewList);

    MAP(glPushClientAttr, todo);
    MAP(glPopClientAttr, todo);
    */

    // shader calls
    MAP_GENOBJ(glGenPrograms, m_legacy_programs,
               LegacyProgramObjectMap::generate);
    MAP_GENOBJ(glDeletePrograms, m_legacy_programs,
               LegacyProgramObjectMap::destroy);
    MAP(glBindProgram, bind_legacy_program);
    MAP_GENOBJ(glProgramString, m_legacy_programs,
               LegacyProgramObjectMap::program_string);


    // Matrix manipulation
    MAP_GENOBJ(glLoadIdentity, m_matrix_states, MatrixObjectMap::LoadIdentity);
    MAP_GENOBJ(glLoadMatrix, m_matrix_states, MatrixObjectMap::LoadMatrix);
    MAP_GENOBJ(glMatrixMode, m_matrix_states, MatrixObjectMap::MatrixMode);
    MAP_GENOBJ(glMultMatrix, m_matrix_states, MatrixObjectMap::matrix_op);
    MAP_GENOBJ(glOrtho, m_matrix_states, MatrixObjectMap::matrix_op);
    MAP_GENOBJ(glRotate, m_matrix_states, MatrixObjectMap::matrix_op);
    MAP_GENOBJ(glScale, m_matrix_states, MatrixObjectMap::matrix_op);
    MAP_GENOBJ(glTranslate, m_matrix_states, MatrixObjectMap::matrix_op);
    MAP_GENOBJ(glPopMatrix, m_matrix_states, MatrixObjectMap::PopMatrix);
    MAP_GENOBJ(glPushMatrix, m_matrix_states, MatrixObjectMap::PushMatrix);
}

const char *object_type(BindType type)
{
    switch (type) {
#define CASE(type) case type: return #type
    CASE(bt_buffer);
    CASE(bt_program);
    CASE(bt_legacy_program);
    CASE(bt_sampler);
    CASE(bt_framebuffer);
    CASE(bt_renderbuffer);
    CASE(bt_texture);
    CASE(bt_vertex_array);
    default:
        return "unknown";
    }
}

}
