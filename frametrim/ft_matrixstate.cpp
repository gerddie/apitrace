#include "ft_matrixstate.hpp"

#include <cstring>

namespace frametrim {

MatrixState::MatrixState(MatrixState::Pointer parent):
    ObjectState(0),
    m_parent(parent)
{

}

void MatrixState::select_matrixtype(PCall call)
{
    m_type_select_call = call;
}

void MatrixState::set_matrix(PCall call)
{
    assert(!strcmp(call->name(), "glLoadIdentity") ||
           !strncmp(call->name(), "glLoadMatrix", 12));

    m_parent = nullptr;
    reset_callset();
    append_call(m_type_select_call);
    append_call(call);
}

void MatrixState::do_emit_calls_to_list(CallSet& list) const
{
    if (m_parent)
        m_parent->emit_calls_to_list(list);
}

}
