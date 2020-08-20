#include "ft_state.hpp"
#include "ft_bufferstate.hpp"
#include "ft_framebufferstate.hpp"
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
	void register_state_calls();


   /* OpenGL calls */
   void ActiveTexture(PCall call);
   void AttachShader(PCall call);
   void Begin(PCall call);
   void BindAttribLocation(PCall call);
   void BindBuffer(PCall call);
   void BindFramebuffer(PCall call);
   void BindRenderbuffer(PCall call);
   void BindTexture(PCall call);
   void BufferData(PCall call);
   void BufferSubData(PCall call);
   void BindProgram(PCall call);
   void CallList(PCall call);
   void Clear(PCall call);
   void CreateShader(PCall call);
   void CreateProgram(PCall call);
   void DeleteLists(PCall call);
   void DrawElements(PCall call);
   void End(PCall call);
   void EndList(PCall call);
   void FramebufferTexture(PCall call);
   void FramebufferRenderbuffer(PCall call);
   void GenBuffers(PCall call);
   void GenFramebuffer(PCall call);
   void GenLists(PCall call);
	void GenSamplers(PCall call);
   void GenTextures(PCall call);
   void GenVertexArrays(PCall call);
   void GenRenderbuffer(PCall call);
   void Light(PCall call);
   void program_call(PCall call);
   void LoadIdentity(PCall call);
   void LoadMatrix(PCall call);
   void Material(PCall call);
   void MatrixMode(PCall call);
   void NewList(PCall call);
   void Normal(PCall call);
   void PopMatrix(PCall call);
   void PushMatrix(PCall call);
   void RenderbufferStorage(PCall call);

   void Rotate(PCall call);
   void ShadeModel(PCall call);
   void shader_call(PCall call);


   void Translate(PCall call);
   void Uniform(PCall call);
   void UseProgram(PCall call);
   void Vertex(PCall call);
   void Viewport(PCall call);
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

   void collect_state_calls(CallSet& lisr) const;

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

   std::unordered_map<GLint, PTextureState> m_textures;
   std::unordered_map<GLint, PTextureState> m_bound_texture;
   unsigned m_active_texture_unit;
   PCall m_active_texture_unit_call;

	std::unordered_map<GLint, PGenObjectState> m_samplers;
   std::unordered_map<GLint, PGenObjectState> m_bound_samplers;

   std::unordered_map<GLint, PObjectState> m_vertex_arrays;
   std::unordered_map<GLint, PBufferState> m_vertex_attr_pointer;

   std::unordered_map<GLint, PFramebufferState> m_framebuffers;
   PFramebufferState m_draw_framebuffer;
   PFramebufferState m_read_framebuffer;
   PCall m_draw_framebuffer_call;
   PCall m_read_framebuffer_call;

   std::unordered_map<GLint, PRenderbufferState> m_renderbuffers;
   PRenderbufferState m_active_renderbuffer;


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

void StateImpl::collect_state_calls(CallSet& list) const
{
   for(auto& a : m_state_calls)
      list.insert(a.second);

   for(auto& cap: m_enables)
      list.insert(cap.second);

   for (auto& l: m_last_lights)
      list.insert(l.second);

   if (!m_mv_matrix.empty())
      m_mv_matrix.top()->emit_calls_to_list(list);

   if (!m_proj_matrix.empty())
      m_proj_matrix.top()->emit_calls_to_list(list);

   if (!m_texture_matrix.empty())
      m_texture_matrix.top()->emit_calls_to_list(list);

   if (!m_color_matrix.empty())
      m_color_matrix.top()->emit_calls_to_list(list);

   /* Set vertex attribute array pointers only if they are enabled */
   for(auto& va : m_vertex_attr_pointer) {
      auto vae = m_va_enables.find(va.first);
      if (vae != m_va_enables.end() &&
          !strcmp(vae->second->name(), "glEnableVertexAttribArray")) {
         va.second->emit_calls_to_list(list);
         list.insert(vae->second);
      }
   }

   for (auto& va: m_vertex_arrays)
      va.second->emit_calls_to_list(list);

   for (auto& buf: m_bound_buffers)
      buf.second->emit_calls_to_list(list);

   for (auto& tex: m_bound_texture)
      tex.second->emit_calls_to_list(list);

   if (m_active_program)
      m_active_program->emit_calls_to_list(list);

   if (m_draw_framebuffer_call)
      list.insert(m_draw_framebuffer_call);

   if (m_read_framebuffer_call)
      list.insert(m_read_framebuffer_call);
}

void StateImpl::start_target_farme()
{
   collect_state_calls(m_required_calls);
}

StateImpl::StateImpl():
   m_active_texture_unit(0),
   m_in_target_frame(false),
   m_required_calls(false)
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
   else if (m_draw_framebuffer)
      m_draw_framebuffer->draw(call);
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

void StateImpl::BindFramebuffer(PCall call)
{
   unsigned target = call->arg(0).toUInt();
   unsigned id = call->arg(1).toUInt();

   if (id)  {

      if ((target == GL_DRAW_FRAMEBUFFER ||
           target == GL_FRAMEBUFFER) &&
          (!m_draw_framebuffer ||
           m_draw_framebuffer->id() != id)) {
         m_draw_framebuffer = m_framebuffers[id];
         m_draw_framebuffer->bind(call);
         m_draw_framebuffer_call = call;

         /* TODO: Create a copy of the current state and record all
          * calls in the framebuffer until it is un-bound
          * attach the framebuffer to any texture that might be
          * attached as render target */

         auto& calls = m_draw_framebuffer->state_calls();
         calls.clear();
         collect_state_calls(calls);
      }

      if ((target == GL_READ_FRAMEBUFFER ||
          target == GL_FRAMEBUFFER) &&
          (!m_read_framebuffer ||
           m_read_framebuffer->id() != id)) {
         m_read_framebuffer = m_framebuffers[id];
         m_read_framebuffer->bind(call);
         m_read_framebuffer_call = call;
      }

   } else {

      if (target == GL_DRAW_FRAMEBUFFER ||
          target == GL_FRAMEBUFFER) {
         m_draw_framebuffer = nullptr;
         m_draw_framebuffer_call = call;
      }

      if (target == GL_READ_FRAMEBUFFER ||
          target == GL_FRAMEBUFFER) {
         m_read_framebuffer = nullptr;
         m_read_framebuffer_call = call;
      }

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
            tex->emit_calls_to_list(m_required_calls);
			}
			if (m_draw_framebuffer)
				tex->emit_calls_to_list(m_draw_framebuffer->state_calls());
      }
   } else
      m_bound_texture.erase(target_unit);
}

void StateImpl::BufferData(PCall call)
{
   unsigned target = call->arg(0).toUInt();
   m_bound_buffers[target]->data(call);
}

void StateImpl::BufferSubData(PCall call)
{
   unsigned target = call->arg(0).toUInt();
   m_bound_buffers[target]->append_data(call);
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

void StateImpl::BindRenderbuffer(PCall call)
{
   assert(call->arg(0).toUInt() == GL_RENDERBUFFER);

   auto id = call->arg(1).toUInt();

   if (!m_active_renderbuffer || m_active_renderbuffer->id() != id) {
      m_active_renderbuffer = m_renderbuffers[id];
      m_active_renderbuffer->append_call(call);
   }
}

void StateImpl::CallList(PCall call)
{
   auto list  = m_display_lists.find(call->arg(0).toUInt());
   assert(list != m_display_lists.end());

   if (m_in_target_frame)
      list->second->emit_calls_to_list(m_required_calls);

   if (m_draw_framebuffer)
      list->second->emit_calls_to_list(m_draw_framebuffer->state_calls());
}

void StateImpl::Clear(PCall call)
{
   if (m_draw_framebuffer)
      m_draw_framebuffer->clear(call);
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
   for(int i = value; i < value_end; ++i) {
      auto list  = m_display_lists.find(call->arg(0).toUInt());
      assert(list != m_display_lists.end());
      m_display_lists.erase(list);
   }
}

void StateImpl::DrawElements(PCall call)
{
   (void)call;

   auto buf = m_bound_buffers[GL_ELEMENT_ARRAY_BUFFER];
   if (buf) {
      if (m_in_target_frame)
         buf->emit_calls_to_list(m_required_calls);

      if (m_draw_framebuffer)
         buf->emit_calls_to_list(m_draw_framebuffer->state_calls());
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
      m_active_display_list->append_call(call);
}

void StateImpl::EndList(PCall call)
{
   if (!m_in_target_frame)
      m_active_display_list->append_call(call);

   m_active_display_list = nullptr;
}

void StateImpl::FramebufferRenderbuffer(PCall call)
{
   unsigned target = call->arg(0).toUInt();
   unsigned attachment = call->arg(1).toUInt();
   unsigned rb_id = call->arg(3).toUInt();

   auto rb = rb_id ? m_renderbuffers[rb_id] : nullptr;

   PFramebufferState  draw_fb;
   bool read_rb = false;

   if (target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER) {
      m_draw_framebuffer->attach(attachment, call, rb);
      draw_fb = m_draw_framebuffer;
   }

   if (target == GL_FRAMEBUFFER || target == GL_READ_FRAMEBUFFER) {
        m_read_framebuffer->attach(attachment, call, rb);
        read_rb = true;
   }

   rb->attach(call, read_rb, draw_fb);
}

void StateImpl::FramebufferTexture(PCall call)
{
   unsigned layer = 0;
   unsigned textarget = 0;
   unsigned has_tex_target = strcmp("glFramebufferTexture", call->name()) != 0;

   unsigned target = call->arg(0).toUInt();
   unsigned attachment = call->arg(1).toUInt();

   if (has_tex_target)
      textarget = call->arg(2).toUInt();

   unsigned texid = call->arg(2 + has_tex_target).toUInt();
   unsigned level = call->arg(3 + has_tex_target).toUInt();

   if (!strcmp(call->name(), "glFramebufferTexture3D"))
      layer = call->arg(5).toUInt();

   assert(level < 16);
   assert(layer < 1 << 12);

   unsigned textargetid = (textarget << 16) | level << 12 | level;

   auto texture = m_textures[texid];

   if (target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER) {
      m_draw_framebuffer->attach(attachment, call, texture);
      texture->rendertarget_of(textargetid, m_draw_framebuffer);
   }

   if (target == GL_FRAMEBUFFER || target == GL_READ_FRAMEBUFFER) {
      m_read_framebuffer->attach(attachment, call, texture);
   }
}

void StateImpl::GenBuffers(PCall call)
{
   const auto ids = (call->arg(1)).toArray();
   for (auto& v : ids->values) {
      auto obj = make_shared<BufferState>(v->toUInt(), call);
      m_buffers[v->toUInt()] = obj;
   }
}

void StateImpl::GenFramebuffer(PCall call)
{
   const auto ids = (call->arg(1)).toArray();
   for (auto& v : ids->values) {
      auto obj = make_shared<FramebufferState>(v->toUInt(), call);
      m_framebuffers[v->toUInt()] = obj;
   }
}

void StateImpl::GenRenderbuffer(PCall call)
{
   const auto ids = (call->arg(1)).toArray();
   for (auto& v : ids->values) {
      auto obj = make_shared<RenderbufferState>(v->toUInt(), call);
      m_renderbuffers[v->toUInt()] = obj;
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

void StateImpl::GenSamplers(PCall call)
{
   const auto ids = (call->arg(1)).toArray();
   for (auto& v : ids->values) {
      auto obj = make_shared<GenObjectState>(v->toUInt(), call);
      obj->append_call(call);
      m_samplers[v->toUInt()] = obj;
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
   m_current_matrix->set_matrix(call);
}

void StateImpl::LoadMatrix(PCall call)
{
   m_current_matrix->set_matrix(call);
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
   m_current_matrix->select_matrixtype(call);
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

void StateImpl::RenderbufferStorage(PCall call)
{
   assert(m_active_renderbuffer);
   m_active_renderbuffer->set_storage(call);
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
         m_active_program->emit_calls_to_list(m_required_calls);
      else
         m_required_calls.insert(call);
   }

   if (m_draw_framebuffer) {
      if (m_active_program)
         m_active_program->emit_calls_to_list(m_draw_framebuffer->state_calls());
      else
         m_draw_framebuffer->draw(call);
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
      m_vertex_attr_pointer[call->arg(0).toUInt()] = buf;

      if (m_active_program)
         m_active_program->set_va(call->arg(0).toUInt(), buf);
      if (m_in_target_frame)
         buf->emit_calls_to_list(m_required_calls);
      else if (m_draw_framebuffer)
         buf->emit_calls_to_list(m_draw_framebuffer->state_calls());
      buf->use(call);
   } else
      std::cerr << "Calling VertexAttribPointer without bound ARRAY_BUFFER, ignored\n";
}

void StateImpl::Viewport(PCall call)
{
   record_state_call(call);
   if (m_draw_framebuffer)
      m_draw_framebuffer->set_viewport(call);
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
   MAP(glBindFramebuffer, BindFramebuffer);
   MAP(glBindTexture, BindTexture);
   MAP(glBindProgram, BindProgram);
   MAP(glBindRenderbuffer, BindRenderbuffer);
   MAP(glBufferData, BufferData);
   MAP(glBufferSubData, BufferSubData);

   MAP(glCallList, CallList);
   MAP(glCheckFramebufferStatus, history_ignore);
   MAP(glClear, Clear);

   MAP(glCreateShader, CreateShader);
   MAP(glCreateProgram, CreateProgram);
   MAP(glCompileShader, shader_call);
   MAP(glDeleteLists, DeleteLists);
   MAP(glDeleteBuffers, history_ignore);
   MAP(glDeleteProgram, history_ignore);
   MAP(glDeleteShader, history_ignore);
   MAP(glDeleteVertexArrays, history_ignore);

   MAP(glCompressedTexImage2D, texture_call);
   MAP(glTexImage1D, texture_call);
   MAP(glTexImage2D, texture_call);
   MAP(glTexImage3D, texture_call);
   MAP(glTexParameter, texture_call);
   MAP(glGenerateMipmap, texture_call);

   MAP(glDetachShader, history_ignore);
   MAP(glDisableVertexAttribArray, record_va_enables);
   MAP(glDrawArrays, history_ignore);
   MAP(glEnableVertexAttribArray, record_va_enables);

   MAP(glFramebufferRenderbuffer, FramebufferRenderbuffer);
   MAP(glFramebufferTexture, FramebufferTexture);
   MAP(glDrawElements, DrawElements);

   MAP(glDisable, record_enable);
   MAP(glEnable, record_enable);
   MAP(glEnd, End);
   MAP(glEndList, EndList);
   MAP(glGenBuffers, GenBuffers);
   MAP(glGenFramebuffer, GenFramebuffer);
   MAP(glGenSamplers, GenSamplers);
	MAP(glGenTexture, GenTextures);

   MAP(glGenLists, GenLists);
   MAP(glGenRenderbuffer, GenRenderbuffer);
   MAP(glGenVertexArrays, GenVertexArrays);
   MAP(glGetUniformLocation, program_call);
   MAP(glGetAttribLocation, program_call);

   MAP(glGetInteger, history_ignore);
   MAP(glGetProgram, history_ignore);
   MAP(glGetShader, history_ignore);
   MAP(glGetString, history_ignore);
   MAP(glGetFloat, history_ignore);
   MAP(glIsEnabled, history_ignore);

   MAP(glLight, Light);
   MAP(glLinkProgram, program_call);
   MAP(glUseProgram, UseProgram);
   MAP(glVertexAttribPointer, VertexAttribPointer);

   MAP(glLoadIdentity, LoadIdentity);
   MAP(glMaterial, Material);
   MAP(glLoadMatrix, LoadMatrix);
   MAP(glMatrixMode, MatrixMode);
   MAP(glNewList, NewList);
   MAP(glNormal, Normal);
   MAP(glPopMatrix, PopMatrix);
   MAP(glPushMatrix, PushMatrix);
   MAP(glRenderbufferStorage, RenderbufferStorage);
   MAP(glRotate, Rotate);
   MAP(glShadeModel, ShadeModel);
   MAP(glShaderSource, shader_call);
   MAP(glTranslate, Translate);
   MAP(glUniform, Uniform);
   MAP(glVertex2, Vertex);
   MAP(glVertex3, Vertex);
   MAP(glVertex4, Vertex);
   MAP(glColor2, Vertex);
   MAP(glColor3, Vertex);
   MAP(glColor4, Vertex);
   MAP(glViewport, Viewport);
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
   MAP(glXGetCurrentDrawable, history_ignore);
/*
   for(auto& x: m_call_table) {
      std::cerr << "Mapped " << x.first << "\n";
   }*/
	register_state_calls();
}

void StateImpl::register_state_calls()
{
	const std::vector<const char *> state_calls  = {
		"glBindVertexArray",
		"glBlendFunc",
		"glCullFace",
		"glClearColor",
		"glDepthFunc",
		"glDrawBuffers",
		"glFrustum",
		"glLineWidth",
		"glAlphaFunc",
		"glFrontFace",
		"glPolygonMode",
		"glPolygonMode",
		"glPolygonOffset",
		"glDepthRange",
		"glColorMask",
		"glBlendEquation",
		"glBlendColor",
		"glDepthMask",
		"glStencilFuncSeparate",
		"glStencilMask",
		"glClearDepth",
		"glClearStencil",
	};

	auto state_call_func = bind(&StateImpl::record_state_call, this, _1);
	for (auto& i : state_calls)
		m_call_table.insert(std::make_pair(i, state_call_func));

	auto state_call_ex_func = bind(&StateImpl::record_state_call_ex, this, _1);
	const std::vector<const char *> state_calls_ex  = {
		"glClipPlane",
		"glColorMaskIndexedEXT"
		"glPixelStorei"
	};

	for (auto& i : state_calls_ex)
		m_call_table.insert(std::make_pair(i, state_call_ex_func));

}

#undef MAP
} // namespace frametrim
