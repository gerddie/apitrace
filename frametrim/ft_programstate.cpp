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

void ProgramState::do_append_calls_to(CallSet& list) const
{
   for(auto& s : m_shaders)
      s.second->append_calls_to(list);
}

}