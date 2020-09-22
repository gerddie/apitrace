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

    PTraceCall select_matrixtype(const trace::Call& call);

    PTraceCall set_matrix(const trace::Call& call);

private:

    void do_emit_calls_to_list(CallSet& list) const override;

    Pointer m_parent;

    PTraceCall m_type_select_call;

};

using PMatrixState = std::shared_ptr<MatrixState>;

class AllMatrisStates {
public:
    AllMatrisStates();

    PTraceCall LoadIdentity(const trace::Call& call);
    PTraceCall LoadMatrix(const trace::Call& call);
    PTraceCall MatrixMode(const trace::Call& call);
    PTraceCall PopMatrix(const trace::Call& call);
    PTraceCall PushMatrix(const trace::Call& call);
    PTraceCall matrix_op(const trace::Call& call);

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
