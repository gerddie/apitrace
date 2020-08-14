#ifndef MATRIXSTATE_HPP
#define MATRIXSTATE_HPP

#include "ft_objectstate.h"

namespace frametrim {

class MatrixState : public ObjectState
{
public:
   using Pointer = std::shared_ptr<MatrixState>;
   MatrixState(Pointer parent);

   void identity(PCall call);

private:

   void do_append_calls_to(CallSet& list) const override;

   Pointer m_parent;
};

using PMatrixState = MatrixState::Pointer;

}

#endif // MATRIXSTATE_HPP
