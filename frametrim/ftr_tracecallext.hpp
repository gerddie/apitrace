#ifndef TRACECALLEXT_HPP
#define TRACECALLEXT_HPP

#include "ftr_tracecall.hpp"
#include "ftr_genobject.hpp"

namespace frametrim_reverse {


class StateCall : public TraceCall {
public:
    StateCall(const trace::Call& call, unsigned num_param_ids);
private:
    static std::string combined_name(const trace::Call& call,
                                     unsigned num_param_ids);
};

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
    TraceCallOnBoundObjWithDeps(const trace::Call& call, PGenObject obj,
                                PGenObject dep);
private:
    void add_dependend_objects(ObjectSet& out_set) const override;
    PGenObject m_dependency;
};

}

#endif // DUMMY_HPP
