#include "ftr_state.hpp"
#include "ftr_tracecall.hpp"
#include "ftr_genobject.hpp"
#include "ftr_bufobject.hpp"
#include "ftr_programobj.hpp"
#include "ftr_texobject.hpp"
#include "ftr_framebufferobject.hpp"

#include <algorithm>
#include <functional>
#include <set>
#include <iostream>

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

struct TraceMirrorImpl {
    TraceMirrorImpl();

    void process(trace::Call &call, bool required);

    PTraceCall call_on_bound_obj(trace::Call &call, BoundObjectMap& map);
    PTraceCall call_on_named_obj(trace::Call &call, BoundObjectMap& map);
    PTraceCall record_call(trace::Call &call);
    PTraceCall record_state_call(trace::Call &call, unsigned num_name_params,
                                 TraceCall::Flags calltype);
    PTraceCall record_enable_call(trace::Call &call, const char *basename);

    void resolve_state_calls(TraceCall &call,
                             CallIdSet& callset /* inout */,
                             unsigned last_required_call);
    void resolve_repeatable_state_calls(TraceCall &call,
                                        CallIdSet& callset /* inout */);


    frametrim::CallIdSet resolve();

    void register_state_calls();
    void register_buffer_calls();
    void register_texture_calls();
    void register_program_calls();
    void register_framebuffer_calls();
    void register_glx_state_calls();

    BufObjectMap m_buffers;
    ProgramObjectMap m_programs;
    ShaderObjectMap m_shaders;
    TexObjectMap m_textures;
    FramebufferObjectMap m_fbo;
    RenderbufferObjectMap m_renderbuffers;

    VertexArrayMap m_va;

    StateCallMap m_state_calls;
    std::unordered_map<std::string, std::string> m_state_call_param_map;

    using SamplerObjectMap = GenObjectMap<BoundObject>;
    SamplerObjectMap m_samplers;

    using CallTable = std::multimap<const char *, ftr_callback, frametrim::string_part_less>;
    CallTable m_call_table;

    LightTrace m_trace;

    std::set<std::string> m_unhandled_calls;
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
    if (required)
        c->set_flag(TraceCall::required);
    m_trace.push_back(c);
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

void TraceMirrorImpl::register_state_calls()
{
    const std::vector<const char *> state_calls_0  = {
        "glAlphaFunc",
        "glBindVertexArray",
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
        "glVertexPointer",
        "glViewport",
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
        "glDrawArrays",
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
    MAP_GENOBJ_DATA(glBindBuffer, m_buffers, BufObjectMap::bind, 1);

    MAP_GENOBJ(glBufferData, m_buffers, BufObjectMap::data);
    MAP_DATA(glBufferSubData, call_on_bound_obj, m_buffers);
    MAP_GENOBJ(glMapBuffer, m_buffers, BufObjectMap::map);
    MAP_GENOBJ(glMapBufferRange, m_buffers, BufObjectMap::map_range);
    MAP_GENOBJ(glUnmapBuffer, m_buffers, BufObjectMap::unmap);
    MAP_GENOBJ(memcpy, m_buffers, BufObjectMap::memcopy);

    MAP_GENOBJ(glGenVertexArrays, m_va, VertexArrayMap::generate);
    MAP_GENOBJ_DATA(glBindVertexArray, m_va, VertexArrayMap::bind, 0);
    MAP_GENOBJ(glDeleteVertexArraym, m_va, VertexArrayMap::destroy);
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

    MAP_GENOBJ_DATA(glBindSampler, m_samplers, SamplerObjectMap::bind, 0);
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
    MAP_GENOBJ_DATA(glUseProgram, m_programs, ProgramObjectMap::bind, 0);

    MAP_GENOBJ(glBindAttribLocation, m_programs,
               ProgramObjectMap::bind_attr_location);

    MAP_DATA(glLinkProgram, call_on_named_obj, m_programs);
    MAP_DATA(glUniform, call_on_named_obj, m_programs);
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
    MAP_GENOBJ_DATA(glBindRenderbuffer, m_renderbuffers, RenderbufferObjectMap::bind, 0);
    MAP_GENOBJ(glDeleteRenderbuffers, m_renderbuffers, RenderbufferObjectMap::destroy);
    MAP_GENOBJ(glGenRenderbuffer, m_renderbuffers, RenderbufferObjectMap::generate);
    MAP_GENOBJ(glRenderbufferStorage, m_renderbuffers, RenderbufferObjectMap::storage);

    MAP_GENOBJ(glGenFramebuffer, m_fbo, FramebufferObjectMap::generate);
    MAP_GENOBJ(glDeleteFramebuffers, m_fbo, FramebufferObjectMap::destroy);
    MAP_GENOBJ(glBindFramebuffer, m_fbo, FramebufferObjectMap::bind);

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
        "glXSwapBuffers",
        "glXSwapIntervalMESA",
        "glXMakeCurrent",
    };
    for (auto& i : required_calls)
        m_call_table.insert(std::make_pair(i, required_func));
}

}
