#ifndef TRACECALLEXT_HPP
#define TRACECALLEXT_HPP

#include "ftr_tracecall.hpp"
#include "ftr_genobject.hpp"

namespace frametrim_reverse {


class TraceCallOnBoundObj : public TraceCall {
public:
    TraceCallOnBoundObj(const trace::Call& call, PGenObject obj);

private:
    void add_owned_object(ObjectSet& out_set) const override;

    PGenObject m_object;
};

/* Currentyl a placeholder, needed for objects that depend on
 * other objects */
class TraceCallOnBoundObjWithDeps : public TraceCallOnBoundObj {
public:
    using TraceCallOnBoundObj::TraceCallOnBoundObj;
};

}

#endif // DUMMY_HPP
