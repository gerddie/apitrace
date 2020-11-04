#ifndef DEPENDECYOBJECT_HPP
#define DEPENDECYOBJECT_HPP

#include "ft_tracecall.hpp"

namespace frametrim {

class DependecyObject
{
public:
    using Pointer = std::shared_ptr<DependecyObject>;

    void add_call(PTraceCall call);
    void add_depenency(Pointer dep);

    void append_calls(CallSet& out_list);

private:

    std::vector<PTraceCall> m_calls;
    std::vector<Pointer> m_dependencies;

};

class DependecyObjectMap {
public:
    PTraceCall Generate(const trace::Call& call);
    PTraceCall Bind(const trace::Call& call);
    PTraceCall CallOnBoundObject(const trace::Call& call);
    PTraceCall CallOnBoundObjectWithDep(const trace::Call& call, int dep_obj_param,
                                        DependecyObjectMap& other_objects);

    DependecyObject::Pointer get_by_id(unsigned id) const;
private:

    virtual unsigned get_id_from_call(const trace::Call& call) const;
    virtual unsigned get_bindpoint_from_call(const trace::Call& call) const;

    std::unordered_map<unsigned, DependecyObject::Pointer> m_objects;
    std::unordered_map<unsigned, DependecyObject::Pointer> m_bound_object;

    std::vector<PTraceCall> m_calls;
};




}

#endif // DEPENDECYOBJECT_HPP
