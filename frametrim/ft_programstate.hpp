#ifndef PROGRAMSTATE_HPP
#define PROGRAMSTATE_HPP

#include "ft_objectstate.hpp"

#include <unordered_map>

namespace frametrim {

class ShaderState: public ObjectState {
public:
   ShaderState(unsigned id, unsigned stage);

   unsigned stage() const;
private:
   unsigned m_stage;
};

using PShaderState = std::shared_ptr<ShaderState>;


class ProgramState : public ObjectState
{
public:
   ProgramState(unsigned id);

   void attach_shader(PShaderState shader);
   void set_uniform(PCall call);
   void set_va(unsigned id, PObjectState va);

   void bind(PCall call);

private:

   void do_emit_calls_to_list(CallSet& list) const override;

   std::unordered_map<unsigned, PShaderState> m_shaders;

   std::unordered_map<unsigned, PCall> m_uniforms;
   std::unordered_map<unsigned, PObjectState> m_va;

   PCall m_last_bind;

};

using PProgramState = std::shared_ptr<ProgramState>;

}

#endif // PROGRAMSTATE_HPP
