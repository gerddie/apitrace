#ifndef PROGRAMOBJ_HPP
#define PROGRAMOBJ_HPP

#include "ftr_genobject.hpp"

#include <unordered_set>

namespace frametrim_reverse {

class ProgramObject : public GenObject
{
public:
    using Pointer = std::shared_ptr<ProgramObject>;

    using GenObject::GenObject;
    void attach_shader(unsigned shader_id);
    void bind_attr_location(unsigned loc);

private:
    std::unordered_set<unsigned> m_attached_shaders;
    std::unordered_set<unsigned> m_bound_attributes;
};

class ProgramObjectMap : public CreateObjectMap<ProgramObject> {
public:
    PTraceCall attach_shader(trace::Call& call);
    PTraceCall bind_attr_location(trace::Call& call);

private:
    unsigned target_id_from_call(trace::Call& call) const override;
};

using ShaderObjectMap = CreateObjectMap<GenObject>;

}

#endif // PROGRAMOBJ_HPP
