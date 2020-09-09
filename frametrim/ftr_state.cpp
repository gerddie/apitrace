#include "ftr_state.hpp"
#include "ftr_tracecall.hpp"
#include "ftr_genobject.hpp"
#include "ftr_bufobject.hpp"
#include "ftr_programobj.hpp"
#include "ftr_texobject.hpp"
#include "ftr_framebufferobject.hpp"
#include "ftr_matrixobject.hpp"

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

struct StateCallRecord {
    unsigned last_before_callid;
    StateCallRecord() :
        last_before_callid(std::numeric_limits<unsigned>::min()){
    }
};

using StateCallMap = std::unordered_map<std::string, StateCallRecord>;

enum BindType {
    bt_buffer,
    bt_framebuffer,
    bt_program,
    bt_renderbuffer,
    bt_sampler,
    bt_texture,
    bt_vertex_array,

};

struct BindTimePoint {
    PGenObject obj;
    unsigned bind_call_no;
    unsigned unbind_call_no;

    BindTimePoint(PGenObject o, unsigned callno):
        obj(o), bind_call_no(callno),
        unbind_call_no(std::numeric_limits<unsigned>::max()) {}
};

using BindTimeline = std::list<BindTimePoint>;


struct TraceMirrorImpl {
    TraceMirrorImpl();

    void process(trace::Call &call, bool required);

    PTraceCall call_on_bound_obj(trace::Call &call, BoundObjectMap& map);
    PTraceCall call_on_named_obj(trace::Call &call, BoundObjectMap& map);
    PTraceCall record_call(trace::Call &call);
    PTraceCall record_state_call(trace::Call &call, unsigned num_name_params,
                                 TraceCall::Flags calltype);
    PTraceCall record_enable_call(trace::Call &call, const char *basename);

    /* These calls need to be redirected to the currently bound draw framebuffer */
    PTraceCall record_draw(trace::Call &call);
    PTraceCall record_viewport_call(trace::Call &call);
    PTraceCall record_clear_call(trace::Call &call);
    PTraceCall record_framebuffer_state(trace::Call &call);

    void resolve_state_calls(TraceCall &call,
                             CallIdSet& callset /* inout */,
                             unsigned last_required_call);
    void resolve_repeatable_state_calls(TraceCall &call,
                                        CallIdSet& callset /* inout */);

    PTraceCall bind_fbo(trace::Call &call);
    PTraceCall bind_texture(trace::Call &call);
    PTraceCall bind_buffer(trace::Call &call);
    PTraceCall bind_program(trace::Call &call);
    PTraceCall bind_renderbuffer(trace::Call &call);
    PTraceCall bind_sampler(trace::Call &call);
    PTraceCall bind_vertex_array(trace::Call &call);

    frametrim::CallIdSet resolve();

    void register_state_calls();
    void register_buffer_calls();
    void register_texture_calls();
    void register_program_calls();
    void register_framebuffer_calls();
    void register_glx_state_calls();
    void register_legacy_calls();

    void register_draw_related_calls();

    void record_bind(BindType type, PGenObject obj,
                     GLenum id, unsigned tex_unit, unsigned callno);

    unsigned buffer_offset(BindType type, GLenum id, unsigned active_unit);
    PTraceCall bind_tracecall(trace::Call &call, PGenObject obj);
    void collect_bound_objects(ObjectSet& required_objects, unsigned before_call);

    BufObjectMap m_buffers;
    ProgramObjectMap m_programs;
    ShaderObjectMap m_shaders;
    TexObjectMap m_textures;
    FramebufferObjectMap m_fbo;
    RenderbufferObjectMap m_renderbuffers;
    MatrixObjectMap m_matrix_states;

    PFramebufferObject m_current_draw_buffer;
    PFramebufferObject m_current_read_buffer;

    VertexArrayMap m_va;

    StateCallMap m_state_calls;
    std::unordered_map<std::string, std::string> m_state_call_param_map;

    using SamplerObjectMap = GenObjectMap<BoundObject>;
    SamplerObjectMap m_samplers;

    using CallTable = std::multimap<const char *, ftr_callback, frametrim::string_part_less>;
    CallTable m_call_table;

    LightTrace m_trace;

    std::set<std::string> m_unhandled_calls;

    std::unordered_map<unsigned, BindTimeline> m_bind_timelines;
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
    CallIdSet calls = impl->resolve();

    std::vector<unsigned> retval(calls.begin(), calls.end());
    std::sort(retval.begin(), retval.end());

    return retval;
}

TraceMirrorImpl::TraceMirrorImpl()
{
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
        m_trace.push_back(c);
    }
}

PTraceCall
TraceMirrorImpl::bind_fbo(trace::Call &call)
{
    auto retval = m_fbo.bind(call);

    unsigned target = call.arg(0).toUInt();

    PGenObject fbo;

    if (target == GL_FRAMEBUFFER) {
        record_bind(bt_framebuffer, m_fbo.draw_buffer(), GL_DRAW_FRAMEBUFFER, 0, call.no);
        record_bind(bt_framebuffer, m_fbo.read_buffer(), GL_READ_FRAMEBUFFER, 0, call.no);
        fbo = m_current_draw_buffer = m_fbo.draw_buffer();

    } else if (target == GL_DRAW_FRAMEBUFFER) {
        record_bind(bt_framebuffer, m_fbo.draw_buffer(), GL_DRAW_FRAMEBUFFER, 0, call.no);
        fbo = m_current_draw_buffer = m_fbo.draw_buffer();
    } else {
        assert(target == GL_READ_FRAMEBUFFER);
        record_bind(bt_framebuffer, m_fbo.read_buffer(), GL_READ_FRAMEBUFFER, 0, call.no);
        return make_shared<TraceCallOnBoundObj>(call, m_fbo.read_buffer());
        fbo = m_current_read_buffer = m_fbo.read_buffer();
    }

    return bind_tracecall(call, fbo);
}

PTraceCall TraceMirrorImpl::bind_texture(trace::Call &call)
{
    auto texture = m_textures.bind(call, 1);
    auto target = call.arg(0).toUInt();

    record_bind(bt_texture, texture, target, m_textures.active_unit(), call.no);
    return bind_tracecall(call, texture);
}

PTraceCall TraceMirrorImpl::bind_buffer(trace::Call &call)
{
    auto buffer = m_buffers.bind(call, 1);
    auto target = call.arg(0).toUInt();
    record_bind(bt_buffer, buffer, target, 0, call.no);
    return bind_tracecall(call, buffer);
}

PTraceCall TraceMirrorImpl::bind_program(trace::Call &call)
{
    auto prog = m_programs.bind(call, 0);
    record_bind(bt_program, prog, 0, 0, call.no);
    return bind_tracecall(call, prog);
}

PTraceCall TraceMirrorImpl::bind_renderbuffer(trace::Call &call)
{
    auto rb = m_programs.bind(call, 1);
    record_bind(bt_renderbuffer, rb, 0, 0, call.no);
    return bind_tracecall(call, rb);
}

PTraceCall TraceMirrorImpl::bind_sampler(trace::Call &call)
{
    auto sampler = m_samplers.bind(call, 1);
    auto target = call.arg(0).toUInt();
    record_bind(bt_sampler, sampler, target, 0, call.no);
    return bind_tracecall(call, sampler);
}

PTraceCall TraceMirrorImpl::bind_vertex_array(trace::Call &call)
{
    auto va = m_va.bind(call, 0);
    record_bind(bt_vertex_array, va, 0, 0, call.no);
    return bind_tracecall(call, va);
}

PTraceCall TraceMirrorImpl::bind_tracecall(trace::Call &call, PGenObject obj)
{
    return obj ? make_shared<TraceCallOnBoundObj>(call, obj):
                 make_shared<TraceCall>(call);
}

PTraceCall
TraceMirrorImpl::call_on_bound_obj(trace::Call &call, BoundObjectMap& map)
{
    auto bound_obj = map.bound_to_call_target_untyped(call);
    return PTraceCall(new TraceCallOnBoundObj(call, bound_obj));
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

CallIdSet
TraceMirrorImpl::resolve()
{
    ObjectSet required_objects;
    CallIdSet required_calls;
    unsigned next_required_call = std::numeric_limits<unsigned>::max();

    /* record all frames from the target frame set */
    for(auto& i : m_trace) {
        if (i->test_flag(TraceCall::required)) {
            i->add_object_to_set(required_objects);
            required_calls.insert(i->call_no());
            if (i->call_no() < next_required_call)
                next_required_call = i->call_no();
        }
    }

    /* Now collect all calls from the beginning that refere to states that
     * can be changed repeatandly (glx and egl stuff) and where we must use
     * the eraliest calls possible */
    for (auto&& c : m_trace) {
        if (c->call_no() >= next_required_call)
            break;
        if (c->test_flag(TraceCall::repeatable_state))
            resolve_repeatable_state_calls(*c, required_calls);
    }

    collect_bound_objects(required_objects, next_required_call);

    while (!required_objects.empty()) {
        auto obj = required_objects.front();
        required_objects.pop();
        obj->collect_objects(required_objects);
        obj->collect_calls(required_calls, next_required_call);
    }

    /* At this point only state calls should remain to be recorded
     * So go in reverse to the list and add them. */
    next_required_call = std::numeric_limits<unsigned>::max();
    for (auto c = m_trace.rbegin(); c != m_trace.rend(); ++c) {
        /*  required calls are already in the output callset */
        if ((*c)->test_flag(TraceCall::required)) {
            next_required_call = (*c)->call_no();
            continue;
        }

        if ((*c)->test_flag(TraceCall::single_state))
            resolve_state_calls(**c, required_calls, next_required_call);
    }
    return required_calls;
}

void
TraceMirrorImpl::collect_bound_objects(ObjectSet& required_objects, unsigned before_call)
{
    for(auto&& timeline : m_bind_timelines) {
        for (auto&& timepoint: timeline.second) {
            if (timepoint.bind_call_no < before_call &&
                timepoint.unbind_call_no >= before_call) {
                required_objects.push(timepoint.obj);
                std::cerr << "Add a bound object with ID " << timepoint.obj->id() << "\n";
            }
        }
    }
}

PTraceCall TraceMirrorImpl::record_draw(trace::Call &call)
{
    auto c = make_shared<TraceCall>(call);
    if (m_current_draw_buffer) {
        m_current_draw_buffer->draw(c);
        return nullptr;
    } else
        return c;
}

PTraceCall TraceMirrorImpl::record_viewport_call(trace::Call &call)
{
    if (m_current_draw_buffer) {
        m_current_draw_buffer->viewport(call);
        return nullptr;
    } else {
        auto c = make_shared<TraceCall>(call);
        c->set_flag(TraceCall::single_state);
        return c;
    }
}

PTraceCall TraceMirrorImpl::record_clear_call(trace::Call &call)
{
    if (m_current_draw_buffer) {
        m_current_draw_buffer->clear(call);
        return nullptr;
    } else
        return make_shared<TraceCall>(call);;
}

PTraceCall TraceMirrorImpl::record_framebuffer_state(trace::Call &call)
{
    auto c = make_shared<TraceCall>(call);
    if (m_current_draw_buffer) {
        m_current_draw_buffer->set_state(c);
        return nullptr;
    } else
        return c;
}

void
TraceMirrorImpl::resolve_state_calls(TraceCall& call,
                                     CallIdSet& callset /* inout */,
                                     unsigned next_required_call)
{
    /* Add the call if the last required call happended before
     * the passed function was called the last time.
     * The _last_before_callid_ value is initialized to the
     * maximum when the object is created. */
    auto& last_call = m_state_calls[call.name()];
    if (call.call_no() < next_required_call &&
        last_call.last_before_callid < call.call_no()) {
        std::cerr << "Add state call " << call.call_no() << ":"
                  << call.name() << " last was " << last_call.last_before_callid << "\n";
        last_call.last_before_callid = call.call_no();
        callset.insert(call.call_no());
        call.set_flag(TraceCall::required);
    }
}

void TraceMirrorImpl::resolve_repeatable_state_calls(TraceCall &call,
                                                     CallIdSet& callset /* inout */)
{
    auto& last_state_param_set = m_state_call_param_map[call.name()];

    if (last_state_param_set != call.name_with_params()) {
        m_state_call_param_map[call.name()] = call.name_with_params();

        callset.insert(call.call_no());
        call.set_flag(TraceCall::required);
    }
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
        "glDrawArrays",
    };

    for (auto n: calls) {
        m_call_table.insert(make_pair(n, bind(&TraceMirrorImpl::record_draw,
                                              this, _1)));
    }

    m_call_table.insert(make_pair("glClear", bind(&TraceMirrorImpl::record_clear_call,
                                                  this, _1)));
    m_call_table.insert(make_pair("glViewport", bind(&TraceMirrorImpl::record_viewport_call,
                                                  this, _1)));

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
    MAP_DATA(glEnableVertexAttrinArray, record_enable_call, "EnableVA");
    MAP_DATA(glDisableVertexAttrinArray, record_enable_call, "EnableVA");
}

void TraceMirrorImpl::register_buffer_calls()
{
    MAP_GENOBJ(glGenBuffers, m_buffers, BufObjectMap::generate);
    MAP_GENOBJ(glDeleteBuffers, m_buffers, BufObjectMap::destroy);
    MAP(glBindBuffer, bind_buffer);

    MAP_GENOBJ(glBufferData, m_buffers, BufObjectMap::data);
    MAP_DATA(glBufferSubData, call_on_bound_obj, m_buffers);
    MAP_GENOBJ(glMapBuffer, m_buffers, BufObjectMap::map);
    MAP_GENOBJ(glMapBufferRange, m_buffers, BufObjectMap::map_range);
    MAP_GENOBJ(glUnmapBuffer, m_buffers, BufObjectMap::unmap);
    MAP_GENOBJ(memcpy, m_buffers, BufObjectMap::memcopy);

    MAP_GENOBJ(glGenVertexArrays, m_va, VertexArrayMap::generate);
    MAP(glBindVertexArray, bind_vertex_array);
    MAP_GENOBJ(glDeleteVertexArraym, m_va, VertexArrayMap::destroy);
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
    MAP_DATA(glTexSubImage1D, call_on_bound_obj, m_textures);
    MAP_DATA(glTexSubImage2D, call_on_bound_obj, m_textures);
    MAP_DATA(glTexSubImage3D, call_on_bound_obj, m_textures);
    MAP_DATA(glCopyTexSubImage2D, call_on_bound_obj, m_textures);
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

    MAP_GENOBJ_DATAREF(glVertexAttribPointer, m_programs,
                       ProgramObjectMap::vertex_attr_pointer, m_buffers);

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

    MAP_GENOBJ(glGenFramebuffer, m_fbo, FramebufferObjectMap::generate);
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

    /*MAP(glReadBuffer, ReadBuffer);
    MAP(glDrawBuffers, DrawBuffers);*/
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


    // shader calls
    MAP_GENOBJ(glGenPrograms, m_legacy_programs,
    MatrixObjectMap();
               LegacyProgramStateMap::generate);
    MAP_GENOBJ(glDeletePrograms, m_legacy_programs,
               LegacyProgramStateMap::destroy);
    MAP_GENOBJ(glBindProgram, m_legacy_programs,
               LegacyProgramStateMap::bind);
    MAP_GENOBJ(glProgramString, m_legacy_programs,
               LegacyProgramStateMap::program_string);

    */
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
    CASE(bt_sampler);
    CASE(bt_framebuffer);
    CASE(bt_renderbuffer);
    CASE(bt_texture);
    CASE(bt_vertex_array);
    default:
        return "unknown";
    }
}

void TraceMirrorImpl::record_bind(BindType type, PGenObject obj,
                                  GLenum id, unsigned tex_unit, unsigned callno)
{
    unsigned index = buffer_offset(type, id, tex_unit);

    auto& recoed = m_bind_timelines[index];
    if (!recoed.empty()) {
        auto& last = recoed.front();
        last.unbind_call_no = callno;
    }
    if (obj) {
        std::cerr << callno << ": record binding of object "
                  << object_type(type) << "@" << id
                  << "-" << tex_unit
                  << " with ID " << obj->id() << "\n";
        BindTimePoint new_time_point(obj, callno);
        recoed.push_front(new_time_point);
    }
}

unsigned
TraceMirrorImpl::buffer_offset(BindType type, GLenum target, unsigned active_unit)
{
    switch (type) {
    case bt_buffer:
        switch (target) {
        case GL_ARRAY_BUFFER: return 1;
        case GL_ATOMIC_COUNTER_BUFFER: 	return 2;
        case GL_COPY_READ_BUFFER: return 3;
        case GL_COPY_WRITE_BUFFER: return 4;
        case GL_DISPATCH_INDIRECT_BUFFER: return 5;
        case GL_DRAW_INDIRECT_BUFFER: return 6;
        case GL_ELEMENT_ARRAY_BUFFER: return 7;
        case GL_PIXEL_PACK_BUFFER: return 8;
        case GL_PIXEL_UNPACK_BUFFER: return 9;
        case GL_QUERY_BUFFER: return 10;
        case GL_SHADER_STORAGE_BUFFER: return 11;
        case GL_TEXTURE_BUFFER: return 12;
        case GL_TRANSFORM_FEEDBACK_BUFFER: return 13;
        case GL_UNIFORM_BUFFER: return 14;
        default:
            assert(0 && "unknown buffer bind point");
        }
    case bt_program:
        return 15;
    case bt_renderbuffer:
        return 16;
    case bt_framebuffer:
        switch (target) {
        case GL_READ_FRAMEBUFFER: return 17;
        case GL_DRAW_FRAMEBUFFER: return 18;
        default:
            /* Since GL_FRAMEBUFFER may set both buffers we must handle this elsewhere */
            assert(0 && "GL_FRAMEBUFFER must be handled before calling buffer_offset");
        }
    case bt_texture: {
        unsigned base = active_unit * 11  + 19;
        switch (target) {
        case GL_TEXTURE_1D: return base;
        case GL_TEXTURE_2D: return base + 1;
        case GL_TEXTURE_3D: return base + 2;
        case GL_TEXTURE_1D_ARRAY: return base + 3;
        case GL_TEXTURE_2D_ARRAY: return base + 4;
        case GL_TEXTURE_RECTANGLE: return base + 5;
        case GL_TEXTURE_CUBE_MAP: return base + 6;
        case GL_TEXTURE_CUBE_MAP_ARRAY: return base + 7;
        case GL_TEXTURE_BUFFER: return base + 8;
        case GL_TEXTURE_2D_MULTISAMPLE: return base + 9;
        case GL_TEXTURE_2D_MULTISAMPLE_ARRAY: return base + 10;
        default:
            assert(0 && "unknown texture bind point");
        }
    }
    case bt_sampler:
        return 800 + target;
    case bt_vertex_array:
        return 900;
    }
    assert(0 && "Unsupported bind point");
}


}
