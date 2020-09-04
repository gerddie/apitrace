#include "ftr_state.hpp"
#include "ftr_tracecall.hpp"
#include "ftr_genobject.hpp"
#include "ftr_bufobject.hpp"
#include "ftr_programobj.hpp"
#include "ftr_texobject.hpp"

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
        last_before_callid(std::numeric_limits<unsigned>::max()){
    }
};

using StateCallMap = std::unordered_map<std::string, StateCallRecord>;

struct TraceMirrorImpl {
    TraceMirrorImpl();

    void process(trace::Call &call, bool required);

    PTraceCall call_on_bound_obj(trace::Call &call, BoundObjectMap& map);
    PTraceCall call_on_named_obj(trace::Call &call, BoundObjectMap& map);
    PTraceCall record_call(trace::Call &call);
    PTraceCall record_state_call(trace::Call &call, unsigned num_name_params);

    void resolve_state_calls(TraceCall &call,
                           CallIdSet& callset /* inout */,
                           unsigned last_required_call);

    void resolve();

    void register_state_calls();
    void register_buffer_calls();
    void register_texture_calls();
    void register_program_calls();
    void register_framebuffer_calls();

    BufObjectMap m_buffers;
    ProgramObjectMap m_programs;
    ShaderObjectMap m_shaders;
    TexObjectMap m_textures;

    StateCallMap m_state_calls;

    using SamplerObjectMap = GenObjectMap<GenObject>;
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

LightTrace
TraceMirror::trace() const
{
    return impl->m_trace;
}

TraceMirrorImpl::TraceMirrorImpl()
{
    register_state_calls();
    register_buffer_calls();
    register_texture_calls();
    register_program_calls();
    register_framebuffer_calls();
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
        c->set_required();
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
    return PTraceCall(new TraceCallOnBoundObj(call, bound_obj));
}

PTraceCall
TraceMirrorImpl::record_call(trace::Call &call)
{
    return make_shared<TraceCall>(call);
}

PTraceCall
TraceMirrorImpl::record_state_call(trace::Call &call, unsigned num_name_params)
{
    return make_shared<StateCall>(call, num_name_params);
}

void
TraceMirrorImpl::resolve()
{
    ObjectSet required_objects;
    CallIdSet required_calls;
    for(auto& i : m_trace) {
        if (i->required()) {
            i->add_object_to_set(required_objects);
            required_calls.insert(i->call_no());
        }
    }

    while (!required_objects.empty()) {
        auto obj = required_objects.front();
        required_objects.pop();
        obj->collect_objects(required_objects);
    }

    /* At this point only state calls should remain to be recorded
     * So go in reverse to the list and add them. */
    unsigned next_required_call = std::numeric_limits<unsigned>::max();
    for (auto c = m_trace.rbegin(); c != m_trace.rend(); ++c) {
        /*  required calls are already in the output callset */
        if ((*c)->required()) {
            next_required_call = (*c)->call_no();
            continue;
        }

        resolve_state_calls(**c, required_calls, next_required_call);
    }
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
    auto last_call = m_state_calls[call.name()];
    if (last_call.last_before_callid > next_required_call) {
        last_call.last_before_callid = next_required_call;
        callset.insert(call.call_no());
        call.set_required();
    }
}

#define MAP(name, call) m_call_table.insert(std::make_pair(#name, bind(&TraceMirrorImpl::call, this, _1)))
#define MAP_DATA(name, call, data) m_call_table.insert(std::make_pair(#name, bind(&TraceMirrorImpl::call, this, _1, data)))

#define MAP_GENOBJ(name, obj, call) \
    m_call_table.insert(std::make_pair(#name, bind(&call, &obj, _1)))
#define MAP_GENOBJ_DATA(name, obj, call, data) \
    m_call_table.insert(std::make_pair(#name, bind(&call, &obj, _1, data)))

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
    };

    for (auto n: state_calls_0) {
        m_call_table.insert(std::make_pair(n, bind(&TraceMirrorImpl::record_state_call, this, _1, 0)));
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
        m_call_table.insert(std::make_pair(n, bind(&TraceMirrorImpl::record_state_call, this, _1, 1)));
    }

    const std::vector<const char *> state_calls_2  = {
        "glMaterial",
        "glTexEnv",
    };

    for (auto n: state_calls_1) {
        m_call_table.insert(std::make_pair(n, bind(&TraceMirrorImpl::record_state_call, this, _1, 2)));
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
        "glXQueryVersion",
        "glXSwapBuffers",
        "glXChooseVisual",
        "glXCreateContext",
        "glXDestroyContext",
        "glXMakeCurrent",
        "glXChooseFBConfig",
        "glXQueryExtensionsString",
        "glXSwapIntervalMESA",
    };
    for (auto n: state_calls) {
        m_call_table.insert(std::make_pair(n, bind(&TraceMirrorImpl::record_call, this, _1)));
    }
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
    MAP_GENOBJ_DATA(glAttachObject, m_programs, ProgramObjectMap::attach_shader, m_shaders);
    MAP_GENOBJ_DATA(glAttachShader, m_programs, ProgramObjectMap::attach_shader, m_shaders);
    MAP_GENOBJ(glCreateProgram, m_programs, ProgramObjectMap::create);
    MAP_GENOBJ(glDeleteProgram, m_programs, ProgramObjectMap::destroy);
    MAP_GENOBJ_DATA(glUseProgram, m_programs, ProgramObjectMap::bind, 0);

    MAP_GENOBJ(glBindAttribLocation, m_programs, ProgramObjectMap::bind_attr_location);

    MAP_DATA(glLinkProgram, call_on_named_obj, m_programs);
    MAP_DATA(glUniform, call_on_named_obj, m_programs);
    MAP_DATA(glBindFragDataLocation, call_on_named_obj, m_programs);
    MAP_DATA(glProgramBinary, call_on_named_obj, m_programs);
    MAP_DATA(glProgramParameter, call_on_named_obj, m_programs);

    MAP_GENOBJ(glCreateShader, m_shaders, ShaderObjectMap::create);
    MAP_GENOBJ(glDeleteShader, m_shaders, ShaderObjectMap::destroy);

    MAP_DATA(glCompileShader, call_on_named_obj, m_shaders);
    MAP_DATA(glShaderSource, call_on_named_obj, m_shaders);
}

void TraceMirrorImpl::register_framebuffer_calls()
{

}

}
