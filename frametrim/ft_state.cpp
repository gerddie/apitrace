#include "ft_state.hpp"
#include "ft_bufferstate.hpp"
#include "ft_matrixstate.hpp"
#include "ft_programstate.hpp"
#include "ft_texturestate.hpp"

#include "trace_writer.hpp"


#include <unordered_set>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <stack>
#include <set>
#include <sstream>

#include <GL/glext.h>

namespace frametrim {

using std::bind;
using std::placeholders::_1;
using std::make_shared;

struct string_part_less {
   bool operator () (const char *lhs, const char *rhs)
   {
      int len = std::min(strlen(lhs), strlen(rhs));
      return strncmp(lhs, rhs, len) < 0;
   }
};


struct StateImpl {

   StateImpl();

   void call(PCall call);
   void register_callbacks();

   /* OpenGL calls */
   void ActiveTexture(PCall call);
   void AttachShader(PCall call);
   void Begin(PCall call);
   void BindAttribLocation(PCall call);
   void BindBuffer(PCall call);
   void BindTexture(PCall call);
   void BufferData(PCall call);
   void BindProgram(PCall call);
   void CallList(PCall call);
   void CreateShader(PCall call);
   void CreateProgram(PCall call);
   void DeleteLists(PCall call);
   void DrawElements(PCall call);
   void End(PCall call);
   void EndList(PCall call);
   void GenBuffers(PCall call);
   void GenLists(PCall call);
   void GenTextures(PCall call);
   void GenVertexArrays(PCall call);
   void Light(PCall call);
   void program_call(PCall call);
   void LoadIdentity(PCall call);
   void Material(PCall call);
   void MatrixMode(PCall call);
   void NewList(PCall call);
   void Normal(PCall call);
   void PopMatrix(PCall call);
   void PushMatrix(PCall call);
   void Rotate(PCall call);
   void ShadeModel(PCall call);
   void shader_call(PCall call);
   void Translate(PCall call);
   void Uniform(PCall call);
   void UseProgram(PCall call);
   void Vertex(PCall call);
   void VertexAttribPointer(PCall call);

   void history_ignore(PCall call);
   void todo(PCall call);


   void record_enable(PCall call);
   void record_va_enables(PCall call);

   void record_state_call(PCall call);
   void record_state_call_ex(PCall call);
   void record_required_call(PCall call);
   void write(trace::Writer& writer);
   void start_target_farme();
   void texture_call(PCall call);

   using CallTable = std::multimap<const char *, ft_callback, string_part_less>;
   CallTable m_call_table;

   std::unordered_map<GLint, PShaderState> m_shaders;
   std::unordered_map<GLint, PProgramState> m_programs;
   PProgramState m_active_program;

   std::unordered_map<GLenum, PCall> m_enables;
   std::unordered_map<GLint, PCall> m_va_enables;


   std::unordered_map<GLint, PObjectState> m_display_lists;
   PObjectState m_active_display_list;

   std::unordered_map<GLint, PBufferState> m_buffers;
   std::unordered_map<GLint, PBufferState> m_bound_buffers;

   // TODO: multitexture support
   std::unordered_map<GLint, PTextureState> m_textures;
   std::unordered_map<GLint, PTextureState> m_bound_texture;
   unsigned m_active_texture_unit;
   PCall m_active_texture_unit_call;

   std::unordered_map<GLint, PObjectState> m_vertex_arrays;
   std::unordered_map<GLint, PCall> m_vertex_attr_pointer;

   std::unordered_map<uint64_t, PCall> m_last_lights;

   std::unordered_map<std::string, PCall> m_state_calls;

   bool m_in_target_frame;

   CallSet m_required_calls;

   std::stack<PMatrixState> m_mv_matrix;
   std::stack<PMatrixState> m_proj_matrix;
   std::stack<PMatrixState> m_texture_matrix;
   std::stack<PMatrixState> m_color_matrix;

   PMatrixState m_current_matrix;
   std::stack<PMatrixState> *m_current_matrix_stack;
};


void State::call(PCall call)
{
   impl->call(call);
}

void State::write(trace::Writer& writer)
{
   impl->write(writer);
}

State::State()
{
   impl = new StateImpl;

   impl->register_callbacks();
}

State::~State()
{
   delete impl;
}

void State::target_frame_started()
{
   if (impl->m_in_target_frame)
      return;

   impl->m_in_target_frame = true;
   impl->start_target_farme();
}

void StateImpl::start_target_farme()
{
   for(auto& a : m_state_calls)
      record_required_call(a.second);

   for(auto& cap: m_enables)
      record_required_call(cap.second);

   for (auto& l: m_last_lights)
      record_required_call(l.second);

   if (!m_mv_matrix.empty())
      m_mv_matrix.top()->append_calls_to(m_required_calls);

   if (!m_proj_matrix.empty())
      m_proj_matrix.top()->append_calls_to(m_required_calls);

   if (!m_texture_matrix.empty())
      m_texture_matrix.top()->append_calls_to(m_required_calls);

   if (!m_color_matrix.empty())
      m_color_matrix.top()->append_calls_to(m_required_calls);

   /* Set vertex attribute array pointers only if they are enabled */
   for(auto& va : m_vertex_attr_pointer) {
      auto vae = m_va_enables.find(va.first);
      if (vae != m_va_enables.end() &&
          !strcmp(vae->second->name(), "glEnableVertexAttribArray")) {
         m_required_calls.insert(va.second);
         m_required_calls.insert(vae->second);
      }
   }

   for (auto& va: m_vertex_arrays)
      va.second->append_calls_to(m_required_calls);

   for (auto& buf: m_bound_buffers)
      buf.second->append_calls_to(m_required_calls);

   for (auto& tex: m_bound_texture)
      tex.second->append_calls_to(m_required_calls);

   if (m_active_program)
      m_active_program->append_calls_to(m_required_calls);
}

StateImpl::StateImpl():
   m_active_texture_unit(0),
   m_in_target_frame(false)
{
   m_mv_matrix.push(make_shared<MatrixState>(nullptr));
   m_current_matrix = m_mv_matrix.top();
   m_current_matrix_stack = &m_mv_matrix;
}

unsigned equal_chars(const char *l, const char *r)
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
      std::cerr << call->name() << " unhandled\n";
   }

   if (m_in_target_frame)
      m_required_calls.insert(call);
}

void StateImpl::ActiveTexture(PCall call)
{
   m_active_texture_unit = call->arg(0).toUInt();
   m_active_texture_unit_call = call;
}

void StateImpl::AttachShader(PCall call)
{
   auto program = m_programs[call->arg(0).toUInt()];
   assert(program);

   auto shader = m_shaders[call->arg(1).toUInt()];
   assert(shader);

   program->attach_shader(shader);
   program->append_call(call);
}

void StateImpl::Begin(PCall call)
{
   if (m_active_display_list)
      m_active_display_list->append_call(call);
}

void StateImpl::BindAttribLocation(PCall call)
{
   GLint program_id = call->arg(0).toSInt();
   auto prog = m_programs.find(program_id);
   assert(prog != m_programs.end());
   prog->second->append_call(call);
}

void StateImpl::BindBuffer(PCall call)
{
   unsigned target = call->arg(0).toUInt();
   unsigned id = call->arg(1).toUInt();

   if (id) {
      if (!m_bound_buffers[target] ||
          m_bound_buffers[target]->id() != id) {
         m_bound_buffers[target] = m_buffers[id];
         m_bound_buffers[target] ->bind(call);
      }
   } else {
      m_bound_buffers.erase(target);
   }
}

void StateImpl::BindTexture(PCall call)
{
   unsigned target = call->arg(0).toUInt();
   unsigned id = call->arg(1).toUInt();
   unsigned target_unit = (target << 16) | m_active_texture_unit;

   if (id) {
      auto tex = m_textures[id];
      if (!m_bound_texture[target_unit] ||
          m_bound_texture[target_unit]->id() != id) {
         m_bound_texture[target_unit] = tex;
         tex->bind_unit(call, m_active_texture_unit_call);

         if (m_in_target_frame) {
            tex->append_calls_to(m_required_calls);
         }
      }
   } else
      m_bound_texture.erase(target_unit);
}

void StateImpl::BufferData(PCall call)
{
   unsigned target = call->arg(0).toUInt();
   m_bound_buffers[target]->data(call);
}

void StateImpl::BindProgram(PCall call)
{
   GLint program_id = call->arg(0).toSInt();

   if (program_id > 0) {
      auto prog = m_programs.find(program_id);
      assert(prog != m_programs.end());
      m_active_program = prog->second;
      m_active_program->append_call(call);
   } else
      m_active_program = nullptr;
}

void StateImpl::CallList(PCall call)
{
   if (m_in_target_frame) {
      auto list  = m_display_lists.find(call->arg(0).toUInt());
      assert(list != m_display_lists.end());
      list->second->append_calls_to(m_required_calls);
   }
}

void StateImpl::CreateShader(PCall call)
{
   GLint shader_id = call->ret->toUInt();
   GLint stage = call->arg(0).toUInt();
   auto shader = make_shared<ShaderState>(shader_id, stage);
   m_shaders[shader_id] = shader;
   shader->append_call(call);
}

void StateImpl::CreateProgram(PCall call)
{
   GLint shader_id = call->ret->toUInt();
   auto program = make_shared<ProgramState>(shader_id);
   m_programs[shader_id] = program;
   program->append_call(call);
}

void StateImpl::DeleteLists(PCall call)
{
   GLint value = call->arg(0).toUInt();
   GLint value_end = call->arg(1).toUInt() + value;
   for(unsigned i = value; i < value_end; ++i) {
      auto list  = m_display_lists.find(call->arg(0).toUInt());
      assert(list != m_display_lists.end());
      m_display_lists.erase(list);
   }
}

void StateImpl::DrawElements(PCall call)
{
   auto buf = m_bound_buffers[GL_ELEMENT_ARRAY_BUFFER];
   if (buf) {
      buf->use(call);
      if (m_in_target_frame)
         buf->append_calls_to(m_required_calls);
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
      m_active_display_list->append_call(call);
}

void StateImpl::EndList(PCall call)
{
   if (!m_in_target_frame)
      m_active_display_list->append_call(call);

   m_active_display_list = nullptr;
}

void StateImpl::GenBuffers(PCall call)
{
   const auto ids = (call->arg(1)).toArray();
   for (auto& v : ids->values) {
      auto obj = make_shared<BufferState>(v->toUInt(), call);
      m_buffers[v->toUInt()] = obj;
   }
}

void StateImpl::GenTextures(PCall call)
{
   const auto ids = (call->arg(1)).toArray();
   for (auto& v : ids->values) {
      auto obj = make_shared<TextureState>(v->toUInt(), call);
      obj->append_call(call);
      m_textures[v->toUInt()] = obj;
   }
}

void StateImpl::GenLists(PCall call)
{
   unsigned nlists = call->arg(0).toUInt();
   GLuint origResult = (*call->ret).toUInt();
   for (unsigned i = 0; i < nlists; ++i)
      m_display_lists[i + origResult] = PObjectState(new ObjectState(i + origResult));

   if (!m_in_target_frame)
      m_display_lists[origResult]->append_call(call);
}

void StateImpl::GenVertexArrays(PCall call)
{
   const auto ids = (call->arg(1)).toArray();
   for (auto& v : ids->values) {
      auto obj = PObjectState(new ObjectState(v->toUInt()));
      obj->append_call(call);
      m_vertex_arrays[v->toUInt()] = obj;
   }
}

void StateImpl::Light(PCall call)
{
   auto light_id = call->arg(0).toUInt();
   m_last_lights[light_id] = call;
}

void StateImpl::program_call(PCall call)
{
   unsigned progid = call->arg(0).toUInt();
   assert(m_programs[progid]);
   m_programs[progid]->append_call(call);
}

void StateImpl::LoadIdentity(PCall call)
{
   m_current_matrix->identity(call);
}

void StateImpl::Material(PCall call)
{
   if (m_active_display_list)
      m_active_display_list->append_call(call);
}

void StateImpl::MatrixMode(PCall call)
{
   switch (call->arg(0).toUInt()) {
   case GL_MODELVIEW:
      m_current_matrix_stack = &m_mv_matrix;
      break;
   case GL_PROJECTION:
      m_current_matrix_stack = &m_proj_matrix;
      break;
   case GL_TEXTURE:
      m_current_matrix_stack = &m_texture_matrix;
      break;
   case GL_COLOR:
      m_current_matrix_stack = &m_color_matrix;
      break;
   default:
      assert(0 && "Unknown matrix mode");
   }

   if (m_current_matrix_stack->empty())
      m_current_matrix_stack->push(make_shared<MatrixState>(nullptr));

   m_current_matrix = m_current_matrix_stack->top();
   m_current_matrix->append_call(call);
}

void StateImpl::NewList(PCall call)
{
   assert(!m_active_display_list);
   auto list  = m_display_lists.find(call->arg(0).toUInt());
   assert(list != m_display_lists.end());
   m_active_display_list = list->second;

   if (!m_in_target_frame)
      m_active_display_list->append_call(call);
}

void StateImpl::Normal(PCall call)
{
   if (m_active_display_list)
      m_active_display_list->append_call(call);
}

void StateImpl::PopMatrix(PCall call)
{
   m_current_matrix->append_call(call);
   m_current_matrix_stack->pop();
   assert(!m_current_matrix_stack->empty());
   m_current_matrix = m_current_matrix_stack->top();

}

void StateImpl::PushMatrix(PCall call)
{
   m_current_matrix = make_shared<MatrixState>(m_current_matrix);
   m_current_matrix_stack->push(m_current_matrix);
   m_current_matrix->append_call(call);
}

void StateImpl::Rotate(PCall call)
{
   m_current_matrix->append_call(call);
}

void StateImpl::ShadeModel(PCall call)
{
   if (m_active_display_list)
      m_active_display_list->append_call(call);
}

void StateImpl::shader_call(PCall call)
{
   auto shader = m_shaders[call->arg(0).toUInt()];
   shader->append_call(call);
}

void StateImpl::texture_call(PCall call)
{
   auto texture = m_bound_texture[(call->arg(0).toUInt() << 16) |
                  m_active_texture_unit];
   assert(texture);
   texture->data(call);
}

void StateImpl::Translate(PCall call)
{
   m_current_matrix->append_call(call);
}

void StateImpl::UseProgram(PCall call)
{
   unsigned progid = call->arg(0).toUInt();
   bool new_program = false;

   if (!m_active_program ||
       m_active_program->id() != progid) {
      m_active_program = progid > 0 ? m_programs[progid] : nullptr;
      new_program = true;
   }

   if (m_active_program && new_program)
         m_active_program->bind(call);

   if (m_in_target_frame) {
      if (m_active_program)
         m_active_program->append_calls_to(m_required_calls);
      else
         m_required_calls.insert(call);
   }
}

 void StateImpl::Uniform(PCall call)
 {
    assert(m_active_program);
    m_active_program->set_uniform(call);
 }

void StateImpl::Vertex(PCall call)
{
   if (m_active_display_list)
      m_active_display_list->append_call(call);
}

void StateImpl::VertexAttribPointer(PCall call)
{
   auto buf = m_bound_buffers[GL_ARRAY_BUFFER];
   if (buf) {
      buf->use(call);
      m_active_program->set_va(call->arg(0).toUInt(), buf);
   } else
      std::cerr << "Calling VertexAttribPointer without bound ARRAY_BUFFER, ignored\n";
}

void StateImpl::record_state_call(PCall call)
{
   m_state_calls[call->name()] = call;
}

void StateImpl::record_state_call_ex(PCall call)
{
   std::stringstream s;
   s << call->name() << call->arg(0).toUInt();
   m_state_calls[s.str()] = call;
}

void StateImpl::record_required_call(PCall call)
{
   if (call)
      m_required_calls.insert(call);
}

void StateImpl::history_ignore(PCall call)
{
   (void)call;
}

void StateImpl::todo(PCall call)
{
   std::cerr << "TODO:" << call->name() << "\n";
}

void StateImpl::write(trace::Writer& writer)
{
   std::vector<PCall> sorted_calls(m_required_calls.begin(),
                                   m_required_calls.end());

   std::sort(sorted_calls.begin(), sorted_calls.end(),
             [](PCall lhs, PCall rhs) {return lhs->no < rhs->no;});

   auto last_call = *sorted_calls.rbegin();
   std::cerr << "Last call flags = "<< last_call->flags << "\n";
   for(auto& call: sorted_calls) {
      if (call) {
         writer.writeCall(call.get());
      }
      else
         std::cerr << "NULL call in callset\n";
   }
}

void StateImpl::register_callbacks()
{
#define MAP(name, call) m_call_table.insert(std::make_pair(#name, bind(&StateImpl:: call, this, _1)))

   MAP(glActiveTexture, ActiveTexture);

   MAP(glAttachShader, AttachShader);
   MAP(glAttachObject, AttachShader);
   MAP(glBegin, Begin);
   MAP(glBindAttribLocation, BindAttribLocation);
   MAP(glBindBuffer, BindBuffer);
   MAP(glBindTexture, BindTexture);
   MAP(glBindVertexArray, record_state_call);

   MAP(glBindProgram, BindProgram);
   MAP(glBlendFunc, record_state_call);

   MAP(glBufferData, BufferData);

   MAP(glCallList, CallList);
   MAP(glClear, history_ignore);
   MAP(glClearColor, record_state_call);

   MAP(glCreateShader, CreateShader);
   MAP(glCreateProgram, CreateProgram);
   MAP(glCompileShader, shader_call);
   MAP(glDeleteLists, DeleteLists);
   MAP(glDeleteBuffers, history_ignore);
   MAP(glDeleteProgram, history_ignore);
   MAP(glDeleteShader, history_ignore);
   MAP(glDeleteVertexArrays, history_ignore);

   MAP(glCompressedTexImage2D, texture_call);
   MAP(TexImage1D, texture_call);
   MAP(TexImage2D, texture_call);
   MAP(TexImage3D, texture_call);

   MAP(glDepthFunc, record_state_call);

   MAP(glDetachShader, history_ignore);
   MAP(glDisableVertexAttribArray, record_va_enables);
   MAP(glDrawArrays, history_ignore);
   MAP(glEnableVertexAttribArray, record_va_enables);

   MAP(glDrawElements, DrawElements);

   MAP(glDisable, record_enable);
   MAP(glEnable, record_enable);
   MAP(glEnd, End);
   MAP(glEndList, EndList);
   MAP(glFrustum, record_state_call);
   MAP(glGenBuffers, GenBuffers);
   MAP(glGenTexture, GenTextures);

   MAP(glGenLists, GenLists);
   MAP(glGenVertexArrays, GenVertexArrays);
   MAP(glGetUniformLocation, program_call);

   MAP(glGetInteger, history_ignore);
   MAP(glGetProgram, history_ignore);
   MAP(glGetShader, history_ignore);
   MAP(glGetString, history_ignore);

   MAP(glLight, Light);
   MAP(glLinkProgram, program_call);
   MAP(glUseProgram, UseProgram);
   MAP(glVertexAttribPointer, VertexAttribPointer);
   MAP(glLoadIdentity, LoadIdentity);
   MAP(glMaterial, Material);
   MAP(glMatrixMode, MatrixMode);
   MAP(glNewList, NewList);
   MAP(glNormal, Normal);
   MAP(glPixelStorei, record_state_call_ex);
   MAP(glPopMatrix, PopMatrix);
   MAP(glPushMatrix, PushMatrix);
   MAP(glRotate, Rotate);
   MAP(glScissor, record_state_call);
   MAP(glShadeModel, ShadeModel);
   MAP(glShaderSource, shader_call);
   MAP(glTranslate, Translate);
   MAP(glUniform, Uniform);
   MAP(glVertex, Vertex);
   MAP(glVertex2, Vertex);
   MAP(glVertex3, Vertex);
   MAP(glVertex4, Vertex);

   MAP(glViewport, record_state_call);

   MAP(glXGetFBConfigAttrib, history_ignore);
   MAP(glXChooseVisual, record_required_call);
   MAP(glXCreateContext, record_required_call);
   MAP(glXDestroyContext, record_required_call);
   MAP(glXGetSwapIntervalMESA, record_required_call);
   MAP(glXQueryExtensionsString, record_required_call);
   MAP(glXMakeCurrent, record_required_call);
   MAP(glXSwapBuffers, history_ignore);
   MAP(glXGetProcAddress, record_required_call);
   MAP(glXQueryVersion, history_ignore);
   MAP(glXGetClientString, history_ignore);
   MAP(glXGetFBConfigs, history_ignore);
   MAP(glXGetVisualFromFBConfig, history_ignore);
   MAP(glXGetCurrentDisplay, history_ignore);

#undef MAP
/*
   for(auto& x: m_call_table) {
      std::cerr << "Mapped " << x.first << "\n";
   }*/

}

} // namespace frametrim