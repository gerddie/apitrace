#include "ft_frametrimmer.hpp"
#include "ft_tracecall.hpp"
#include "ft_framebuffer.hpp"
#include "ft_objectstate.hpp"
#include "ft_matrixstate.hpp"

#include <unordered_set>
#include <algorithm>
#include <sstream>
#include <set>

namespace frametrim {

using std::bind;
using std::placeholders::_1;
using std::make_shared;


using ft_callback = std::function<void(const trace::Call&)>;

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

    std::vector<unsigned> get_sorted_call_ids() const;

    static unsigned equal_chars(const char *l, const char *r);

    void record_state_call(const trace::Call& call, unsigned no_param_sel);

    void register_state_calls();
    void register_legacy_calls();

    void record_enable(const trace::Call& call);

    void update_call_table(const std::vector<const char*>& names,
                           ft_callback cb);

    void Begin(const trace::Call& call);
    void End(const trace::Call& call);
    void Vertex(const trace::Call& call);
    void EndList(const trace::Call& call);
    void GenLists(const trace::Call& call);
    void NewList(const trace::Call& call);
    void CallList(const trace::Call& call);
    void DeleteLists(const trace::Call& call);

    void todo(const trace::Call& call);


    Framebuffer::Pointer m_current_draw_buffer;

    using CallTable = std::multimap<const char *, ft_callback, string_part_less>;
    CallTable m_call_table;

    CallSet m_required_calls;
    std::set<std::string> m_unhandled_calls;

    std::unordered_map<unsigned, ObjectState::Pointer> m_display_lists;
    ObjectState::Pointer m_active_display_list;

    std::map<std::string, PTraceCall> m_state_calls;
    std::map<unsigned, PTraceCall> m_enables;

    AllMatrisStates m_matrix_states;

    bool m_in_target_frame;
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



}

std::vector<unsigned>
FrameTrimmer::get_sorted_call_ids() const
{
    return impl->get_sorted_call_ids();
}



FrameTrimmeImpl::FrameTrimmeImpl()
{

}

void
FrameTrimmeImpl::call(const trace::Call& call, bool in_target_frame)
{
    const char *call_name = call.name();

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
        cb->second(call);
    } else {
        /* This should be some debug output only, because we might
         * not handle some calls deliberately */
        if (m_unhandled_calls.find(call_name) == m_unhandled_calls.end()) {
            std::cerr << "Call " << call.no
                      << " " << call_name << " not handled\n";
            m_unhandled_calls.insert(call_name);
        }
    }

    if (in_target_frame)
        m_required_calls.insert(trace2call(call));
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

void
FrameTrimmeImpl::record_state_call(const trace::Call& call,
                                   unsigned no_param_sel)
{
    std::stringstream s;
    s << call.name();
    for (unsigned i = 0; i < no_param_sel; ++i)
        s << "_" << call.arg(i).toUInt();

    m_state_calls[s.str()] = std::make_shared<TraceCall>(call, s.str());
}

void
FrameTrimmeImpl::record_enable(const trace::Call& call)
{
    unsigned value = call.arg(0).toUInt();
    m_enables[value] = trace2call(call);
}


#define MAP(name, call) m_call_table.insert(std::make_pair(#name, bind(&FrameTrimmeImpl:: call, this, _1)))
#define MAP_GENOBJ(name, obj, call) \
    m_call_table.insert(std::make_pair(#name, bind(&call, &obj, _1)))
#define MAP_GENOBJ_DATA(name, obj, call, data) \
    m_call_table.insert(std::make_pair(#name, bind(&call, &obj, _1, data)))
#define MAP_GENOBJ_DATAREF(name, obj, call, data) \
    m_call_table.insert(std::make_pair(#name, bind(&call, &obj, _1, std::ref(data))))


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


void FrameTrimmeImpl::update_call_table(const std::vector<const char*>& names,
                                        ft_callback cb)
{
    for (auto& i : names)
        m_call_table.insert(std::make_pair(i, cb));
}

void FrameTrimmeImpl::Begin(const trace::Call& call)
{
    if (m_active_display_list)
        m_active_display_list->append_call(trace2call(call));
}

void FrameTrimmeImpl::End(const trace::Call& call)
{
    if (m_active_display_list)
        m_active_display_list->append_call(trace2call(call));
}


void FrameTrimmeImpl::Vertex(const trace::Call& call)
{
    if (m_active_display_list)
        m_active_display_list->append_call(trace2call(call));
}

void FrameTrimmeImpl::EndList(const trace::Call& call)
{
    if (!m_in_target_frame)
        m_active_display_list->append_call(trace2call(call));

    m_active_display_list = nullptr;
}

void FrameTrimmeImpl::GenLists(const trace::Call& call)
{
    unsigned nlists = call.arg(0).toUInt();
    GLuint origResult = (*call.ret).toUInt();
    for (unsigned i = 0; i < nlists; ++i)
        m_display_lists[i + origResult] = std::make_shared<ObjectState>(i + origResult);

    if (!m_in_target_frame)
        m_display_lists[origResult]->append_call(trace2call(call));
}

void FrameTrimmeImpl::NewList(const trace::Call& call)
{
    assert(!m_active_display_list);
    auto list  = m_display_lists.find(call.arg(0).toUInt());
    assert(list != m_display_lists.end());
    m_active_display_list = list->second;

    if (!m_in_target_frame)
        m_active_display_list->append_call(trace2call(call));
}

void FrameTrimmeImpl::CallList(const trace::Call& call)
{
    auto list  = m_display_lists.find(call.arg(0).toUInt());
    assert(list != m_display_lists.end());

    if (m_in_target_frame)
        list->second->emit_calls_to_list(m_required_calls);

    /*
    auto draw_fbo = m_framebuffers.draw_fb();
    if (draw_fbo)
        list->second->emit_calls_to_list(draw_fbo->state_calls()); */
}

void FrameTrimmeImpl::DeleteLists(const trace::Call& call)
{
    GLint value = call.arg(0).toUInt();
    GLint value_end = call.arg(1).toUInt() + value;
    for(int i = value; i < value_end; ++i) {
        auto list  = m_display_lists.find(call.arg(0).toUInt());
        assert(list != m_display_lists.end());
        m_display_lists.erase(list);
    }
}

void FrameTrimmeImpl::todo(const trace::Call& call)
{
    std::cerr << "TODO: " << call.name() << "\n";
}

}
