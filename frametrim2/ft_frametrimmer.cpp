#include "ft_frametrimmer.hpp"
#include "ft_tracecall.hpp"
#include "ft_framebuffer.hpp"
#include "ft_matrixstate.hpp"
#include "ft_objectstate.hpp"
#include "ft_programstate.hpp"
#include "ft_texturestate.hpp"

#include <unordered_set>
#include <algorithm>
#include <functional>
#include <sstream>
#include <iostream>
#include <set>

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

    PTraceCall record_enable(const trace::Call& call);
    PTraceCall record_required_call(const trace::Call& call);

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

    PTraceCall todo(const trace::Call& call);

    FramebufferState::Pointer m_current_draw_buffer;

    using CallTable = std::multimap<const char *, ft_callback, string_part_less>;
    CallTable m_call_table;

    CallSet m_required_calls;
    std::set<std::string> m_unhandled_calls;

    std::unordered_map<unsigned, ObjectState::Pointer> m_display_lists;
    ObjectState::Pointer m_active_display_list;

    std::map<std::string, PTraceCall> m_state_calls;
    std::map<unsigned, PTraceCall> m_enables;

    AllMatrisStates m_matrix_states;
    LegacyProgramStateMap m_legacy_programs;
    TextureStateMap m_textures;
    FramebufferStateMap m_fbo;

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



FrameTrimmeImpl::FrameTrimmeImpl()
{
    register_state_calls();
    register_legacy_calls();
    register_required_calls();
    register_framebuffer_calls();
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
    m_recording_frame = true;

    for(auto& s : m_state_calls)
        m_required_calls.insert(s.second);

    for(auto& s : m_enables)
        m_required_calls.insert(s.second);

    m_matrix_states.emit_state_to_lists(m_required_calls);

    m_fbo.current_framebuffer().emit_calls_to_list(m_required_calls);



}

void FrameTrimmeImpl::end_target_frame()
{
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
#define MAP_GENOBJ(name, obj, call) \
    m_call_table.insert(std::make_pair(#name, bind(&call, &obj, _1)))
#define MAP_GENOBJ_DATA(name, obj, call, data) \
    m_call_table.insert(std::make_pair(#name, bind(&call, &obj, _1, data)))
#define MAP_GENOBJ_DATAREF(name, obj, call, data) \
    m_call_table.insert(std::make_pair(#name, bind(&call, &obj, _1, std::ref(data))))
#define MAP_GENOBJ_DATAREF_2(name, obj, call, data, param1, param2) \
    m_call_table.insert(std::make_pair(#name, bind(&call, &obj, _1, \
                        std::ref(data), param1, param2)))



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

void FrameTrimmeImpl::register_framebuffer_calls()
{
    /*MAP(glBindRenderbuffer, bind_renderbuffer);
    MAP_GENOBJ(glDeleteRenderbuffers, m_renderbuffers, RenderbufferObjectMap::destroy);
    MAP_GENOBJ(glGenRenderbuffer, m_renderbuffers, RenderbufferObjectMap::generate);
    MAP_GENOBJ(glRenderbufferStorage, m_renderbuffers, RenderbufferObjectMap::storage);
    */
    MAP_GENOBJ(glGenFramebuffer, m_fbo, FramebufferStateMap::generate);
    MAP_GENOBJ(glDeleteFramebuffers, m_fbo, FramebufferStateMap::destroy);
    MAP_GENOBJ(glBindFramebuffer, m_fbo, FramebufferStateMap::bind);
    MAP_GENOBJ(glViewport, m_fbo, FramebufferStateMap::viewport);

    //MAP_GENOBJ(glBlitFramebuffer, m_fbo, FramebufferStateMap::blit);
    MAP_GENOBJ_DATAREF_2(glFramebufferTexture, m_fbo,
                         FramebufferStateMap::attach_texture, m_textures,
                         2, 3);
    MAP_GENOBJ_DATAREF_2(glFramebufferTexture1D, m_fbo,
                         FramebufferStateMap::attach_texture, m_textures,
                         3, 4);
    MAP_GENOBJ_DATAREF_2(glFramebufferTexture2D, m_fbo,
                         FramebufferStateMap::attach_texture, m_textures,
                         3, 4);


/*    MAP_GENOBJ_DATAREF(glFramebufferTexture3D, m_fbo,
                         FramebufferStateMap::attach_texture3d, m_textures);

    MAP_GENOBJ_DATAREF(glFramebufferRenderbuffer, m_fbo,
                       FramebufferStateMap::attach_renderbuffer, m_renderbuffers); */

    /* MAP(glReadBuffer, ReadBuffer); */
}



void
FrameTrimmeImpl::register_state_calls()
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

    auto state_call_func = bind(&FrameTrimmeImpl::record_state_call, this, _1, 0);
    update_call_table(state_calls, state_call_func);

    /* These are state functions with an extra parameter */
    auto state_call_1_func = bind(&FrameTrimmeImpl::record_state_call, this, _1, 1);
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
        "glXChooseFBConfig",
        "glXQueryExtensionsString",
        "glXSwapIntervalMESA",
    };
    update_call_table(required_calls, required_func);
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
        m_display_lists[i + origResult] = std::make_shared<ObjectState>(i + origResult);

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

PTraceCall
FrameTrimmeImpl::todo(const trace::Call& call)
{
    std::cerr << "TODO: " << call.name() << "\n";
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
