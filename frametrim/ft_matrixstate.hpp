#ifndef MATRIXSTATE_HPP
#define MATRIXSTATE_HPP

#include "ft_objectstate.hpp"

namespace frametrim {

class MatrixState : public ObjectState
{
public:
   using Pointer = std::shared_ptr<MatrixState>;
   MatrixState(Pointer parent);

   void identity(PCall call);

private:

   void do_emit_calls_to_list(CallSet& list) const override;

   Pointer m_parent;
};

using PMatrixState = MatrixState::Pointer;

}

#endif // MATRIXSTATE_HPP
