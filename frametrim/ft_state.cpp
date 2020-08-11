#include "ft_state.h"

#include <unordered_set>
#include <iostream>

namespace frametrim {

using std::bind;
using std::placeholders::_1;


struct StateImpl {

   StateImpl();

   void call(trace::Call& call);
   void register_callbacks();

   /* OpenGL calls */
   void Begin(trace::Call& call);
   void BindProgram(trace::Call& call);
   void CallList(trace::Call& call);
   void Clear(trace::Call& call);
   void DeleteLists(trace::Call& call);
   void Disable(trace::Call& call);
   void Enable(trace::Call& call);
   void End(trace::Call& call);
   void EndList(trace::Call& call);
   void Frustum(trace::Call& call);
   void GenLists(trace::Call& call);
   void Light(trace::Call& call);
   void LoadIdentity(trace::Call& call);
   void Material(trace::Call& call);
   void MatrixMode(trace::Call& call);
   void NewList(trace::Call& call);
   void Normal(trace::Call& call);
   void PopMatrix(trace::Call& call);
   void PushMatrix(trace::Call& call);
   void Rotate(trace::Call& call);
   void Scissor(trace::Call& call);
   void ShadeModel(trace::Call& call);
   void Translate(trace::Call& call);
   void Vertex(trace::Call& call);
   void Viewport(trace::Call& call);

   std::map<const char*, ft_callback> m_call_table;
   std::unordered_map<GLint, PObjectState> m_programs;
   PObjectState m_active_program;
   std::vector<trace::Call *> m_calls;

   std::unordered_map<GLenum, trace::Call *> m_enables;

   std::unordered_map<GLint, PObjectState> m_display_lists;
   PObjectState m_active_display_list;

   std::unordered_map<uint64_t, trace::Call *> m_last_lights;
   trace::Call *m_last_viewport;

   bool m_target_frame_started;

   std::vector<trace::Call *> m_required_calls;

};


void State::call(trace::Call& call)
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
   impl->m_target_frame_started = true;
}

StateImpl::StateImpl():
   m_last_viewport(nullptr),
   m_target_frame_started(false)
{
}

void StateImpl::call(trace::Call& call)
{
   auto cb = m_call_table.find(call.name());

   if (cb != m_call_table.end())
      cb->second(call);
   else {
      std::cerr << "Unahandled call " << call.name() << "\n";
   }
   if (m_target_frame_started)
      m_required_calls.push_back(&call);


}

void StateImpl::Begin(trace::Call& call)
{
   if (m_active_display_list)
      m_active_display_list->append_call(&call);
}

void StateImpl::BindProgram(trace::Call& call)
{
   GLint program_id = call.arg(0).toSInt();

   if (program_id > 0) {
      auto prog = m_programs.find(program_id);
      assert(prog != m_programs.end());
      m_active_program = prog->second;
   } else
      m_active_program = nullptr;
}


void StateImpl::CallList(trace::Call& call)
{
   if (m_target_frame_started) {
      auto list  = m_display_lists.find(call.arg(0).toUInt());
      assert(list != m_display_lists.end());
      list->second->append_calls_to(m_required_calls);
   }
}

void StateImpl::Clear(trace::Call& call)
{

}

void StateImpl::DeleteLists(trace::Call& call)
{
   GLint value = call.arg(0).toUInt();
   GLint value_end = call.arg(1).toUInt() + value;
   for(unsigned i = value; i < value_end; ++i) {
      auto list  = m_display_lists.find(call.arg(0).toUInt());
      assert(list != m_display_lists.end());
      m_display_lists.erase(list);
   }
}

void StateImpl::Disable(trace::Call& call)
{
   GLenum value = call.arg(0).toUInt();
   m_enables[value] = &call;
}

void StateImpl::Enable(trace::Call& call)
{
   GLenum value = call.arg(0).toUInt();
   m_enables[value] = &call;
}

void StateImpl::End(trace::Call& call)
{
   if (m_active_display_list)
      m_active_display_list->append_call(&call);
}

void StateImpl::EndList(trace::Call& call)
{
   m_active_display_list = nullptr;
}

void StateImpl::Frustum(trace::Call& call)
{

}

void StateImpl::GenLists(trace::Call& call)
{
   unsigned nlists = call.arg(0).toUInt();
   GLuint origResult = (*call.ret).toUInt();
   for (unsigned i = 0; i < nlists; ++i)
      m_display_lists[i + origResult] = PObjectState(new ObjectState(i + origResult));
}

void StateImpl::Light(trace::Call& call)
{
   auto light_id = call.arg(0).toUInt();
   m_last_lights[light_id] = &call;
}

void StateImpl::LoadIdentity(trace::Call& call)
{

}

void StateImpl::Material(trace::Call& call)
{
   if (m_active_display_list)
      m_active_display_list->append_call(&call);
}

void StateImpl::MatrixMode(trace::Call& call)
{

}

void StateImpl::NewList(trace::Call& call)
{
   assert(!m_active_display_list);
   auto list  = m_display_lists.find(call.arg(0).toUInt());
   assert(list != m_display_lists.end());
   m_active_display_list = list->second;
}

void StateImpl::Normal(trace::Call& call)
{
   if (m_active_display_list)
      m_active_display_list->append_call(&call);
}

void StateImpl::PopMatrix(trace::Call& call)
{

}

void StateImpl::PushMatrix(trace::Call& call)
{

}

void StateImpl::Rotate(trace::Call& call)
{

}

void StateImpl::Scissor(trace::Call& call)
{

}

void StateImpl::ShadeModel(trace::Call& call)
{
   if (m_active_display_list)
      m_active_display_list->append_call(&call);
}

void StateImpl::Translate(trace::Call& call)
{

}

void StateImpl::Vertex(trace::Call& call)
{
   if (m_active_display_list)
      m_active_display_list->append_call(&call);
}

void StateImpl::Viewport(trace::Call& call)
{
   m_last_viewport = &call;
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

#undef MAP
}

} // namespace frametrim
