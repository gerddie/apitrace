#include "ft_state.h"

#include <unordered_set>
#include <iostream>
#include <cstring>

namespace frametrim {

using std::bind;
using std::placeholders::_1;


struct StateImpl {

   StateImpl();

   void call(PCall call);
   void register_callbacks();

   /* OpenGL calls */
   void Begin(PCall call);
   void BindProgram(PCall call);
   void CallList(PCall call);
   void Clear(PCall call);
   void DeleteLists(PCall call);
   void Disable(PCall call);
   void Enable(PCall call);
   void End(PCall call);
   void EndList(PCall call);
   void Frustum(PCall call);
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
   void Scissor(PCall call);
   void ShadeModel(PCall call);
   void Translate(PCall call);
   void Vertex(PCall call);
   void Viewport(PCall call);

   void record_required_call(PCall call);

   std::map<const char*, ft_callback> m_call_table;
   std::unordered_map<GLint, PObjectState> m_programs;
   PObjectState m_active_program;
   std::vector<PCall > m_calls;

   std::unordered_map<GLenum, PCall > m_enables;

   std::unordered_map<GLint, PObjectState> m_display_lists;
   PObjectState m_active_display_list;

   std::unordered_map<uint64_t, PCall> m_last_lights;
   PCall m_last_viewport;
   PCall m_last_frustum;
   PCall m_last_scissor;

   bool m_in_target_frame;

   std::vector<PCall > m_required_calls;

};


void State::call(PCall call)
{
   impl->call(call);
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
}

StateImpl::StateImpl():
   m_in_target_frame(false)
{
}

void StateImpl::call(PCall call)
{
   auto cb = m_call_table.lower_bound(call->name());

   if (cb != m_call_table.end() &&
       !strncmp(cb->first, call->name(), strlen(cb->first))) {
         cb->second(call);
   } else {
      /* This should be some debug output only, because we might
       * not handle some calls deliberately */
      std::cerr << "Unahandled call " << call->name() << "\n";
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

void StateImpl::Clear(PCall call)
{

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
   m_active_display_list = nullptr;
}

void StateImpl::Frustum(PCall call)
{
   m_last_frustum = call;
}

void StateImpl::GenLists(PCall call)
{
   unsigned nlists = call->arg(0).toUInt();
   GLuint origResult = (*call->ret).toUInt();
   for (unsigned i = 0; i < nlists; ++i)
      m_display_lists[i + origResult] = PObjectState(new ObjectState(i + origResult));
}

void StateImpl::Light(PCall call)
{
   auto light_id = call->arg(0).toUInt();
   m_last_lights[light_id] = call;
}

void StateImpl::LoadIdentity(PCall call)
{

}

void StateImpl::Material(PCall call)
{
   if (m_active_display_list)
      m_active_display_list->append_call(call);
}

void StateImpl::MatrixMode(PCall call)
{

}

void StateImpl::NewList(PCall call)
{
   assert(!m_active_display_list);
   auto list  = m_display_lists.find(call->arg(0).toUInt());
   assert(list != m_display_lists.end());
   m_active_display_list = list->second;
}

void StateImpl::Normal(PCall call)
{
   if (m_active_display_list)
      m_active_display_list->append_call(call);
}

void StateImpl::PopMatrix(PCall call)
{

}

void StateImpl::PushMatrix(PCall call)
{

}

void StateImpl::Rotate(PCall call)
{

}

void StateImpl::Scissor(PCall call)
{
   m_last_scissor = call;
}

void StateImpl::ShadeModel(PCall call)
{
   if (m_active_display_list)
      m_active_display_list->append_call(call);
}

void StateImpl::Translate(PCall call)
{

}

void StateImpl::Vertex(PCall call)
{
   if (m_active_display_list)
      m_active_display_list->append_call(call);
}

void StateImpl::Viewport(PCall call)
{
   m_last_viewport = call;
}

void StateImpl::record_required_call(PCall call)
{
   m_required_calls.push_back(call);
}

void StateImpl::register_callbacks()
{
#define MAP(name, call) m_call_table[#name] = bind(&StateImpl:: call, this, _1)


   MAP(glBegin, Begin);
   MAP(glBindProgram, BindProgram);
   MAP(glCallList, CallList);
   MAP(glClear, Clear);
   MAP(glDeleteLists, DeleteLists);
   MAP(glDisable, Disable);
   MAP(glEnable, Enable);
   MAP(glEnd, End);
   MAP(glEndList, EndList);
   MAP(glFrustum, Frustum);
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
   MAP(glScissor, Scissor);
   MAP(glShadeModel, ShadeModel);
   MAP(glTranslate, Translate);
   MAP(glVertex, Vertex);
   MAP(glViewport, Viewport);

   MAP(glXChooseVisual, record_required_call);
   MAP(glXCreateContext, record_required_call);
   MAP(glXDestroyContext, record_required_call);
   MAP(glXGetSwapIntervalMESA, record_required_call);
   MAP(glXMakeCurrent, record_required_call);
   MAP(glXSwapBuffers, record_required_call);

#undef MAP
}

} // namespace frametrim
