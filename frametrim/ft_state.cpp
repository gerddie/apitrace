#include "ft_state.hpp"
#include "ft_bufferstate.hpp"

#include "ft_framebufferstate.hpp"
#include "ft_matrixstate.hpp"
#include "ft_programstate.hpp"
#include "ft_samplerstate.hpp"
#include "ft_texturestate.hpp"


#include <unordered_set>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <stack>
#include <set>
#include <list>
#include <sstream>

#include <GL/glext.h>

namespace frametrim {

using std::bind;
using std::placeholders::_1;
using std::make_shared;

struct StateImpl {

    StateImpl(GlobalState *gs);

    void call(PCall call);


    /* OpenGL calls */
    void Begin(PCall call);
    void CallList(PCall call);
    void Clear(PCall call);

    void DeleteLists(PCall call);
    void DrawElements(PCall call);
    void End(PCall call);
    void EndList(PCall call);

    void GenLists(PCall call);
    void NewList(PCall call);

    void ReadBuffer(PCall call);
    void DrawBuffers(PCall call);

    void ShadeModel(PCall call);

    void Vertex(PCall call);
    void Viewport(PCall call);
    void VertexAttribPointer(PCall call);

    void ignore_history(PCall call);
    void todo(PCall call);


    void record_enable(PCall call);
    void record_va_enables(PCall call);

    void record_default_fb_state_call(PCall call);

    void record_state_call(PCall call);
    void record_state_call_ex(PCall call);
    void record_state_call_ex2(PCall call);
    void record_required_call(PCall call);

    void start_target_farme(unsigned callno);

    void collect_state_calls(CallSet& lisr) const;

    void register_callbacks();

    void register_buffer_calls();
    void register_legacy_calls();
    void register_state_calls();
    void register_ignore_history_calls();
    void register_required_calls();
    void register_texture_calls();
    void register_program_calls();
    void register_framebuffer_calls();


    void update_call_table(const std::vector<const char*>& names,
                           ft_callback cb);

    std::vector<unsigned> get_sorted_call_ids() const;

    using CallTable = std::multimap<const char *, ft_callback, string_part_less>;
    CallTable m_call_table;

    bool m_in_target_frame;

    CallSet m_required_calls;


    AllMatrisStates m_matrix_states;

    ShaderStateMap m_shaders;
    ProgramStateMap m_programs;
    LegacyProgramStateMap m_legacy_programs;

    std::unordered_map<GLenum, PCall> m_enables;
    std::unordered_map<GLint, PCall> m_va_enables;


    std::unordered_map<GLint, PObjectState> m_display_lists;
    PObjectState m_active_display_list;

    BufferStateMap m_buffers;

    SamplerStateMap m_samplers;
    TextureStateMap m_textures;


    TGenObjStateMap<ObjectState> m_vertex_arrays;
    std::unordered_map<GLint, PBufferState> m_vertex_attr_pointer;

    FramebufferMap m_framebuffers;
    RenderbufferMap m_renderbuffers;

    std::unordered_map<std::string, PTraceCall> m_state_calls;
    // These state calls make no sense with fbos, and must not be emitted
    // to the draw_fbo
    std::unordered_map<std::string, PTraceCall> m_default_fb_state_calls;

    std::set<std::string> m_unhandled_calls;
};


void State::call(PCall call)
{
    impl->call(call);
}

State::State()
{
    impl = new StateImpl(this);

    impl->register_callbacks();
}

State::~State()
{
    if (!impl->m_unhandled_calls.empty()) {
        std::cerr << "Unhandled calls: ";
        for (auto& call: impl->m_unhandled_calls)
            std::cerr << "    '" << call << "'\n";
    }
    delete impl;
}

bool State::in_target_frame() const
{
    return impl->m_in_target_frame;
}

CallSet &State::global_callset()
{
    return impl->m_required_calls;
}

PFramebufferState State::draw_framebuffer() const
{
    return impl->m_framebuffers.draw_fb();
}

PFramebufferState State::read_framebuffer() const
{
    return impl->m_framebuffers.read_fb();
}

void State::collect_state_calls(CallSet& list)
{
    impl->collect_state_calls(list);
}

std::vector<unsigned> State::get_sorted_call_ids() const
{
    return impl->get_sorted_call_ids();
}

void State::target_frame_started(unsigned callno)
{
    if (impl->m_in_target_frame)
        return;

    impl->m_in_target_frame = true;
    impl->start_target_farme(callno);
}

void StateImpl::collect_state_calls(CallSet& list) const
{
    for(auto& a : m_state_calls)
        list.insert(a.second);

    for(auto& cap: m_enables)
        list.insert(trace2call(*cap.second));

    m_matrix_states.emit_state_to_lists(list);

    /* Set vertex attribute array pointers only if they are enabled */
    for(auto& va : m_vertex_attr_pointer) {
        auto vae = m_va_enables.find(va.first);
        if (vae != m_va_enables.end() &&
                !strcmp(vae->second->name(), "glEnableVertexAttribArray")) {
            va.second->emit_calls_to_list(list);
            list.insert(trace2call(*vae->second));
        }
    }

    m_vertex_arrays.emit_calls_to_list(list);
    m_buffers.emit_calls_to_list(list);
    m_programs.emit_calls_to_list(list);
    m_legacy_programs.emit_calls_to_list(list);
    m_samplers.emit_calls_to_list(list);
    m_textures.emit_calls_to_list(list);

}

void StateImpl::start_target_farme(unsigned callno)
{
    std::cerr << "m_required_calls.set_reference_call_no(" << callno << ");\n";

    collect_state_calls(m_required_calls);
    for(auto& c: m_default_fb_state_calls) {
        if (c.second)
            m_required_calls.insert(c.second);
    }

    auto default_fb = m_framebuffers.default_fb();
    if (default_fb)
       default_fb->emit_calls_to_list(m_required_calls);

}

StateImpl::StateImpl(GlobalState *gs):
    m_in_target_frame(false),
    m_shaders(gs),
    m_programs(gs),
    m_legacy_programs(gs),
    m_buffers(gs),
    m_samplers(gs),
    m_textures(gs),
    m_vertex_arrays(gs),
    m_framebuffers(gs),
    m_renderbuffers(gs)
{
}

void StateImpl::call(PCall call)
{
    auto cb_range = m_call_table.equal_range(call->name());
    if (cb_range.first != m_call_table.end() &&
            std::distance(cb_range.first, cb_range.second) > 0) {

        CallTable::const_iterator cb = cb_range.first;
        CallTable::const_iterator i = cb_range.first;
        ++i;

        unsigned max_equal = equal_chars(cb->first, call->name());

        while (i != cb_range.second && i != m_call_table.end()) {
            auto n = equal_chars(i->first, call->name());
            if (n > max_equal) {
                max_equal = n;
                cb = i;
            }
            ++i;
        }

        //std::cerr << "Handle " << call->name() << " as " << cb->first << "\n";
        cb->second(call);
    } else {
        /* This should be some debug output only, because we might
         * not handle some calls deliberately */
        if (m_unhandled_calls.find(call->name()) == m_unhandled_calls.end()) {
            std::cerr << "Call " << call->no
                      << " " << call->name() << " not handled\n";
            m_unhandled_calls.insert(call->name());
        }
    }

    if (m_in_target_frame)
        m_required_calls.insert(trace2call(*call));
    else if (m_framebuffers.draw_fb())
        m_framebuffers.draw_fb()->draw(call);
    else if (m_framebuffers.default_fb())
       m_framebuffers.default_fb()->draw(call);
}

void StateImpl::Begin(PCall call)
{
    if (m_active_display_list)
        m_active_display_list->append_call(trace2call(*call));
}

void StateImpl::CallList(PCall call)
{
    auto list  = m_display_lists.find(call->arg(0).toUInt());
    assert(list != m_display_lists.end());

    if (m_in_target_frame)
        list->second->emit_calls_to_list(m_required_calls);

    auto draw_fbo = m_framebuffers.draw_fb();
    if (draw_fbo)
        list->second->emit_calls_to_list(draw_fbo->state_calls());
}

void StateImpl::Clear(PCall call)
{
    auto draw_fbo = m_framebuffers.draw_fb();
    if (draw_fbo)
        draw_fbo->clear(call);
}

void StateImpl::DeleteLists(PCall call)
{
    GLint value = call->arg(0).toUInt();
    GLint value_end = call->arg(1).toUInt() + value;
    for(int i = value; i < value_end; ++i) {
        auto list  = m_display_lists.find(call->arg(0).toUInt());
        assert(list != m_display_lists.end());
        m_display_lists.erase(list);
    }
}


void StateImpl::DrawElements(PCall call)
{
    (void)call;

    auto buf = m_buffers.bound_to(GL_ELEMENT_ARRAY_BUFFER);
    if (buf) {
        if (m_in_target_frame)
            buf->emit_calls_to_list(m_required_calls);

        auto draw_fbo = m_framebuffers.draw_fb();
        if (draw_fbo)
            buf->emit_calls_to_list(draw_fbo->state_calls());
        buf->use();
    }
}

void StateImpl::record_va_enables(PCall call)
{
    m_va_enables[call->arg(0).toUInt()] = call;
}

void StateImpl::record_enable(PCall call)
{
    GLenum value = call->arg(0).toUInt();
    m_enables[value] = call;
}

void StateImpl::End(PCall call)
{
    if (m_active_display_list)
        m_active_display_list->append_call(trace2call(*call));
}

void StateImpl::EndList(PCall call)
{
    if (!m_in_target_frame)
        m_active_display_list->append_call(trace2call(*call));

    m_active_display_list = nullptr;
}

void StateImpl::GenLists(PCall call)
{
    unsigned nlists = call->arg(0).toUInt();
    GLuint origResult = (*call->ret).toUInt();
    for (unsigned i = 0; i < nlists; ++i)
        m_display_lists[i + origResult] = PObjectState(new ObjectState(i + origResult));

    if (!m_in_target_frame)
        m_display_lists[origResult]->append_call(trace2call(*call));
}

void StateImpl::NewList(PCall call)
{
    assert(!m_active_display_list);
    auto list  = m_display_lists.find(call->arg(0).toUInt());
    assert(list != m_display_lists.end());
    m_active_display_list = list->second;

    if (!m_in_target_frame)
        m_active_display_list->append_call(trace2call(*call));
}

void StateImpl::ReadBuffer(PCall call)
{
    auto read_fb = m_framebuffers.read_fb();
    if (read_fb)
        read_fb->set_state_call(call, 0);
    else
        record_state_call(call);
}

void StateImpl::DrawBuffers(PCall call)
{
    auto draw_fb = m_framebuffers.draw_fb();
    if (draw_fb)
        draw_fb->set_state_call(call, 0);
    else
        record_default_fb_state_call(call);
}

void StateImpl::ShadeModel(PCall call)
{
    if (m_active_display_list)
        m_active_display_list->append_call(trace2call(*call));
}

void StateImpl::Vertex(PCall call)
{
    if (m_active_display_list)
        m_active_display_list->append_call(trace2call(*call));
}

void StateImpl::VertexAttribPointer(PCall call)
{
    auto buf = m_buffers.bound_to(GL_ARRAY_BUFFER);
    auto draw_fb = m_framebuffers.draw_fb();

    if (buf) {
        m_vertex_attr_pointer[call->arg(0).toUInt()] = buf;
        if (m_in_target_frame)
            buf->emit_calls_to_list(m_required_calls);
        else if (draw_fb)
            buf->emit_calls_to_list(draw_fb->state_calls());
        buf->use(call);
    } else {
        m_vertex_attr_pointer[call->arg(0).toUInt()] = nullptr;
    }
}

void StateImpl::Viewport(PCall call)
{
    auto draw_fb = m_framebuffers.draw_fb();

    if (draw_fb)
        draw_fb->set_viewport(call);
    else {
       auto default_fb = m_framebuffers.default_fb();
       if (default_fb)
          default_fb->set_viewport(call);
       record_state_call(call);
    }
}

void StateImpl::record_state_call(PCall call)
{
    m_state_calls[call->name()] = trace2call(*call);
    if (m_active_display_list)
        m_active_display_list->append_call(trace2call(*call));
}

void StateImpl::record_default_fb_state_call(PCall call)
{
    m_default_fb_state_calls[call->name()] = trace2call(*call);
    if (m_active_display_list)
        m_active_display_list->append_call(trace2call(*call));
}


void StateImpl::record_state_call_ex(PCall call)
{
    std::stringstream s;
    s << call->name() << "_" << call->arg(0).toUInt();
    m_state_calls[s.str()] = trace2call(*call);
    if (m_active_display_list)
        m_active_display_list->append_call(trace2call(*call));
}

void StateImpl::record_state_call_ex2(PCall call)
{
    std::stringstream s;
    s << call->name() << "_"
      << call->arg(0).toUInt()
      << "_" << call->arg(1).toUInt();

    m_state_calls[s.str()] = trace2call(*call);

    if (m_active_display_list)
        m_active_display_list->append_call(trace2call(*call));

}

void StateImpl::record_required_call(PCall call)
{
    if (call)
        m_required_calls.insert(trace2call(*call));
}

void StateImpl::ignore_history(PCall call)
{
    (void)call;
}

void StateImpl::todo(PCall call)
{
    std::cerr << "TODO:" << call->name() << "\n";
}

std::vector<unsigned> StateImpl::get_sorted_call_ids() const
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

void StateImpl::register_callbacks()
{
#define MAP(name, call) m_call_table.insert(std::make_pair(#name, bind(&StateImpl:: call, this, _1)))
#define MAP_GENOBJ(name, obj, call) \
    m_call_table.insert(std::make_pair(#name, bind(&call, &obj, _1)))
#define MAP_GENOBJ_DATA(name, obj, call, data) \
    m_call_table.insert(std::make_pair(#name, bind(&call, &obj, _1, data)))
#define MAP_GENOBJ_DATAREF(name, obj, call, data) \
    m_call_table.insert(std::make_pair(#name, bind(&call, &obj, _1, std::ref(data))))


    MAP(glClear, Clear);

    MAP(glDisableVertexAttribArray, record_va_enables);
    MAP(glDrawElements, DrawElements);
    MAP(glDrawRangeElements, DrawElements);
    MAP(glEnableVertexAttribArray, record_va_enables);

    MAP_GENOBJ(glGenVertexArrays, m_vertex_arrays,
               TGenObjStateMap<ObjectState>::generate);
    MAP_GENOBJ(glDeleteVertexArrays, m_vertex_arrays,
               TGenObjStateMap<ObjectState>::destroy);

    MAP(glVertexAttribPointer, VertexAttribPointer);
    MAP(glViewport, Viewport);

    register_required_calls();
    register_state_calls();
    register_ignore_history_calls();
    register_legacy_calls();
    register_buffer_calls();
    register_framebuffer_calls();
    register_program_calls();
    register_texture_calls();
}

void StateImpl::register_buffer_calls()
{
    MAP_GENOBJ(glGenBuffers, m_buffers, BufferStateMap::generate);
    MAP_GENOBJ(glDeleteBuffers, m_buffers, BufferStateMap::destroy);

    MAP_GENOBJ(glBindBuffer, m_buffers, BufferStateMap::bind);
    MAP_GENOBJ(glBufferData, m_buffers, BufferStateMap::data);
    MAP_GENOBJ(glBufferSubData, m_buffers, BufferStateMap::sub_data);

    MAP_GENOBJ(glMapBuffer, m_buffers, BufferStateMap::map);
    MAP_GENOBJ(glMapBufferRange, m_buffers, BufferStateMap::map_range);
    MAP_GENOBJ(memcpy, m_buffers, BufferStateMap::memcpy);
    MAP_GENOBJ(glUnmapBuffer, m_buffers, BufferStateMap::unmap);
}

void StateImpl::register_program_calls()
{
    MAP_GENOBJ_DATAREF(glAttachObject, m_programs,
                       ProgramStateMap::attach_shader, m_shaders);
    MAP_GENOBJ_DATAREF(glAttachShader, m_programs,
                       ProgramStateMap::attach_shader, m_shaders);

    MAP_GENOBJ(glCompileShader, m_shaders, ShaderStateMap::data);
    MAP_GENOBJ(glCreateShader, m_shaders, ShaderStateMap::create);
    MAP_GENOBJ(glShaderSource, m_shaders, ShaderStateMap::data);
    MAP_GENOBJ(glBindAttribLocation, m_programs,
               ProgramStateMap::bind_attr_location);
    MAP_GENOBJ(glCreateProgram, m_programs, ProgramStateMap::create);
    MAP_GENOBJ(glGetAttribLocation, m_programs, ProgramStateMap::data);
    MAP_GENOBJ(glGetUniformLocation, m_programs, ProgramStateMap::data);
    MAP_GENOBJ(glBindFragDataLocation, m_programs, ProgramStateMap::data);
    MAP_GENOBJ(glLinkProgram, m_programs, ProgramStateMap::data);
    MAP_GENOBJ(glProgramBinary, m_programs, ProgramStateMap::data);
    MAP_GENOBJ(glUniform, m_programs, ProgramStateMap::uniform);
    MAP_GENOBJ(glUseProgram, m_programs, ProgramStateMap::use);
    MAP_GENOBJ_DATA(glProgramParameter, m_programs, ProgramStateMap::set_state, 2);
}

void StateImpl::register_texture_calls()
{
    MAP_GENOBJ(glGenTextures, m_textures, TextureStateMap::generate);
    MAP_GENOBJ(glDeleteTextures, m_textures, TextureStateMap::destroy);

    MAP_GENOBJ(glActiveTexture, m_textures, TextureStateMap::active_texture);
    MAP_GENOBJ(glClientActiveTexture, m_textures, TextureStateMap::active_texture);
    MAP_GENOBJ(glBindTexture, m_textures, TextureStateMap::bind);
    MAP_GENOBJ(glBindMultiTexture, m_textures, TextureStateMap::bind_multitex);

    MAP_GENOBJ(glCompressedTexImage2D, m_textures, TextureStateMap::set_data);
    MAP_GENOBJ(glGenerateMipmap, m_textures, TextureStateMap::gen_mipmap);
    MAP_GENOBJ(glTexImage1D, m_textures, TextureStateMap::set_data);
    MAP_GENOBJ(glTexImage2D, m_textures, TextureStateMap::set_data);
    MAP_GENOBJ(glTexImage3D, m_textures, TextureStateMap::set_data);
    MAP_GENOBJ(glTexSubImage1D, m_textures, TextureStateMap::set_sub_data);
    MAP_GENOBJ(glTexSubImage2D, m_textures, TextureStateMap::set_sub_data);
    MAP_GENOBJ(glTexSubImage3D, m_textures, TextureStateMap::set_sub_data);
    MAP_GENOBJ(glCopyTexSubImage2D, m_textures, TextureStateMap::copy_sub_data);
    MAP_GENOBJ_DATA(glTexParameter, m_textures, TextureStateMap::set_state, 2);

    MAP_GENOBJ(glBindSampler, m_samplers, SamplerStateMap::bind);
    MAP_GENOBJ(glGenSamplers, m_samplers, SamplerStateMap::generate);
    MAP_GENOBJ(glDeleteSamplers, m_samplers, SamplerStateMap::destroy);
    MAP_GENOBJ_DATA(glSamplerParameter, m_samplers, SamplerStateMap::set_state, 2);
}

void StateImpl::register_framebuffer_calls()
{
    MAP_GENOBJ(glBindFramebuffer, m_framebuffers, FramebufferMap::bind);
    MAP_GENOBJ(glBlitFramebuffer, m_framebuffers, FramebufferMap::blit);
    MAP_GENOBJ_DATAREF(glFramebufferRenderbuffer, m_framebuffers,
                       FramebufferMap::renderbuffer, m_renderbuffers);
    MAP_GENOBJ_DATAREF(glFramebufferTexture, m_framebuffers,
                       FramebufferMap::texture, m_textures);
    MAP_GENOBJ(glGenFramebuffer, m_framebuffers, FramebufferMap::generate);
    MAP_GENOBJ(glDeleteFramebuffers, m_framebuffers, FramebufferMap::destroy);

    MAP_GENOBJ(glBindRenderbuffer, m_renderbuffers, RenderbufferMap::bind);
    MAP_GENOBJ(glDeleteRenderbuffers, m_renderbuffers, RenderbufferMap::destroy);
    MAP_GENOBJ(glGenRenderbuffer, m_renderbuffers, RenderbufferMap::generate);
    MAP_GENOBJ(glRenderbufferStorage, m_renderbuffers, RenderbufferMap::storage);

    MAP(glReadBuffer, ReadBuffer);
    MAP(glDrawBuffers, DrawBuffers);
}

void StateImpl::register_legacy_calls()
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
    MAP_GENOBJ(glBindProgram, m_legacy_programs,
               LegacyProgramStateMap::bind);
    MAP_GENOBJ(glProgramString, m_legacy_programs,
               LegacyProgramStateMap::program_string);

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

void StateImpl::register_ignore_history_calls()
{
    /* These functions are ignored outside required recordings
     * TODO: Delete calls should probably really delete things */
    const std::vector<const char *> ignore_history_calls = {
        "glCheckFramebufferStatus",
        "glDeleteBuffers",
        "glDeleteProgram",
        "glDeleteShader",
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
     };
    auto ignore_history_func = bind(&StateImpl::ignore_history, this, _1);
    update_call_table(ignore_history_calls, ignore_history_func);
}

void StateImpl::register_state_calls()
{
    /* These Functions change the state and we only need to remember the last
     * call before the target frame or an fbo creating a dependency is draw. */
    const std::vector<const char *> state_calls  = {
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
        "glXSwapBuffers",
    };

    auto state_call_func = bind(&StateImpl::record_state_call, this, _1);
    update_call_table(state_calls, state_call_func);

    /* These are state functions with an extra parameter */
    auto state_call_ex_func = bind(&StateImpl::record_state_call_ex, this, _1);
    const std::vector<const char *> state_calls_ex  = {
        "glClipPlane",
        "glColorMaskIndexedEXT",
        "glColorMaterial",
        "glDisableClientState",
        "glEnableClientState",
        "glLight",
        "glPixelStorei",
        "glPixelTransfer",
    };
    update_call_table(state_calls_ex, state_call_ex_func);

    MAP(glDisable, record_enable);
    MAP(glEnable, record_enable);  
    MAP(glMaterial, record_state_call_ex2);
    MAP(glTexEnv, record_state_call_ex2);
}

void StateImpl::register_required_calls()
{
    /* These function set up the context and are, therefore, required
     * TODO: figure out what is really required, and whether the can be
     * tracked like state variables. */
    auto required_func = bind(&StateImpl::record_required_call, this, _1);
    const std::vector<const char *> required_calls = {
        "glXChooseVisual",
        "glXCreateContext",
        "glXDestroyContext",
        "glXMakeCurrent",
        "glXChooseFBConfig",
        "glXQueryExtensionsString",
        "glXSwapIntervalMESA",
    };
    update_call_table(required_calls, required_func);
}

void StateImpl::update_call_table(const std::vector<const char*>& names,
                                  ft_callback cb)
{
    for (auto& i : names)
        m_call_table.insert(std::make_pair(i, cb));
}

#undef MAP

} // namespace frametrim
