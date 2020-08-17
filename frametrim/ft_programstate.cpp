#include "ft_programstate.hpp"

#include <iostream>

namespace frametrim {

ShaderState::ShaderState(unsigned id, unsigned stage):
   ObjectState(id),
   m_stage(stage)
{

}

unsigned ShaderState::stage() const
{
   return m_stage;
}

ProgramState::ProgramState(unsigned id):
   ObjectState(id)

{

}

void ProgramState::attach_shader(PShaderState shader)
{
   m_shaders[shader->stage()] = shader;
}

void ProgramState::set_uniform(PCall call)
{
   m_uniforms[call->arg(0).toUInt()] = call;
}

void ProgramState::set_va(unsigned id, PObjectState va)
{
   m_va[id] = va;
}

void ProgramState::bind(PCall call)
{
   m_last_bind = call;
   m_uniforms.clear();
}


void ProgramState::do_append_calls_to(CallSet& list) const
{
   for(auto& s : m_shaders)
      s.second->append_calls_to(list);

   if (m_last_bind) {
      std::cerr << "Emit program "
                << id() << " bind call as no " << m_last_bind->no << "\n";
      list.insert(m_last_bind);

      for (auto& s: m_uniforms)
         list.insert(s.second);

      for (auto& va: m_va)
         va.second->append_calls_to(list);
   }
}

}