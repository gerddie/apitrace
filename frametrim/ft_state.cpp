#include "ft_state.h"
#include "ft_matrixstate.hpp"
#include "trace_writer.hpp"

#include <unordered_set>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <stack>

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
   void Begin(PCall call);
   void BindProgram(PCall call);
   void CallList(PCall call);
   void DeleteLists(PCall call);
   void Disable(PCall call);
   void Enable(PCall call);
   void End(PCall call);
   void EndList(PCall call);
   void GenLists(PCall call);
   void Light(PCall call);
   void LoadIdentity(PCall call);
   void Material(PCall call);
   void MatrixMode(PCall call);
   void NewList(PCall call);
   void Normal(PCall call);
   void PopMatrix(PCall call);
   void PushMatrix(PCall call);
   void Rotate(PCall call);
   void ShadeModel(PCall call);
   void Translate(PCall call);
   void Vertex(PCall call);

   void history_ignore(PCall call);

   void record_state_call(PCall call);
   void record_required_call(PCall call);
   void write(trace::Writer& writer);
   void start_target_farme();

   using CallTable = std::multimap<const char *, ft_callback, string_part_less>;
   CallTable m_call_table;

   std::unordered_map<GLint, PObjectState> m_programs;
   PObjectState m_active_program;

   std::unordered_map<GLenum, PCall > m_enables;

   std::unordered_map<GLint, PObjectState> m_display_lists;
   PObjectState m_active_display_list;

   std::unordered_map<uint64_t, PCall> m_last_lights;

   std::unordered_map<std::string, PCall> m_state_calls;

   bool m_in_target_frame;

   std::vector<PCall > m_required_calls;

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
}

StateImpl::StateImpl():
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
   return retval;
}

void StateImpl::call(PCall call)
{
   auto cb_range = m_call_table.equal_range(call->name());

   if (cb_range.first != m_call_table.end()) {
      CallTable::const_iterator cb = cb_range.first;
      CallTable::const_iterator i = cb_range.first;
      ++i;

      unsigned max_equal = equal_chars(cb->first, call->name());

      while (i != cb_range.second) {
         auto n = equal_chars(i->first, call->name());
         if (n > max_equal) {
            max_equal = n;
            cb = i;
         }
         ++i;
      }

      cb->second(call);
   } else {
      /* This should be some debug output only, because we might
       * not handle some calls deliberately */
      std::cerr << call->name() << " unhandled\n";
   }

   if (m_in_target_frame)
      m_required_calls.push_back(call);
}

void StateImpl::Begin(PCall call)
{
   if (m_active_display_list)
      m_active_display_list->append_call(call);
}

void StateImpl::BindProgram(PCall call)
{
   GLint program_id = call->arg(0).toSInt();

   if (program_id > 0) {
      auto prog = m_programs.find(program_id);
      assert(prog != m_programs.end());
      m_active_program = prog->second;
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

void StateImpl::Disable(PCall call)
{
   GLenum value = call->arg(0).toUInt();
   m_enables[value] = call;
}

void StateImpl::Enable(PCall call)
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

void StateImpl::GenLists(PCall call)
{
   unsigned nlists = call->arg(0).toUInt();
   GLuint origResult = (*call->ret).toUInt();
   for (unsigned i = 0; i < nlists; ++i)
      m_display_lists[i + origResult] = PObjectState(new ObjectState(i + origResult));

   if (!m_in_target_frame)
      m_display_lists[origResult]->append_call(call);
}

void StateImpl::Light(PCall call)
{
   auto light_id = call->arg(0).toUInt();
   m_last_lights[light_id] = call;
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

void StateImpl::Translate(PCall call)
{
   m_current_matrix->append_call(call);
}

void StateImpl::Vertex(PCall call)
{
   if (m_active_display_list)
      m_active_display_list->append_call(call);
}

void StateImpl::record_state_call(PCall call)
{
   m_state_calls[call->name()] = call;
}

void StateImpl::record_required_call(PCall call)
{
   if (call)
      m_required_calls.push_back(call);
}

void StateImpl::history_ignore(PCall call)
{
   (void)call;
}

void StateImpl::write(trace::Writer& writer)
{
   std::sort(m_required_calls.begin(), m_required_calls.end(),
             [](PCall rhs, PCall lhs) {
      return rhs->no < lhs->no;
   });

   for(auto& call: m_required_calls)
      writer.writeCall(call.get());
}

void StateImpl::register_callbacks()
{
#define MAP(name, call) m_call_table.insert(std::make_pair(#name, bind(&StateImpl:: call, this, _1)))


   MAP(glBegin, Begin);
   MAP(glBindProgram, BindProgram);
   MAP(glCallList, CallList);
   MAP(glClear, history_ignore);
   MAP(glDeleteLists, DeleteLists);
   MAP(glDisable, Disable);
   MAP(glEnable, Enable);
   MAP(glEnd, End);
   MAP(glEndList, EndList);
   MAP(glFrustum, record_state_call);
   MAP(glGenLists, GenLists);
   MAP(glLight, Light);
   MAP(glLoadIdentity, LoadIdentity);
   MAP(glMaterial, Material);
   MAP(glMatrixMode, MatrixMode);
   MAP(glNewList, NewList);
   MAP(glNormal, Normal);
   MAP(glPopMatrix, PopMatrix);
   MAP(glPushMatrix, PushMatrix);
   MAP(glRotate, Rotate);
   MAP(glScissor, record_state_call);
   MAP(glShadeModel, ShadeModel);
   MAP(glTranslate, Translate);
   MAP(glVertex, Vertex);
   MAP(glViewport, record_state_call);

   MAP(glXChooseVisual, record_required_call);
   MAP(glXCreateContext, record_required_call);
   MAP(glXDestroyContext, record_required_call);
   MAP(glXGetSwapIntervalMESA, record_required_call);
   MAP(glXQueryExtensionsString, record_required_call);
   MAP(glXMakeCurrent, record_required_call);
   MAP(glXSwapBuffers, history_ignore);
   MAP(glXGetProcAddress, record_required_call);


#undef MAP

   for(auto& x: m_call_table) {
      std::cerr << "Mapped " << x.first << "\n";
   }

}

} // namespace frametrim
