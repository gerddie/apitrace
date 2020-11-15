#ifndef MATRIXSTATE_HPP
#define MATRIXSTATE_HPP

#include "ftb_dependecyobject.hpp"
#include <stack>

namespace frametrim {

class MatrixState : public UsedObject
{
public:
    using Pointer = std::shared_ptr<MatrixState>;

    MatrixState(Pointer parent);

    void select_matrixtype(const trace::Call& call);

    void set_matrix(const trace::Call& call);

private:
    Pointer m_parent;

    PTraceCall m_type_select_call;
};

using PMatrixState = std::shared_ptr<MatrixState>;

class AllMatrisStates {
public:
    AllMatrisStates();

    void LoadIdentity(const trace::Call& call);
    void LoadMatrix(const trace::Call& call);
    void MatrixMode(const trace::Call& call);
    void PopMatrix(const trace::Call& call);
    void PushMatrix(const trace::Call& call);
    void matrix_op(const trace::Call& call);

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
