#include "ftr_matrixobject.hpp"
#include "ftr_tracecall.hpp"

#include <GL/gl.h>


namespace frametrim_reverse {

using std::make_shared;

MatrixObject::MatrixObject(Pointer parent):
    m_parent(parent)
{

}

void MatrixObject::call(unsigned callno, CallType type)
{
    if (type == reset && m_parent) {
        m_parent->insert_last_select_from_before(m_calls, callno);
        m_parent = nullptr;
    }
    m_calls.push_back(std::make_pair(callno, type));
}

void
MatrixObject::insert_last_select_from_before(MatrixCallList& calls, unsigned callno)
{
    for(auto c = m_calls.rbegin(); c != m_calls.rend(); ++c) {
        if (c->first < callno && c->second == select) {
            calls.push_back(*c);
            return;
        }
    }
}

void MatrixObject::collect_data_calls(CallIdSet& calls, unsigned call_before)
{
    bool hit_reset = false;
    for(auto c = m_calls.rbegin(); c != m_calls.rend(); ++c) {
        if (c->first >= call_before)
            continue;
        if (!hit_reset) {
            calls.insert(c->first);
            hit_reset = c->second == reset;
        } else if (c->second == select) {
            calls.insert(c->first);
            if (hit_reset)
                break;
        }
    }

    if (m_parent)
        m_parent->collect_calls(calls, call_before);
}

MatrixObjectMap::MatrixObjectMap()
{
    m_mv_matrix.push(std::make_shared<MatrixObject>());
    m_current_matrix = m_mv_matrix.top();
    m_current_matrix_stack = &m_mv_matrix;
}

PTraceCall MatrixObjectMap::LoadIdentity(const trace::Call& call)
{
    m_current_matrix->call(call.no, MatrixObject::reset);
    return make_shared<TraceCall>(call);
}

PTraceCall MatrixObjectMap::LoadMatrix(const trace::Call& call)
{
    m_current_matrix->call(call.no, MatrixObject::reset);
    return make_shared<TraceCall>(call);
}

PTraceCall MatrixObjectMap::MatrixMode(const trace::Call& call)
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
        m_current_matrix_stack->push(std::make_shared<MatrixObject>());

    m_current_matrix = m_current_matrix_stack->top();
    m_current_matrix->call(call.no, MatrixObject::select);
    return make_shared<TraceCall>(call);
}

PTraceCall MatrixObjectMap::PopMatrix(const trace::Call& call)
{
    m_current_matrix->call(call.no, MatrixObject::data);
    m_current_matrix_stack->pop();
    assert(!m_current_matrix_stack->empty());
    m_current_matrix = m_current_matrix_stack->top();
    return make_shared<TraceCall>(call);
}

PTraceCall MatrixObjectMap::PushMatrix(const trace::Call& call)
{
    m_current_matrix = std::make_shared<MatrixObject>(m_current_matrix);
    m_current_matrix_stack->push(m_current_matrix);
    m_current_matrix->call(call.no, MatrixObject::data);
    return make_shared<TraceCall>(call);
}

PTraceCall MatrixObjectMap::matrix_op(const trace::Call& call)
{
    m_current_matrix->call(call.no,MatrixObject::data);
    return make_shared<TraceCall>(call);
}

void MatrixObjectMap::collect_current_state(GenObject::Queue& objects) const
{
    if (!m_mv_matrix.empty())
        objects.push(m_mv_matrix.top());

    if (!m_proj_matrix.empty())
        objects.push(m_proj_matrix.top());

    if (!m_texture_matrix.empty())
        objects.push(m_texture_matrix.top());

    if (!m_color_matrix.empty())
        objects.push(m_color_matrix.top());
}

}