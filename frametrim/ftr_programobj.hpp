#ifndef PROGRAMOBJ_HPP
#define PROGRAMOBJ_HPP

#include "ftr_boundobject.hpp"
#include "ftr_bufobject.hpp"

#include <unordered_set>
#include <unordered_map>

namespace frametrim_reverse {

class ShaderObject : public BoundObject {
public:
    using BoundObject::BoundObject;
    using Pointer = std::shared_ptr<ShaderObject>;
    PTraceCall source(const trace::Call& call);
    PTraceCall compile(const trace::Call& call);
private:
    void collect_data_calls(CallIdSet& calls, unsigned call_before) override;
    PTraceCall m_source_call;
    PTraceCall m_compile_call;
};

using PShaderObject = ShaderObject::Pointer;

class ShaderObjectMap: public CreateObjectMap<ShaderObject> {
public:
    PTraceCall source(const trace::Call& call);
    PTraceCall compile(const trace::Call& call);
};

using VertexArrayMap = GenObjectMap<BoundObject>;

class ProgramObject : public BoundObject
{
public:
    using Pointer = std::shared_ptr<ProgramObject>;

    using BoundObject::BoundObject;
    PTraceCall attach_shader(const trace::Call& call, PGenObject shader);
    void bind_attr_location(unsigned loc);
    PTraceCall data_call(const trace::Call& call);
    PTraceCall link(const trace::Call& call);
    PTraceCall uniform(const trace::Call& call);
private:
    void collect_dependend_obj(Queue& objects, const TraceCallRange& call_range) override;
    void collect_data_calls(CallIdSet& calls, unsigned before_call) override;

    std::list<PTraceCall> m_attach_calls;
    PTraceCall m_link_call;
    std::list<PTraceCall> m_data_calls;
    std::map<unsigned, std::list<PTraceCall>> m_uniforms_calls;
    std::unordered_set<PGenObject> m_attached_shaders;
    std::unordered_set<unsigned> m_bound_attributes;
};

class ProgramObjectMap : public CreateObjectMap<ProgramObject> {
public:
    PTraceCall attach_shader(trace::Call& call, const ShaderObjectMap& shaders);
    PTraceCall bind_attr_location(trace::Call& call);
    PTraceCall vertex_attr_pointer(trace::Call& call, BufObjectMap& buffers);
    PTraceCall data(trace::Call& call);
    PTraceCall link(trace::Call& call);
    PTraceCall uniform(trace::Call& call);
private:
    unsigned target_id_from_call(const trace::Call& call) const override;
};

class LegacyProgramObjectMap : public GenObjectMap<ShaderObject> {
public:
    PGenObject gen_from_bind_call(const trace::Call& call);
    PTraceCall program_string(const trace::Call& call);

};

}

#endif // PROGRAMOBJ_HPP
