#ifndef MATRIXOBJECT_HPP
#define MATRIXOBJECT_HPP

#include "ftr_genobject.hpp"
#include "ftr_tracecall.hpp"

#include <stack>

namespace frametrim_reverse {

class MatrixObject : public TraceObject {
public:
    using Pointer = std::shared_ptr<MatrixObject>;
    MatrixObject(Pointer parent = nullptr);

    void set_parent(Pointer parent);

    PTraceCall call(const trace::Call &call, TraceCall::Flags);
    void accept(ObjectVisitor& visitor) override {visitor.visit(*this);};
private:
    using MatrixCallList = std::vector<std::pair<unsigned, PTraceCall>>;
    void insert_last_select_from_before(MatrixCallList& calls, unsigned callno);

    void collect_data_calls(CallSet& calls, unsigned call_before) override;

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

    void collect_current_state(ObjectVector &objects) const;

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
