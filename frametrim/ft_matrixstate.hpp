#ifndef MATRIXSTATE_HPP
#define MATRIXSTATE_HPP

#include "ft_objectstate.hpp"
#include <stack>

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

class AllMatrisStates {
public:
    AllMatrisStates();

    void LoadIdentity(PCall call);
    void LoadMatrix(PCall call);
    void MatrixMode(PCall call);
    void PopMatrix(PCall call);
    void PushMatrix(PCall call);

    void matrix_op(PCall call);

    void emit_state_to_lists(CallSet& list) const;

private:
    std::stack<PMatrixState> m_mv_matrix;
    std::stack<PMatrixState> m_proj_matrix;
    std::stack<PMatrixState> m_texture_matrix;
    std::stack<PMatrixState> m_color_matrix;

    PMatrixState m_current_matrix;
    std::stack<PMatrixState> *m_current_matrix_stack;
};


}

#endif // MATRIXSTATE_HPP
