#ifndef PROGRAMOBJ_HPP
#define PROGRAMOBJ_HPP

#include "ftr_boundobject.hpp"

#include <unordered_set>

namespace frametrim_reverse {

using ShaderObjectMap = CreateObjectMap<GenObject>;
using VertexArrayMap = GenObjectMap<GenObject>;

class ProgramObject : public GenObject
{
public:
    using Pointer = std::shared_ptr<ProgramObject>;

    using GenObject::GenObject;
    void attach_shader(PGenObject shader);
    void bind_attr_location(unsigned loc);

private:
    void collect_dependend_obj(Queue& objects) override;

    std::unordered_set<PGenObject> m_attached_shaders;
    std::unordered_set<unsigned> m_bound_attributes;
};

class ProgramObjectMap : public CreateObjectMap<ProgramObject> {
public:
    PTraceCall attach_shader(trace::Call& call, const ShaderObjectMap& shaders);
    PTraceCall bind_attr_location(trace::Call& call);

private:
    unsigned target_id_from_call(const trace::Call& call) const override;
};

}

#endif // PROGRAMOBJ_HPP
