#include "ft_frametrimmer.hpp"
#include "ft_tracecall.hpp"
#include "ft_framebuffer.hpp"
#include "ft_matrixstate.hpp"
#include "ft_objectstate.hpp"
#include "ft_programstate.hpp"
#include "ft_texturestate.hpp"
#include "ft_bufferstate.hpp"
#include "ft_vertexarray.hpp"
#include "ft_displaylists.hpp"
#include "ft_renderbuffer.hpp"
#include "ft_samplerstate.hpp"

#include <unordered_set>
#include <algorithm>
#include <functional>
#include <sstream>
#include <iostream>
#include <set>

#include <GL/gl.h>
#include <GL/glext.h>

namespace frametrim {

using std::bind;
using std::placeholders::_1;
using std::make_shared;


using ft_callback = std::function<PTraceCall(const trace::Call&)>;

struct string_part_less {
    bool operator () (const char *lhs, const char *rhs)
    {
        int len = std::min(strlen(lhs), strlen(rhs));
        return strncmp(lhs, rhs, len) < 0;
    }
};

struct FrameTrimmeImpl {

    enum PointerType {
        pt_vertex,
        pt_texcoord,
        pt_color,
        pt_normal,
        pt_va,
        pt_last
    };

    FrameTrimmeImpl();
    void call(const trace::Call& call, bool in_target_frame);
    void start_target_frame();
    void end_target_frame();

    std::vector<unsigned> get_sorted_call_ids() const;

    static unsigned equal_chars(const char *l, const char *r);

    PTraceCall record_state_call(const trace::Call& call, unsigned no_param_sel);

    void register_state_calls();
    void register_legacy_calls();
    void register_required_calls();
    void register_framebuffer_calls();
    void register_buffer_calls();
    void register_program_calls();
    void register_va_calls();
    void register_texture_calls();
    void register_draw_calls();
    void register_ignore_history_calls();

    PTraceCall record_enable(const trace::Call& call);
    PTraceCall record_va_enable(const trace::Call& call, bool enable, PointerType type);
    PTraceCall record_required_call(const trace::Call& call);
    PTraceCall record_client_state_enable(const trace::Call& call,
                                          bool enable);

    void update_call_table(const std::vector<const char*>& names,
                           ft_callback cb);

    PTraceCall Begin(const trace::Call& call);
    PTraceCall End(const trace::Call& call);
    PTraceCall Vertex(const trace::Call& call);
    PTraceCall EndList(const trace::Call& call);
    PTraceCall GenLists(const trace::Call& call);
    PTraceCall NewList(const trace::Call& call);
    PTraceCall CallList(const trace::Call& call);
    PTraceCall DeleteLists(const trace::Call& call);
    PTraceCall AttribPointer(const trace::Call& call, PointerType type);
    PTraceCall DrawElements(const trace::Call& call);
    PTraceCall DrawArrays(const trace::Call& call);

    PTraceCall todo(const trace::Call& call);
    PTraceCall ignore_history(const trace::Call& call);

    void finalize();

    FramebufferState::Pointer m_current_draw_buffer;

    using CallTable = std::multimap<const char *, ft_callback, string_part_less>;
    CallTable m_call_table;

    CallSet m_required_calls;
    std::set<std::string> m_unhandled_calls;

    std::unordered_map<unsigned, DisplayLists::Pointer> m_display_lists;
    DisplayLists::Pointer m_active_display_list;

    std::map<std::string, PTraceCall> m_state_calls;
    std::map<unsigned, PTraceCall> m_enables;

    AllMatrisStates m_matrix_states;
    LegacyProgramStateMap m_legacy_programs;
    ProgramStateMap m_programs;
    TextureStateMap m_textures;
    FramebufferStateMap m_fbo;
    BufferStateMap m_buffers;
    ShaderStateMap m_shaders;
    RenderbufferMap m_renderbuffers;
    SamplerStateMap m_samplers;

    std::unordered_map<GLint, PTraceCall> m_va_enables;
    std::unordered_map<GLint, bool> m_va_is_enabled;
    TGenObjStateMap<VertexArray> m_vertex_arrays;
    std::unordered_map<GLint, PBufferState> m_vertex_attr_pointer;

    bool m_recording_frame;


};

FrameTrimmer::FrameTrimmer()
{
    impl = new FrameTrimmeImpl;
}

FrameTrimmer::~FrameTrimmer()
{
    delete impl;
}

void
FrameTrimmer::call(const trace::Call& call, bool in_target_frame)
{
    impl->call(call, in_target_frame);
}

std::vector<unsigned>
FrameTrimmer::get_sorted_call_ids() const
{
    return impl->get_sorted_call_ids();
}

void FrameTrimmer::finalize()
{
    impl->finalize();
}

FrameTrimmeImpl::FrameTrimmeImpl():
    m_recording_frame(false)
{
    register_state_calls();
    register_legacy_calls();
    register_required_calls();
    register_framebuffer_calls();
    register_buffer_calls();
    register_program_calls();
    register_va_calls();
    register_texture_calls();
    register_draw_calls();
    register_ignore_history_calls();
}

void
FrameTrimmeImpl::call(const trace::Call& call, bool in_target_frame)
{
    const char *call_name = call.name();

    PTraceCall c;

    if (!m_recording_frame && in_target_frame)
        start_target_frame();
    if (m_recording_frame && !in_target_frame)
        end_target_frame();

    auto cb_range = m_call_table.equal_range(call.name());
    if (cb_range.first != m_call_table.end() &&
            std::distance(cb_range.first, cb_range.second) > 0) {

        CallTable::const_iterator cb = cb_range.first;
        CallTable::const_iterator i = cb_range.first;
        ++i;

        unsigned max_equal = equal_chars(cb->first, call_name);

        while (i != cb_range.second && i != m_call_table.end()) {
            auto n = equal_chars(i->first, call_name);
            if (n > max_equal) {
                max_equal = n;
                cb = i;
            }
            ++i;
        }

        //std::cerr << "Handle " << call->name() << " as " << cb->first << "\n";
        c = cb->second(call);
    } else {
        /* This should be some debug output only, because we might
         * not handle some calls deliberately */
        if (m_unhandled_calls.find(call_name) == m_unhandled_calls.end()) {
            std::cerr << "Call " << call.no
                      << " " << call_name << " not handled\n";
            m_unhandled_calls.insert(call_name);
        }
    }

    if (in_target_frame) {
        if (!c)
            c = trace2call(call);
        m_required_calls.insert(c);
    }
}

void FrameTrimmeImpl::start_target_frame()
{
    std::cerr << "Start recording\n";

    m_recording_frame = true;

    for(auto& s : m_state_calls)
        m_required_calls.insert(s.second);

    for(auto& s : m_enables)
        m_required_calls.insert(s.second);

    m_matrix_states.emit_state_to_lists(m_required_calls);

    m_fbo.current_framebuffer().emit_calls_to_list(m_required_calls);

    m_textures.emit_calls_to_list(m_required_calls);

    m_programs.emit_calls_to_list(m_required_calls);

    m_vertex_arrays.emit_calls_to_list(m_required_calls);

    for (auto&& va: m_va_is_enabled) {
        if (!va.second)
            continue;
        auto buf = m_vertex_attr_pointer[va.first];
        if (buf)
            buf->emit_calls_to_list(m_required_calls);
    }
    for (auto&& va: m_va_enables)
        m_required_calls.insert(va.second);
}

void FrameTrimmeImpl::finalize()
{
    m_fbo.emit_calls_to_list(m_required_calls);
}

void FrameTrimmeImpl::end_target_frame()
{
    m_fbo.emit_calls_to_list(m_required_calls);
    std::cerr << "End recording\n";
    m_recording_frame = false;
}

std::vector<unsigned>
FrameTrimmeImpl::get_sorted_call_ids() const
{
    std::unordered_set<unsigned> make_sure_its_singular;

    for(auto&& c: m_required_calls)
        make_sure_its_singular.insert(c->call_no());

    std::vector<unsigned> sorted_calls(
                make_sure_its_singular.begin(),
                make_sure_its_singular.end());
    std::sort(sorted_calls.begin(), sorted_calls.end());
    return sorted_calls;
}

unsigned
FrameTrimmeImpl::equal_chars(const char *l, const char *r)
{
    unsigned retval = 0;
    while (*l && *r && *l == *r) {
        ++retval;
        ++l; ++r;
    }
    if (!*l && !*r)
        ++retval;

    return retval;
}

PTraceCall
FrameTrimmeImpl::record_state_call(const trace::Call& call,
                                   unsigned no_param_sel)
{
    std::stringstream s;
    s << call.name();
    for (unsigned i = 0; i < no_param_sel; ++i)
        s << "_" << call.arg(i).toUInt();

    auto c = std::make_shared<TraceCall>(call, s.str());
    m_state_calls[s.str()] = c;

    if (m_active_display_list)
        m_active_display_list->append_call(c);

    return c;
}

PTraceCall
FrameTrimmeImpl::record_enable(const trace::Call& call)
{
    unsigned value = call.arg(0).toUInt();
    m_enables[value] = trace2call(call);
    return m_enables[value];
}


#define MAP(name, call) m_call_table.insert(std::make_pair(#name, bind(&FrameTrimmeImpl:: call, this, _1)))
#define MAP_DATA(name, call, data) m_call_table.insert(std::make_pair(#name, bind(&FrameTrimmeImpl:: call, this, _1, data)))
#define MAP_DATA2(name, call, data, data2) \
    m_call_table.insert(std::make_pair(#name, bind(&FrameTrimmeImpl:: call, this, _1, data, data2)))
#define MAP_GENOBJ(name, obj, call) \
    m_call_table.insert(std::make_pair(#name, bind(&call, &obj, _1)))
#define MAP_GENOBJ_DATA(name, obj, call, data) \
    m_call_table.insert(std::make_pair(#name, bind(&call, &obj, _1, data)))
#define MAP_GENOBJ_DATAREF(name, obj, call, data) \
    m_call_table.insert(std::make_pair(#name, bind(&call, &obj, _1, std::ref(data))))
#define MAP_GENOBJ_DATAREF_2(name, obj, call, data, param1, param2) \
    m_call_table.insert(std::make_pair(#name, bind(&call, &obj, _1, \
                        std::ref(data), param1, param2)))

#define MAP_GENOBJ_DATA_DATAREF(name, obj, call, data, dataref) \
    m_call_table.insert(std::make_pair(#name, bind(&call, &obj, _1, data, std::ref(dataref))))

#define MAP_DATAREF_DATA(name, call, data, param1) \
    m_call_table.insert(std::make_pair(#name, bind(&FrameTrimmeImpl:: call, _1, \
                        std::ref(data), param1)))



void FrameTrimmeImpl::register_legacy_calls()
{
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
               LegacyProgramStateMap::generate);
    MAP_GENOBJ(glDeletePrograms, m_legacy_programs,
               LegacyProgramStateMap::destroy);
    MAP_GENOBJ_DATAREF(glBindProgram, m_legacy_programs,
               LegacyProgramStateMap::bind_shader, m_fbo.current_framebuffer());
    MAP_GENOBJ(glProgramString, m_legacy_programs,
               LegacyProgramStateMap::program_string);
    MAP_GENOBJ(glProgramLocalParameter, m_legacy_programs,
               LegacyProgramStateMap::program_parameter);


    // Matrix manipulation
    MAP_GENOBJ(glLoadIdentity, m_matrix_states, AllMatrisStates::LoadIdentity);
    MAP_GENOBJ(glLoadMatrix, m_matrix_states, AllMatrisStates::LoadMatrix);
    MAP_GENOBJ(glMatrixMode, m_matrix_states, AllMatrisStates::MatrixMode);
    MAP_GENOBJ(glMultMatrix, m_matrix_states, AllMatrisStates::matrix_op);
    MAP_GENOBJ(glOrtho, m_matrix_states, AllMatrisStates::matrix_op);
    MAP_GENOBJ(glRotate, m_matrix_states, AllMatrisStates::matrix_op);
    MAP_GENOBJ(glScale, m_matrix_states, AllMatrisStates::matrix_op);
    MAP_GENOBJ(glTranslate, m_matrix_states, AllMatrisStates::matrix_op);
    MAP_GENOBJ(glPopMatrix, m_matrix_states, AllMatrisStates::PopMatrix);
    MAP_GENOBJ(glPushMatrix, m_matrix_states, AllMatrisStates::PushMatrix);
}

void FrameTrimmeImpl::register_framebuffer_calls()
{
    MAP_GENOBJ_DATA_DATAREF(glBindRenderbuffer, m_renderbuffers, RenderbufferMap::bind, 1,
                            m_fbo.current_framebuffer());
    MAP_GENOBJ(glDeleteRenderbuffers, m_renderbuffers, RenderbufferMap::destroy);
    MAP_GENOBJ(glGenRenderbuffer, m_renderbuffers, RenderbufferMap::generate);
    MAP_GENOBJ(glRenderbufferStorage, m_renderbuffers, RenderbufferMap::storage);

    MAP_GENOBJ(glGenFramebuffer, m_fbo, FramebufferStateMap::generate);
    MAP_GENOBJ(glDeleteFramebuffers, m_fbo, FramebufferStateMap::destroy);
    MAP_GENOBJ_DATAREF(glBindFramebuffer, m_fbo, FramebufferStateMap::bind_fbo,
                       m_recording_frame);
    MAP_GENOBJ(glViewport, m_fbo, FramebufferStateMap::viewport);

    MAP_GENOBJ(glBlitFramebuffer, m_fbo, FramebufferStateMap::blit);
    MAP_GENOBJ_DATAREF_2(glFramebufferTexture, m_fbo,
                         FramebufferStateMap::attach_texture, m_textures,
                         2, 3);
    MAP_GENOBJ_DATAREF_2(glFramebufferTexture1D, m_fbo,
                         FramebufferStateMap::attach_texture, m_textures,
                         3, 4);
    MAP_GENOBJ_DATAREF_2(glFramebufferTexture2D, m_fbo,
                         FramebufferStateMap::attach_texture, m_textures,
                         3, 4);

    MAP_GENOBJ(glReadBuffer, m_fbo, FramebufferStateMap::read_buffer);
    MAP_GENOBJ(glDrawBuffer, m_fbo, FramebufferStateMap::draw_buffer);


/*    MAP_GENOBJ_DATAREF(glFramebufferTexture3D, m_fbo,
                         FramebufferStateMap::attach_texture3d, m_textures);
      MAP(glReadBuffer, ReadBuffer); */


    MAP_GENOBJ_DATAREF(glFramebufferRenderbuffer, m_fbo,
                       FramebufferStateMap::attach_renderbuffer, m_renderbuffers);

    MAP_GENOBJ(glClear, m_fbo, FramebufferStateMap::clear);

}

void
FrameTrimmeImpl::register_buffer_calls()
{
    MAP_GENOBJ(glGenBuffers, m_buffers, BufferStateMap::generate);
    MAP_GENOBJ(glDeleteBuffers, m_buffers, BufferStateMap::destroy);

    MAP_GENOBJ_DATA_DATAREF(glBindBuffer, m_buffers, BufferStateMap::bind, 1,
                            m_fbo.current_framebuffer());

    /* This will need a bit more to be handled correctly */
    MAP_GENOBJ_DATA_DATAREF(glBindBufferBase, m_buffers, BufferStateMap::bind, 2,
                            m_fbo.current_framebuffer());

    MAP_GENOBJ(glBufferData, m_buffers, BufferStateMap::data);
    MAP_GENOBJ(glBufferSubData, m_buffers, BufferStateMap::sub_data);

    MAP_GENOBJ(glMapBuffer, m_buffers, BufferStateMap::map);
    MAP_GENOBJ(glMapBufferRange, m_buffers, BufferStateMap::map_range);
    MAP_GENOBJ(memcpy, m_buffers, BufferStateMap::memcpy);
    MAP_GENOBJ(glUnmapBuffer, m_buffers, BufferStateMap::unmap);
}

void FrameTrimmeImpl::register_draw_calls()
{
    MAP(glDrawArrays, DrawArrays);
    MAP(glDrawElements, DrawElements);
    MAP(glDrawRangeElements, DrawElements);
    MAP(glDrawRangeElementsBaseVertex, DrawElements);
}

void
FrameTrimmeImpl::register_program_calls()
{
    MAP_GENOBJ_DATAREF(glAttachObject, m_programs,
                       ProgramStateMap::attach_shader, m_shaders);
    MAP_GENOBJ_DATAREF(glAttachShader, m_programs,
                       ProgramStateMap::attach_shader, m_shaders);

    MAP_GENOBJ(glCompileShader, m_shaders, ShaderStateMap::data);
    MAP_GENOBJ(glCreateShader, m_shaders, ShaderStateMap::create);
    MAP_GENOBJ(glDeleteShader, m_shaders, ShaderStateMap::destroy);
    MAP_GENOBJ(glShaderSource, m_shaders, ShaderStateMap::data);
    MAP_GENOBJ(glBindAttribLocation, m_programs,
               ProgramStateMap::bind_attr_location);
    MAP_GENOBJ(glCreateProgram, m_programs, ProgramStateMap::create);
    MAP_GENOBJ(glGetAttribLocation, m_programs, ProgramStateMap::data);
    MAP_GENOBJ(glGetUniformLocation, m_programs, ProgramStateMap::data);
    MAP_GENOBJ(glBindFragDataLocation, m_programs, ProgramStateMap::data);
    MAP_GENOBJ(glLinkProgram, m_programs, ProgramStateMap::data);
    MAP_GENOBJ(glProgramBinary, m_programs, ProgramStateMap::data);
    MAP_GENOBJ(glDeleteProgram, m_programs, ProgramStateMap::destroy);
    MAP_GENOBJ(glUniform, m_programs, ProgramStateMap::uniform);
    MAP_GENOBJ_DATAREF(glUseProgram, m_programs, ProgramStateMap::use,
                       m_fbo.current_framebuffer());
    MAP_GENOBJ_DATA(glProgramParameter, m_programs, ProgramStateMap::set_state, 2);
}

void FrameTrimmeImpl::register_texture_calls()
{
    MAP_GENOBJ(glGenTextures, m_textures, TextureStateMap::generate);
    MAP_GENOBJ(glDeleteTextures, m_textures, TextureStateMap::destroy);

    MAP_GENOBJ(glActiveTexture, m_textures, TextureStateMap::active_texture);
    MAP_GENOBJ(glClientActiveTexture, m_textures, TextureStateMap::active_texture);
    MAP_GENOBJ_DATA_DATAREF(glBindTexture, m_textures, TextureStateMap::bind, 1,
                            m_fbo.current_framebuffer());
    MAP_GENOBJ(glBindMultiTexture, m_textures, TextureStateMap::bind_multitex);

    MAP_GENOBJ(glCompressedTexImage2D, m_textures, TextureStateMap::set_data);
    MAP_GENOBJ(glGenerateMipmap, m_textures, TextureStateMap::gen_mipmap);
    MAP_GENOBJ(glTexImage1D, m_textures, TextureStateMap::set_data);
    MAP_GENOBJ(glTexImage2D, m_textures, TextureStateMap::set_data);
    MAP_GENOBJ(glTexStorage2D, m_textures, TextureStateMap::storage);
    MAP_GENOBJ(glTexImage3D, m_textures, TextureStateMap::set_data);
    MAP_GENOBJ(glTexSubImage1D, m_textures, TextureStateMap::set_sub_data);
    MAP_GENOBJ(glTexSubImage2D, m_textures, TextureStateMap::set_sub_data);
    MAP_GENOBJ(glTexSubImage3D, m_textures, TextureStateMap::set_sub_data);
    MAP_GENOBJ_DATA(glTexParameter, m_textures, TextureStateMap::set_state, 2);

    /*
    MAP_GENOBJ(glCopyTexSubImage2D, m_textures, TextureStateMap::copy_sub_data);
    */


    MAP_GENOBJ_DATA_DATAREF(glBindSampler, m_samplers, SamplerStateMap::bind, 1,
                            m_fbo.current_framebuffer());
    MAP_GENOBJ(glGenSamplers, m_samplers, SamplerStateMap::generate);
    MAP_GENOBJ(glDeleteSamplers, m_samplers, SamplerStateMap::destroy);
    MAP_GENOBJ_DATA(glSamplerParameter, m_samplers, SamplerStateMap::set_state, 2);
}

void
FrameTrimmeImpl::register_state_calls()
{
    /* These Functions change the state and we only need to remember the last
     * call before the target frame or an fbo creating a dependency is draw. */
    const std::vector<const char *> state_calls  = {
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
        "glPatchParameteri",
        "glPixelZoom",
        "glPointSize",
        "glPolygonMode",
        "glPolygonOffset",
        "glPolygonStipple",
        "glPrimitiveBoundingBox",
        "glSampleCoverage",
        "glShadeModel",
        "glScissor",
        "glStencilFuncSeparate",
        "glStencilMask",
        "glStencilOpSeparate",
        "glXSwapBuffers",
        "eglSwapBuffers",
        "glFinish",
    };

    auto state_call_func = bind(&FrameTrimmeImpl::record_state_call, this, _1, 0);
    update_call_table(state_calls, state_call_func);

    /* These are state functions with an extra parameter */
    auto state_call_1_func = bind(&FrameTrimmeImpl::record_state_call, this, _1, 1);
    const std::vector<const char *> state_calls_1  = {
        "glClipPlane",
        "glColorMaskIndexedEXT",
        "glColorMaterial",
        "glFog",
        "glHint",
        "glLight",
        "glPixelStorei",
        "glPixelTransfer",
        "glVertexAttribDivisor",
    };
    update_call_table(state_calls_1, state_call_1_func);

    /* These are state functions with an extra parameter */
    auto state_call_2_func = bind(&FrameTrimmeImpl::record_state_call, this, _1, 2);
    const std::vector<const char *> state_calls_2  = {
        "glMaterial",
        "glTexEnv",
    };
    update_call_table(state_calls_2, state_call_2_func);

    MAP(glDisable, record_enable);
    MAP(glEnable, record_enable);
}

void FrameTrimmeImpl::register_required_calls()
{
    /* These function set up the context and are, therefore, required
     * TODO: figure out what is really required, and whether the can be
     * tracked like state variables. */
    auto required_func = bind(&FrameTrimmeImpl::record_required_call, this, _1);
    const std::vector<const char *> required_calls = {
        "glXChooseVisual",
        "glXCreateContext",
        "glXDestroyContext",
        "glXMakeCurrent",
        "glXMakeContextCurrent",
        "glXChooseFBConfig",
        "glXQueryExtensionsString",
        "glXSwapIntervalMESA",

        "eglInitialize",
        "eglCreatePlatformWindowSurface",
        "eglBindAPI",
        "eglCreateContext",
        "eglMakeCurrent"
    };
    update_call_table(required_calls, required_func);
}

void FrameTrimmeImpl::register_ignore_history_calls()
{
    /* These functions are ignored outside required recordings
     * TODO: Delete calls should probably really delete things */
    const std::vector<const char *> ignore_history_calls = {
        "glCheckFramebufferStatus",
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
        "glReadPixels",
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
        "eglGetProcAddress",
        "eglQueryString",
        "eglGetError",
        "eglGetPlatformDisplay",
        "eglGetConfigs",
        "eglGetConfigAttrib",
        "eglQuerySurface",
     };
    auto ignore_history_func = bind(&FrameTrimmeImpl::ignore_history, this, _1);
    update_call_table(ignore_history_calls, ignore_history_func);
}


void
FrameTrimmeImpl::register_va_calls()
{
    MAP_GENOBJ(glGenVertexArrays, m_vertex_arrays,
               TGenObjStateMap<VertexArray>::generate);
    MAP_GENOBJ(glDeleteVertexArrays, m_vertex_arrays,
               TGenObjStateMap<VertexArray>::destroy);
    MAP_GENOBJ_DATA_DATAREF(glBindVertexArray, m_vertex_arrays,
                            TGenObjStateMap<VertexArray>::bind, 0,
                            m_fbo.current_framebuffer());

    MAP_DATA2(glDisableVertexAttribArray, record_va_enable, false, pt_va);
    MAP_DATA2(glEnableVertexAttribArray, record_va_enable, true, pt_va);

    MAP_DATA(glVertexAttribPointer, AttribPointer, pt_va);
    MAP_DATA(glVertexPointer, AttribPointer, pt_vertex);
    MAP_DATA(glTexCoordPointer, AttribPointer, pt_texcoord);

    MAP_DATA(glDisableClientState, record_client_state_enable, false);
    MAP_DATA(glEnableClientState, record_client_state_enable, true);

}

void
FrameTrimmeImpl::update_call_table(const std::vector<const char*>& names,
                                        ft_callback cb)
{
    for (auto& i : names)
        m_call_table.insert(std::make_pair(i, cb));
}

PTraceCall
FrameTrimmeImpl::Begin(const trace::Call& call)
{
    auto c = trace2call(call);
    if (m_active_display_list)
        m_active_display_list->append_call(c);
    return c;
}

PTraceCall
FrameTrimmeImpl::End(const trace::Call& call)
{
    auto c = trace2call(call);
    if (m_active_display_list)
        m_active_display_list->append_call(c);
    return c;
}


PTraceCall
FrameTrimmeImpl::Vertex(const trace::Call& call)
{
    auto c = trace2call(call);
    if (m_active_display_list)
        m_active_display_list->append_call(c);
    return c;
}

PTraceCall
FrameTrimmeImpl::EndList(const trace::Call& call)
{
    auto c = trace2call(call);
    if (!m_recording_frame)
        m_active_display_list->append_call(c);

    m_active_display_list = nullptr;
    return c;
}

PTraceCall
FrameTrimmeImpl::GenLists(const trace::Call& call)
{
    unsigned nlists = call.arg(0).toUInt();
    GLuint origResult = (*call.ret).toUInt();
    for (unsigned i = 0; i < nlists; ++i)
        m_display_lists[i + origResult] = std::make_shared<DisplayLists>(i + origResult);

    auto c = trace2call(call);
    if (!m_recording_frame)
        m_display_lists[origResult]->append_call(c);
    return c;
}

PTraceCall
FrameTrimmeImpl::NewList(const trace::Call& call)
{
    assert(!m_active_display_list);
    auto list  = m_display_lists.find(call.arg(0).toUInt());
    assert(list != m_display_lists.end());
    m_active_display_list = list->second;

    auto c = trace2call(call);
    if (!m_recording_frame)
        m_active_display_list->append_call(c);
    return c;
}

PTraceCall
FrameTrimmeImpl::CallList(const trace::Call& call)
{
    auto list  = m_display_lists.find(call.arg(0).toUInt());
    assert(list != m_display_lists.end());

    if (m_recording_frame)
        list->second->emit_calls_to_list(m_required_calls);

    return trace2call(call);
}

PTraceCall
FrameTrimmeImpl::DeleteLists(const trace::Call& call)
{
    GLint value = call.arg(0).toUInt();
    GLint value_end = call.arg(1).toUInt() + value;
    for(int i = value; i < value_end; ++i) {
        auto list  = m_display_lists.find(call.arg(0).toUInt());
        assert(list != m_display_lists.end());
        m_display_lists.erase(list);
    }
    return trace2call(call);
}

PTraceCall FrameTrimmeImpl::DrawElements(const trace::Call& call)
{
    auto c = m_fbo.current_framebuffer().draw(call);
    auto ibo = m_buffers.bound_to(GL_ELEMENT_ARRAY_BUFFER);
    if (ibo) {
        if (m_recording_frame) {
            c = ibo->use(call);
            ibo->emit_calls_to_list(m_required_calls);
        }
        ibo->flush_state_cache(m_fbo.current_framebuffer());
    }
    return c;
}

PTraceCall FrameTrimmeImpl::DrawArrays(const trace::Call& call)
{
    return m_fbo.current_framebuffer().draw(call);
}

PTraceCall FrameTrimmeImpl::record_va_enable(const trace::Call& call,
                                             bool enable, PointerType type)
{
    unsigned id = type == pt_va ? call.arg(0).toUInt() * pt_last + type:
                                  (unsigned)type;
    m_va_enables[id] = trace2call(call);
    m_va_is_enabled[id] = enable;
    return m_va_enables[id];
}

PTraceCall
FrameTrimmeImpl::record_client_state_enable(const trace::Call& call,
                                                       bool enable)
{
    auto state = call.arg(0).toUInt();
    PointerType type;
    switch (state) {
    case GL_VERTEX_ARRAY: type = pt_vertex; break;
    case GL_TEXTURE_COORD_ARRAY: type = pt_texcoord; break;
    case GL_COLOR_ARRAY: type = pt_color; break;
    case GL_NORMAL_ARRAY: type = pt_normal; break;
    default:
        assert(0 && "Client state not supported");
    }
    return record_va_enable(call, enable, type);
}

PTraceCall FrameTrimmeImpl::AttribPointer(const trace::Call& call, PointerType type)
{
    auto buf = m_buffers.bound_to(GL_ARRAY_BUFFER);
    PTraceCall c = trace2call(call);
    if (buf) {
        c = buf->use(call);
        unsigned id = type == pt_va ? call.arg(0).toUInt() * pt_last + type:
                                      (unsigned)type;
        m_vertex_attr_pointer[id] = buf;
    }
    return c;
}

PTraceCall
FrameTrimmeImpl::todo(const trace::Call& call)
{
    std::cerr << "TODO: " << call.name() << "\n";
    return trace2call(call);
}

PTraceCall FrameTrimmeImpl::ignore_history(const trace::Call& call)
{
    return trace2call(call);
}

PTraceCall
FrameTrimmeImpl::record_required_call(const trace::Call& call)
{
    auto c = trace2call(call);
    m_required_calls.insert(c);
    return c;
}

}
