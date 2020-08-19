#ifndef MATRIXSTATE_HPP
#define MATRIXSTATE_HPP

#include "ft_objectstate.hpp"

namespace frametrim {

class MatrixState : public ObjectState
{
public:
   using Pointer = std::shared_ptr<MatrixState>;

   MatrixState(Pointer parent);

   void select_matrixtype(PCall call);

   void set_matrix(PCall call);

private:

   void do_emit_calls_to_list(CallSet& list) const override;

   Pointer m_parent;

   PCall m_type_select_call;

};

using PMatrixState = std::shared_ptr<MatrixState>;

}

#endif // MATRIXSTATE_HPP
