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
   assert(!strcmp(call->name(), "glLoadIdentity"));

   m_parent = nullptr;
   append_call(call);
}

void MatrixState::do_append_calls_to(std::vector<PCall>& list) const
{
   if (m_parent)
      m_parent->append_calls_to(list);
}

}
