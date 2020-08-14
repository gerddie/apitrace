#ifndef PROGRAMSTATE_HPP
#define PROGRAMSTATE_HPP

#include "ft_objectstate.h"

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

private:

   void do_append_calls_to(CallSet& list) const override;

   std::unordered_map<unsigned, PShaderState> m_shaders;

};

using PProgramState = std::shared_ptr<ProgramState>;

}

#endif // PROGRAMSTATE_HPP
