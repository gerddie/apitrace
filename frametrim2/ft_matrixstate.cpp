#include "ft_matrixstate.hpp"

#include <cstring>

using std::make_shared;

namespace frametrim {

MatrixState::MatrixState(MatrixState::Pointer parent):
    ObjectState(0, nullptr),
    m_parent(parent)
{

}

void MatrixState::select_matrixtype(const trace::Call& call)
{
    m_type_select_call = trace2call(call);
}

void MatrixState::set_matrix(const trace::Call &call)
{
    assert(!strcmp(call.name(), "glLoadIdentity") ||
           !strncmp(call.name(), "glLoadMatrix", 12));

    /* Remember matrix type when doing
     * glMatrixMode
     * glPushMatrix
     * glLoad*
     */
    if (!m_type_select_call && m_parent)
        m_type_select_call = m_parent->m_type_select_call;

    m_parent = nullptr;
    reset_callset();
    if (m_type_select_call)
        append_call(m_type_select_call);
    append_call(trace2call(call));
}

void MatrixState::do_emit_calls_to_list(CallSet& list) const
{
    if (m_parent)
        m_parent->emit_calls_to_list(list);
}

AllMatrisStates::AllMatrisStates()
{
    m_mv_matrix.push(make_shared<MatrixState>(nullptr));
    m_current_matrix = m_mv_matrix.top();
    m_current_matrix_stack = &m_mv_matrix;
}

void AllMatrisStates::emit_state_to_lists(CallSet& list) const
{
    if (!m_mv_matrix.empty())
        m_mv_matrix.top()->emit_calls_to_list(list);

    if (!m_proj_matrix.empty())
        m_proj_matrix.top()->emit_calls_to_list(list);

    if (!m_texture_matrix.empty())
        m_texture_matrix.top()->emit_calls_to_list(list);

    if (!m_color_matrix.empty())
        m_color_matrix.top()->emit_calls_to_list(list);
}

void AllMatrisStates::LoadIdentity(const trace::Call& call)
{
    m_current_matrix->set_matrix(call);
}

void AllMatrisStates::LoadMatrix(const trace::Call& call)
{
    m_current_matrix->set_matrix(call);
}

void AllMatrisStates::MatrixMode(const trace::Call& call)
{
    switch (call.arg(0).toUInt()) {
    case GL_MODELVIEW:
        m_current_matrix_stack = &m_mv_matrix;
        break;
    case GL_PROJECTION:
        m_current_matrix_stack = &m_proj_matrix;
        break;
    case GL_TEXTURE:
        m_current_matrix_stack = &m_texture_matrix;
        break;
    case GL_COLOR:
        m_current_matrix_stack = &m_color_matrix;
        break;
    default:
        assert(0 && "Unknown matrix mode");
    }

    if (m_current_matrix_stack->empty())
        m_current_matrix_stack->push(make_shared<MatrixState>(nullptr));

    m_current_matrix = m_current_matrix_stack->top();
    m_current_matrix->select_matrixtype(call);
}

void AllMatrisStates::PopMatrix(const trace::Call& call)
{
    m_current_matrix->append_call(trace2call(call));
    m_current_matrix_stack->pop();
    assert(!m_current_matrix_stack->empty());
    m_current_matrix = m_current_matrix_stack->top();
}

void AllMatrisStates::PushMatrix(const trace::Call& call)
{
    m_current_matrix = make_shared<MatrixState>(m_current_matrix);
    m_current_matrix_stack->push(m_current_matrix);
    m_current_matrix->append_call(trace2call(call));
}

void AllMatrisStates::matrix_op(const trace::Call& call)
{
    m_current_matrix->append_call(trace2call(call));
}

}
