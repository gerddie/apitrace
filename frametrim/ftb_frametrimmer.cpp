
#include "ftb_frametrimmer.hpp"
#include "ft_tracecall.hpp"
#include "ft_matrixstate.hpp"
#include "ft_displaylists.hpp"
#include "ftb_dependecyobject.hpp"


#include "trace_model.hpp"


#include <unordered_set>
#include <algorithm>
#include <functional>
#include <sstream>
#include <iostream>
#include <memory>
#include <set>

#include <GL/gl.h>
#include <GL/glext.h>

namespace frametrim {

using std::bind;
using std::placeholders::_1;
using std::make_shared;


using ft_callback = std::function<void(const trace::Call&)>;

struct string_part_less {
    bool operator () (const char *lhs, const char *rhs) const
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

    using ObjectMap = std::unordered_map<unsigned, UsedObject::Pointer>;

    FrameTrimmeImpl();
    void call(const trace::Call& call, Frametype frametype);
    void start_target_frame();
    void end_target_frame();

    std::vector<unsigned> get_sorted_call_ids();

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

    void record_required_call(const trace::Call& call);

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
    void Bind(const trace::Call& call, DependecyObjectMap& map, unsigned bind_param);
    void BindFbo(const trace::Call& call, DependecyObjectMap& map,
                       unsigned bind_param);
    void BindWithCreate(const trace::Call& call, DependecyObjectMap& map,
                        unsigned bind_param);
    void WaitSync(const trace::Call& call);
    void CallOnBoundObjWithDep(const trace::Call& call, DependecyObjectMap& map,
                               unsigned obj_id_param, DependecyObjectMap &dep_map, bool reverse_dep_too);

    void BindMultitex(const trace::Call& call);
    void DispatchCompute(const trace::Call& call);

    void todo(const trace::Call& call);
    void ignore_history(const trace::Call& call);

    bool skip_delete_obj(const trace::Call& call);
    bool skip_delete_impl(unsigned obj_id, DependecyObjectMap& map);
    void finalize();

    using CallTable = std::multimap<const char *, ft_callback, string_part_less>;
    CallTable m_call_table;

    CallSet m_required_calls;
    std::set<std::string> m_unhandled_calls;

    std::unordered_map<unsigned, DisplayLists::Pointer> m_display_lists;
    DisplayLists::Pointer m_active_display_list;

    AllMatrisStates m_matrix_states;

    using LegacyProgrambjectMap = DependecyObjectWithDefaultBindPointMap;
    using ProgramObjectMap = DependecyObjectWithSingleBindPointMap;
    using SamplerObjectMap = DependecyObjectWithDefaultBindPointMap;
    using SyncObjectMap = DependecyObjectWithSingleBindPointMap;
    using ShaderStateMap = DependecyObjectWithDefaultBindPointMap;
    using RenderObjectMap = DependecyObjectWithSingleBindPointMap;
    using VertexArrayMap = DependecyObjectWithDefaultBindPointMap;

    LegacyProgrambjectMap m_legacy_programs;
    ProgramObjectMap m_programs;
    TextureObjectMap m_textures;
    FramebufferObjectMap m_fbo;
    BufferObjectMap m_buffers;
    ShaderStateMap m_shaders;
    RenderObjectMap m_renderbuffers;
    SamplerObjectMap m_samplers;
    SyncObjectMap m_sync_objects;
    VertexArrayMap m_vertex_arrays;
    VertexAttribObjectMap m_vertex_attrib_pointers;

    std::unordered_map<GLint, UsedObject::Pointer> m_vertex_attr_pointer;
    std::unordered_map<GLint, PTraceCall> m_va_enables;
    std::unordered_map<GLint, PTraceCall> m_vertex_attribs;
    std::map<std::string, PTraceCall> m_state_calls;
    std::map<unsigned, PTraceCall> m_enables;

    std::unordered_map<GLint, bool> m_va_is_enabled;

    bool m_recording_frame;

    std::unordered_map<EStateCaches, PCallSet> m_state_caches;
    std::bitset<sc_last> m_state_caches_dirty;
    PTraceCall m_last_swap;
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
FrameTrimmer::call(const trace::Call& call, Frametype target_frame_type)
{
    impl->call(call, target_frame_type);
}

std::vector<unsigned>
FrameTrimmer::get_sorted_call_ids()
{
    return impl->get_sorted_call_ids();
}

void FrameTrimmer::finalize()
{
    impl->finalize();
}

FrameTrimmeImpl::FrameTrimmeImpl():
    m_required_calls(true),
    m_recording_frame(false),
    m_state_caches(sc_last)
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
FrameTrimmeImpl::call(const trace::Call& call, Frametype frametype)
{
    const char *call_name = call.name();

    if (!m_recording_frame && (frametype != ft_none))
        start_target_frame();
    if (m_recording_frame && (frametype == ft_none))
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

        //  std::cerr << "Handle " << call.name() << " as " << cb->first << "\n";
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

    auto c = trace2call(call);

    if (frametype == ft_none) {
        if (call.flags & trace::CALL_FLAG_END_FRAME)
            m_last_swap = c;
    } else {
        /* Skip delete calls for objects that have never been emitted */
        if (skip_delete_obj(call))
            return;

        if (!(call.flags & trace::CALL_FLAG_END_FRAME)) {
            m_required_calls.insert(c);
        } else {
            if (frametype == ft_retain_frame) {
                m_required_calls.insert(c);
                if (m_last_swap) {
                    m_required_calls.insert(m_last_swap);
                    m_last_swap = nullptr;
                }
            } else
                m_last_swap = c;
        }
    }
}

void FrameTrimmeImpl::start_target_frame()
{
    std::cerr << "Start recording\n";

    m_recording_frame = true;

    m_matrix_states.emit_state_to_lists(m_required_calls);

    for (auto&& [name, call] : m_state_calls)
        m_required_calls.insert(call);

    for (auto&& [id, call]: m_enables)
        m_required_calls.insert(call);
}

bool
FrameTrimmeImpl::skip_delete_obj(const trace::Call& call)
{
    if (!strcmp(call.name(), "glDeleteProgram"))
        return skip_delete_impl(call.arg(0).toUInt(), m_programs);

    if (!strcmp(call.name(), "glDeleteSync"))
        return skip_delete_impl(call.arg(0).toUInt(), m_sync_objects);

    DependecyObjectMap *map = nullptr;
    if (!strcmp(call.name(), "glDeleteBuffers"))
        map = &m_buffers;

    if (!strcmp(call.name(), "glDeleteTextures"))
        map = &m_textures;

    if (!strcmp(call.name(), "glDeleteFramebuffers"))
        map = &m_fbo;

    if (!strcmp(call.name(), "glDeleteRenderbuffers"))
        map = &m_renderbuffers;

    if (map) {
        const auto ids = (call.arg(1)).toArray();
        for (auto& v : ids->values) {
            if (!skip_delete_impl(v->toUInt(), *map))
                return false;
        }
        return true;
    }
    return false;
}

bool
FrameTrimmeImpl::skip_delete_impl(unsigned obj_id,
                                  DependecyObjectMap& map)
{
    auto obj = map.get_by_id(obj_id);
    return !obj->emitted();
}

void FrameTrimmeImpl::finalize()
{
    m_programs.emit_bound_objects(m_required_calls);
    m_textures.emit_bound_objects(m_required_calls);
    m_fbo.emit_bound_objects(m_required_calls);
    m_buffers.emit_bound_objects(m_required_calls);
    m_shaders.emit_bound_objects(m_required_calls);
    m_renderbuffers.emit_bound_objects(m_required_calls);
    m_samplers.emit_bound_objects(m_required_calls);
    m_sync_objects.emit_bound_objects(m_required_calls);
    m_vertex_arrays.emit_bound_objects(m_required_calls);
    m_vertex_attrib_pointers.emit_bound_objects(m_required_calls);
    m_legacy_programs.emit_bound_objects(m_required_calls);

    if (m_last_swap)
        m_required_calls.insert(m_last_swap);
}

void FrameTrimmeImpl::end_target_frame()
{
    std::cerr << "End recording\n";
    m_recording_frame = false;
}

std::vector<unsigned>
FrameTrimmeImpl::get_sorted_call_ids()
{
    std::unordered_set<unsigned> make_sure_its_singular;

    m_required_calls.deep_resolve();
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

    m_state_caches_dirty.set(sc_states);

    return c;
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

#define MAP_GENOBJ_DATAREF2(name, obj, call, data1, data2) \
    m_call_table.insert(std::make_pair(#name, bind(&call, &obj, _1, std::ref(data1), \
                        std::ref(data2))))

#define MAP_GENOBJ_RDD(name, obj, call, data, param1, param2) \
    m_call_table.insert(std::make_pair(#name, bind(&call, &obj, _1, \
                        std::ref(data), param1, param2)))

#define MAP_GENOBJ_RD(name, obj, call, data, param) \
    m_call_table.insert(std::make_pair(#name, bind(&call, &obj, _1, \
                        std::ref(data), param)))

#define MAP_GENOBJ_RRD(name, obj, call, data1, data2, param) \
    m_call_table.insert(std::make_pair(#name, bind(&call, &obj, _1, \
                        std::ref(data1), std::ref(data2), param)))



#define MAP_GENOBJ_RDRR(name, obj, call, data1, param, data2, data3) \
    m_call_table.insert(std::make_pair(#name, bind(&call, &obj, _1, \
                        std::ref(data1), param, std::ref(data2), std::ref(data3))))

#define MAP_GENOBJ_RRR(name, obj, call, data1, data2, data3) \
    m_call_table.insert(std::make_pair(#name, bind(&call, &obj, _1, \
                        std::ref(data1), std::ref(data2), std::ref(data3))))


#define MAP_GENOBJ_DATA_DATAREF(name, obj, call, data, dataref) \
    m_call_table.insert(std::make_pair(#name, bind(&call, &obj, _1, data, std::ref(dataref))))

#define MAP_DATAREF_DATA(name, call, data, param1) \
    m_call_table.insert(std::make_pair(#name, bind(&FrameTrimmeImpl:: call, this, _1, \
                        std::ref(data), param1)))

#define MAP_DATAREF_DATA_DATAREF(name, call, data1, param1, data2) \
    m_call_table.insert(std::make_pair(#name, bind(&FrameTrimmeImpl:: call, this, _1, \
                        std::ref(data1), param1, std::ref(data2))))

#define MAP_RDRD(name, call, data1, param1, data2, param2) \
    m_call_table.insert(std::make_pair(#name, bind(&FrameTrimmeImpl:: call, this, _1, \
                        std::ref(data1), param1, std::ref(data2), param2)))

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
               DependecyObjectWithDefaultBindPointMap::Generate);
    MAP_DATAREF_DATA(glBindProgram, BindWithCreate, m_legacy_programs, 1);

    MAP_GENOBJ(glDeletePrograms, m_legacy_programs,
               LegacyProgrambjectMap::Destroy);

    MAP_GENOBJ(glProgramString, m_legacy_programs,
               LegacyProgrambjectMap::CallOnBoundObject);
    MAP_GENOBJ(glProgramLocalParameter, m_legacy_programs,
               LegacyProgrambjectMap::CallOnBoundObject);
    MAP_GENOBJ(glProgramEnvParameter, m_legacy_programs,
               LegacyProgrambjectMap::CallOnBoundObject);

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
    MAP(glDispatchCompute, DispatchCompute);
}

void FrameTrimmeImpl::register_framebuffer_calls()
{
    MAP_DATAREF_DATA(glBindRenderbuffer, Bind, m_renderbuffers, 1);
    MAP_GENOBJ(glDeleteRenderbuffers, m_renderbuffers, DependecyObjectWithSingleBindPointMap::Destroy);
    MAP_GENOBJ(glGenRenderbuffer, m_renderbuffers, DependecyObjectWithSingleBindPointMap::Generate);
    MAP_GENOBJ(glRenderbufferStorage, m_renderbuffers, DependecyObjectWithSingleBindPointMap::CallOnBoundObject);

    MAP_GENOBJ(glGenFramebuffer, m_fbo, FramebufferObjectMap::Generate);
    MAP_GENOBJ(glDeleteFramebuffers, m_fbo, FramebufferObjectMap::Destroy);
    MAP_DATAREF_DATA(glBindFramebuffer, BindFbo, m_fbo, 1);
    MAP_GENOBJ_DATA(glViewport, m_fbo, FramebufferObjectMap::CallOnObjectBoundTo, GL_DRAW_FRAMEBUFFER);

    MAP_GENOBJ(glBlitFramebuffer, m_fbo, FramebufferObjectMap::Blit);
    MAP_RDRD(glFramebufferTexture, CallOnBoundObjWithDep, m_fbo, 2, m_textures, true);
    MAP_RDRD(glFramebufferTexture1D, CallOnBoundObjWithDep, m_fbo,
                            3, m_textures, true);
    MAP_RDRD(glFramebufferTexture2D, CallOnBoundObjWithDep, m_fbo,
                            3, m_textures, true);

    MAP_GENOBJ(glReadBuffer, m_fbo, FramebufferObjectMap::ReadBuffer);
    MAP_GENOBJ_DATA(glDrawBuffer, m_fbo, FramebufferObjectMap::CallOnObjectBoundTo, GL_DRAW_FRAMEBUFFER);

    MAP_GENOBJ_DATA(glClearBuffer, m_fbo, FramebufferObjectMap::CallOnObjectBoundTo, GL_DRAW_FRAMEBUFFER);
    MAP_GENOBJ_DATA(glClearBufferfi, m_fbo, FramebufferObjectMap::CallOnObjectBoundTo, GL_DRAW_FRAMEBUFFER);
    MAP_GENOBJ_DATA(glClearBufferfv, m_fbo, FramebufferObjectMap::CallOnObjectBoundTo, GL_DRAW_FRAMEBUFFER);
    MAP_GENOBJ_DATA(glClearBufferiv, m_fbo, FramebufferObjectMap::CallOnObjectBoundTo, GL_DRAW_FRAMEBUFFER);

/*    MAP_GENOBJ_DATAREF(glFramebufferTexture3D, m_fbo,
                         FramebufferStateMap::attach_texture3d, m_textures);
      MAP(glReadBuffer, ReadBuffer); */

    MAP_RDRD(glFramebufferRenderbuffer, CallOnBoundObjWithDep,
             m_fbo, 3, m_renderbuffers, true);

    MAP_GENOBJ_DATA(glClear, m_fbo, FramebufferObjectMap::CallOnObjectBoundTo, GL_DRAW_FRAMEBUFFER);

}

void
FrameTrimmeImpl::register_buffer_calls()
{
    MAP_GENOBJ(glGenBuffers, m_buffers, BufferObjectMap::Generate);
    MAP_GENOBJ(glDeleteBuffers, m_buffers, BufferObjectMap::Destroy);

    MAP_DATAREF_DATA(glBindBuffer, Bind, m_buffers, 1);
    MAP_DATAREF_DATA(glBindBufferRange, Bind, m_buffers, 2);

    /* This will need a bit more to be handled correctly */
    MAP_DATAREF_DATA(glBindBufferBase, Bind, m_buffers, 2);

    MAP_GENOBJ(glBufferData, m_buffers, BufferObjectMap::data);
    MAP_GENOBJ(glBufferSubData, m_buffers, BufferObjectMap::CallOnBoundObject);

    MAP_GENOBJ(glMapBuffer, m_buffers, BufferObjectMap::map);
    MAP_GENOBJ(glMapBufferRange, m_buffers, BufferObjectMap::map_range);
    MAP_GENOBJ(memcpy, m_buffers, BufferObjectMap::memcopy);
    MAP_GENOBJ(glFlushMappedBufferRange, m_buffers, BufferObjectMap::CallOnBoundObject);
    MAP_GENOBJ(glUnmapBuffer, m_buffers, BufferObjectMap::unmap);
    MAP_GENOBJ(glClearBufferData, m_buffers, BufferObjectMap::CallOnBoundObject);
    MAP_GENOBJ(glInvalidateBufferData, m_buffers, BufferObjectMap::CallOnNamedObject);
}

void FrameTrimmeImpl::register_draw_calls()
{
    MAP_GENOBJ_DATA(glDrawArrays, m_fbo,
                    FramebufferObjectMap::CallOnObjectBoundTo, GL_DRAW_FRAMEBUFFER);
    MAP_GENOBJ_DATAREF(glDrawElements, m_fbo,
                       FramebufferObjectMap::DrawFromBuffer, m_buffers);
    MAP_GENOBJ_DATAREF(glDrawRangeElements, m_fbo,
                       FramebufferObjectMap::DrawFromBuffer, m_buffers);
    MAP_GENOBJ_DATAREF(glDrawRangeElementsBaseVertex, m_fbo,
                       FramebufferObjectMap::DrawFromBuffer, m_buffers);
}

void
FrameTrimmeImpl::register_program_calls()
{
    MAP_GENOBJ_RDD(glAttachObject, m_programs,
                   ProgramObjectMap::CallOnNamedObjectWithDep, m_shaders, 1, false);
    MAP_GENOBJ_RDD(glAttachShader, m_programs,
                   ProgramObjectMap::CallOnNamedObjectWithDep, m_shaders, 1, false);

    MAP_GENOBJ(glCompileShader, m_shaders, ShaderStateMap::CallOnNamedObject);
    MAP_GENOBJ(glCreateShader, m_shaders, ShaderStateMap::Create);
    MAP_GENOBJ(glDeleteShader, m_shaders, ShaderStateMap::Delete);
    MAP_GENOBJ(glShaderSource, m_shaders, ShaderStateMap::CallOnNamedObject);

    MAP_GENOBJ(glBindAttribLocation, m_programs, ProgramObjectMap::CallOnNamedObject);
    MAP_GENOBJ(glCreateProgram, m_programs, ProgramObjectMap::Create);
    MAP_GENOBJ(glDeleteProgram, m_programs, ProgramObjectMap::Delete);

    MAP_GENOBJ(glGetAttribLocation, m_programs, ProgramObjectMap::CallOnNamedObject);
    MAP_GENOBJ(glGetUniformLocation, m_programs, ProgramObjectMap::CallOnNamedObject);
    MAP_GENOBJ(glBindFragDataLocation, m_programs, ProgramObjectMap::CallOnNamedObject);
    MAP_GENOBJ(glLinkProgram, m_programs, ProgramObjectMap::CallOnNamedObject);
    MAP_GENOBJ(glProgramBinary, m_programs, ProgramObjectMap::CallOnNamedObject);

    MAP_GENOBJ(glUniform, m_programs, ProgramObjectMap::CallOnBoundObject);
    MAP_DATAREF_DATA(glUseProgram, Bind, m_programs, 0);
    MAP_GENOBJ(glProgramParameter, m_programs, ProgramObjectMap::CallOnNamedObject);
    MAP_GENOBJ(glShaderStorageBlockBinding, m_programs, ProgramObjectMap::CallOnNamedObject);
}

void FrameTrimmeImpl::register_texture_calls()
{
    MAP_GENOBJ(glGenTextures, m_textures, TextureObjectMap::Generate);
    MAP_GENOBJ(glDeleteTextures, m_textures, TextureObjectMap::Destroy);

    MAP_GENOBJ(glActiveTexture, m_textures, TextureObjectMap::ActiveTexture);
    MAP_GENOBJ(glClientActiveTexture, m_textures, TextureObjectMap::ActiveTexture);
    MAP_DATAREF_DATA(glBindTexture, Bind, m_textures, 1);
    MAP(glBindMultiTexture, BindMultitex);

    MAP_GENOBJ(glCompressedTexImage2D, m_textures, TextureObjectMap::CallOnBoundObject);
    MAP_GENOBJ(glGenerateMipmap, m_textures, TextureObjectMap::CallOnBoundObject);
    MAP_GENOBJ(glTexImage1D, m_textures, TextureObjectMap::CallOnBoundObject);
    MAP_GENOBJ(glTexImage2D, m_textures, TextureObjectMap::CallOnBoundObject);
    MAP_GENOBJ(glTexStorage1D, m_textures, TextureObjectMap::CallOnBoundObject);
    MAP_GENOBJ(glTexStorage2D, m_textures, TextureObjectMap::CallOnBoundObject);
    MAP_GENOBJ(glTexStorage3D, m_textures, TextureObjectMap::CallOnBoundObject);
    MAP_GENOBJ(glTexImage3D, m_textures, TextureObjectMap::CallOnBoundObject);
    MAP_GENOBJ_RD(glTexSubImage1D, m_textures, TextureObjectMap::CallOnBoundObjectWithDepBoundTo,
               m_buffers, GL_PIXEL_UNPACK_BUFFER);
    MAP_GENOBJ_RD(glTexSubImage2D, m_textures, TextureObjectMap::CallOnBoundObjectWithDepBoundTo,
                    m_buffers, GL_PIXEL_UNPACK_BUFFER);
    MAP_GENOBJ(glCompressedTexSubImage2D, m_textures, TextureObjectMap::CallOnBoundObject);
    MAP_GENOBJ(glTexSubImage3D, m_textures, TextureObjectMap::CallOnBoundObject);
    MAP_GENOBJ(glTexParameter, m_textures, TextureObjectMap::CallOnBoundObject);
    MAP_GENOBJ_RDD(glTextureView, m_textures, TextureObjectMap::CallOnNamedObjectWithDep,
                   m_textures, 2, true);

    MAP_GENOBJ_RDD(glTexBuffer, m_textures, TextureObjectMap::CallOnBoundObjectWithDep,
                   m_buffers, 2, true);

    /* Should add a dependency on the read fbo */
    MAP_GENOBJ(glCopyTexSubImage, m_textures, TextureObjectMap::CallOnBoundObject);

    MAP_GENOBJ(glCopyTexImage2D, m_textures, TextureObjectMap::CallOnBoundObject);

    MAP_GENOBJ(glCopyImageSubData, m_textures, TextureObjectMap::Copy);
    MAP_GENOBJ(glBindImageTexture, m_textures, TextureObjectMap::BindToImageUnit);

    /*
    MAP_GENOBJ(glCopyTexSubImage2D, m_textures, TextureStateMap::copy_sub_data);
    */
    MAP_DATAREF_DATA(glBindSampler, Bind, m_samplers, 1);
    MAP_GENOBJ(glGenSamplers, m_samplers, SamplerObjectMap::Generate);
    MAP_GENOBJ(glDeleteSamplers, m_samplers, SamplerObjectMap::Destroy);
    MAP_GENOBJ(glSamplerParameter, m_samplers, SamplerObjectMap::CallOnNamedObject);
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
        "glColorMask",
        "glColorPointer",
        "glCullFace",
        "glDepthFunc",
        "glDepthMask",
        "glDepthRange",
        "glDepthBounds",
        "glFlush",
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
        "glProvokingVertex",
        "glPrimitiveBoundingBox",
        "glSampleCoverage",
        "glShadeModel",
        "glScissor",
        "glStencilMask",
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
        "glPixelTransfer",
        "glStencilOpSeparate",
        "glStencilFuncSeparate",
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

    MAP(glDisable, record_required_call);
    MAP(glEnable, record_required_call);
    MAP(glEnablei, record_required_call);

    MAP_GENOBJ(glFenceSync, m_sync_objects, SyncObjectMap::Create);
    MAP(glWaitSync, WaitSync);
    MAP(glClientWaitSync, WaitSync);
    MAP_GENOBJ(glDeleteSync, m_sync_objects, SyncObjectMap::CallOnNamedObject);
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
        "eglMakeCurrent",

        "glPixelStorei", /* Being lazy here, we could track the dependency
                            in the relevant calls */
    };
    update_call_table(required_calls, required_func);
}

void FrameTrimmeImpl::register_ignore_history_calls()
{
    /* These functions are ignored outside required recordings
     * TODO: Delete calls should probably really delete things */
    const std::vector<const char *> ignore_history_calls = {
        "glCheckFramebufferStatus",
        "glGetActiveUniform",
        "glGetBoolean",
        "glGetError",
        "glGetFloat",
        "glGetFramebufferAttachmentParameter",
        "glGetInfoLog",
        "glGetInteger",
        "glGetObjectLabelEXT",
        "glGetObjectParameter",
        "glGetProgram",
        "glGetShader",
        "glGetString",
        "glGetTexLevelParameter",
        "glGetTexParameter",
        "glGetTexImage",
        "glGetUniform",
        "glLabelObjectEXT",
        "glIsEnabled",
        "glIsVertexArray",
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
    MAP_GENOBJ(glGenVertexArrays, m_vertex_arrays, VertexArrayMap::Generate);
    MAP_GENOBJ(glDeleteVertexArrays, m_vertex_arrays, VertexArrayMap::Destroy);
    MAP_DATAREF_DATA(glBindVertexArray, Bind, m_vertex_arrays, 0);

    MAP(glDisableVertexAttribArray, record_required_call);
    MAP(glEnableVertexAttribArray, record_required_call);
    MAP_GENOBJ_DATAREF(glVertexAttribPointer, m_vertex_attrib_pointers,
                       VertexAttribObjectMap::BindAVO, m_buffers);

    MAP_GENOBJ_RRR(glBindVertexBuffer, m_vertex_attrib_pointers,
                   VertexAttribObjectMap::BindVAOBuf, m_buffers,
                   m_required_calls, m_recording_frame);

    MAP(glVertexPointer, record_required_call);
    MAP(glTexCoordPointer, record_required_call);
    MAP(glVertexAttrib1, record_required_call);
    MAP(glVertexAttrib2, record_required_call);
    MAP(glVertexAttrib3, record_required_call);
    MAP(glVertexAttrib4, record_required_call);
    MAP(glVertexAttribI, record_required_call);
    MAP(glVertexAttribL, record_required_call);
    MAP(glVertexAttribP1, record_required_call);
    MAP(glVertexAttribP2, record_required_call);
    MAP(glVertexAttribP3, record_required_call);
    MAP(glVertexAttribP4, record_required_call);
    MAP(glTexGen, record_required_call);

    MAP(glDisableClientState, record_required_call);
    MAP(glEnableClientState, record_required_call);

}

void
FrameTrimmeImpl::update_call_table(const std::vector<const char*>& names,
                                        ft_callback cb)
{
    for (auto& i : names)
        m_call_table.insert(std::make_pair(i, cb));
}

void
FrameTrimmeImpl::Begin(const trace::Call& call)
{
    if (m_active_display_list)
        m_active_display_list->append_call(trace2call(call));
}

void
FrameTrimmeImpl::End(const trace::Call& call)
{
    if (m_active_display_list)
        m_active_display_list->append_call(trace2call(call));
}

void
FrameTrimmeImpl::Vertex(const trace::Call& call)
{
    if (m_active_display_list)
        m_active_display_list->append_call(trace2call(call));
}

void
FrameTrimmeImpl::EndList(const trace::Call& call)
{
    if (!m_recording_frame)
        m_active_display_list->append_call(trace2call(call));

    m_active_display_list = nullptr;
}

void
FrameTrimmeImpl::GenLists(const trace::Call& call)
{
    unsigned nlists = call.arg(0).toUInt();
    GLuint origResult = (*call.ret).toUInt();
    for (unsigned i = 0; i < nlists; ++i)
        m_display_lists[i + origResult] = std::make_shared<DisplayLists>(i + origResult);

    auto c = trace2call(call);
    if (!m_recording_frame)
        m_display_lists[origResult]->append_call(c);
}

void
FrameTrimmeImpl::NewList(const trace::Call& call)
{
    assert(!m_active_display_list);
    auto list  = m_display_lists.find(call.arg(0).toUInt());
    assert(list != m_display_lists.end());
    m_active_display_list = list->second;

    if (!m_recording_frame)
        m_active_display_list->append_call(trace2call(call));
}

void
FrameTrimmeImpl::CallList(const trace::Call& call)
{
    auto list  = m_display_lists.find(call.arg(0).toUInt());
    assert(list != m_display_lists.end());

    if (m_recording_frame)
        list->second->emit_calls_to_list(m_required_calls);
}

void
FrameTrimmeImpl::DeleteLists(const trace::Call& call)
{
    GLint value = call.arg(0).toUInt();
    GLint value_end = call.arg(1).toUInt() + value;
    for(int i = value; i < value_end; ++i) {
        auto list  = m_display_lists.find(call.arg(0).toUInt());
        assert(list != m_display_lists.end());
        list->second->append_call(trace2call(call));
        m_display_lists.erase(list);
    }
}

void
FrameTrimmeImpl::Bind(const trace::Call& call, DependecyObjectMap& map,
                      unsigned bind_param)
{
    auto bound_obj = map.Bind(call, bind_param);
    if (bound_obj)
        bound_obj->add_call(trace2call(call));
    else
        map.add_call(trace2call(call));

    if (m_recording_frame && bound_obj)
        bound_obj->emit_calls_to(m_required_calls);
}

void
FrameTrimmeImpl::BindWithCreate(const trace::Call& call, DependecyObjectMap& map,
                                unsigned bind_param)
{
    auto bound_obj = map.BindWithCreate(call, bind_param);
    if (bound_obj)
        bound_obj->add_call(trace2call(call));
    else
        map.add_call(trace2call(call));
    if (m_recording_frame && bound_obj)
        bound_obj->emit_calls_to(m_required_calls);
}

void FrameTrimmeImpl::BindMultitex(const trace::Call& call)
{
    auto tex = m_textures.BindMultitex(call);
    if (tex)
        tex->add_call(trace2call(call));
    else
        m_textures.add_call(trace2call(call));

    if (m_recording_frame && tex)
        tex->emit_calls_to(m_required_calls);
}

void
FrameTrimmeImpl::WaitSync(const trace::Call& call)
{
    auto obj = m_sync_objects.get_by_id(call.arg(0).toUInt());
    assert(obj);
    obj->add_call(trace2call(call));
    if (m_recording_frame)
        obj->emit_calls_to(m_required_calls);
}

void
FrameTrimmeImpl::BindFbo(const trace::Call& call, DependecyObjectMap& map,
                         unsigned bind_param)
{
    auto bound_obj = map.Bind(call, bind_param);
    bound_obj->add_call(trace2call(call));
    if (m_recording_frame && bound_obj->id())
        bound_obj->emit_calls_to(m_required_calls);
}

void
FrameTrimmeImpl::CallOnBoundObjWithDep(const trace::Call& call, DependecyObjectMap& map,
                                        unsigned obj_id_param,
                                        DependecyObjectMap& dep_map, bool reverse_dep_too)
{
    auto dep_obj = map.CallOnBoundObjectWithDep(call, dep_map, obj_id_param,
                                                reverse_dep_too);
    if (m_recording_frame && dep_obj)
        dep_obj->emit_calls_to(m_required_calls);
}

void FrameTrimmeImpl::DispatchCompute(const trace::Call& call)
{
    auto cur_prog = m_programs.bound_to(0);
    assert(cur_prog);

    cur_prog->add_call(trace2call(call));
    // Without knowning how the compute program is actually written
    // we have to assume that any bound ssbo and any bound image can be
    // changed by the program, so the program has to depend on all bound
    // ssbos and images, ane they in turn must depend on the program

    m_buffers.add_ssbo_dependencies(cur_prog);
    m_textures.add_image_dependencies(cur_prog);
}

void
FrameTrimmeImpl::todo(const trace::Call& call)
{
    std::cerr << "TODO: " << call.name() << "\n";
}

void
FrameTrimmeImpl::ignore_history(const trace::Call& call)
{
    (void)call;
}

void
FrameTrimmeImpl::record_required_call(const trace::Call& call)
{
    auto c = trace2call(call);
    m_required_calls.insert(c);
}

}
