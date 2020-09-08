#ifndef MATRIXOBJECT_HPP
#define MATRIXOBJECT_HPP

#include "ftr_genobject.hpp"

#include <stack>

namespace frametrim_reverse {

class MatrixObject : public TraceObject {
public:
    using Pointer = std::shared_ptr<MatrixObject>;
    enum CallType {
        reset,
        select,
        data
    };

    MatrixObject(Pointer parent = nullptr);

    void set_parent(Pointer parent);

    void call(unsigned callno, CallType type);
private:
    using MatrixCallList = std::vector<std::pair<unsigned, CallType>>;
    void insert_last_select_from_before(MatrixCallList& calls, unsigned callno);

    void collect_data_calls(CallIdSet& calls, unsigned call_before) override;

    MatrixCallList  m_calls;
    Pointer m_parent;
};

using PMatrixObject = MatrixObject::Pointer;

class MatrixObjectMap {
public:

    MatrixObjectMap();

    PTraceCall LoadIdentity(const trace::Call& call);
    PTraceCall LoadMatrix(const trace::Call& call);
    PTraceCall MatrixMode(const trace::Call& call);
    PTraceCall PopMatrix(const trace::Call& call);
    PTraceCall PushMatrix(const trace::Call& call);
    PTraceCall matrix_op(const trace::Call& call);

private:
    std::stack<PMatrixObject> m_mv_matrix;
    std::stack<PMatrixObject> m_proj_matrix;
    std::stack<PMatrixObject> m_texture_matrix;
    std::stack<PMatrixObject> m_color_matrix;

    PMatrixObject m_current_matrix;
    std::stack<PMatrixObject> *m_current_matrix_stack;
};

}

#endif // MATRIXOBJECT_HPP
