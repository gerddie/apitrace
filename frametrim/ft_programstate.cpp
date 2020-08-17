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
   std::cerr << "Set Program bind call\n";

}


void ProgramState::do_append_calls_to(CallSet& list) const
{
   if (m_last_bind)
      list.insert(m_last_bind);

   for(auto& s : m_shaders)
      s.second->append_calls_to(list);

   for (auto& s: m_uniforms)
      list.insert(s.second);

   for (auto& va: m_va)
      va.second->append_calls_to(list);
}

}