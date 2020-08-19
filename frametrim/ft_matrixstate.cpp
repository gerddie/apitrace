#include "ft_matrixstate.hpp"

#include <cstring>

namespace frametrim {

MatrixState::MatrixState(MatrixState::Pointer parent):
   ObjectState(0),
   m_parent(parent)
{

}

void MatrixState::identity(PCall call)
{
   assert(!strcmp(call->name(), "glLoadIdentity") ||
          !strncmp(call->name(), "glLoadMatrix", 12));

   m_parent = nullptr;
   append_call(call);
}

void MatrixState::do_emit_calls_to_list(CallSet& list) const
{
   if (m_parent)
      m_parent->emit_calls_to_list(list);
}

}
