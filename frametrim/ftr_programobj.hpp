#ifndef PROGRAMOBJ_HPP
#define PROGRAMOBJ_HPP

#include "ftr_boundobject.hpp"
#include "ftr_bufobject.hpp"

#include <unordered_set>
#include <unordered_map>

namespace frametrim_reverse {

class ShaderObject : public GenObject {
public:
    using GenObject::GenObject;
    using Pointer = std::shared_ptr<ShaderObject>;
    void source(const trace::Call& call);
    void compile(const trace::Call& call);
private:
    void collect_data_calls(CallIdSet& calls, unsigned call_before) override;
    unsigned m_source_call;
    unsigned m_compile_call;
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
    void attach_shader(const trace::Call& call, PGenObject shader);
    void bind_attr_location(unsigned loc);
    void bind_attr_pointer(unsigned callid, unsigned attr_id, PBufObject buf);
    void data_call(const trace::Call& call);
    void link(const trace::Call& call);
    void uniform(const trace::Call& call);
private:
    void collect_dependend_obj(Queue& objects) override;
    void collect_data_calls(CallIdSet& calls, unsigned call_before) override;

    std::vector<unsigned> m_attach_calls;
    unsigned m_link_call;
    std::vector<unsigned> m_data_calls;
    std::map<unsigned, std::vector<unsigned>> m_uniforms_calls;
    std::unordered_set<PGenObject> m_attached_shaders;
    std::unordered_set<unsigned> m_bound_attributes;
    std::unordered_map<unsigned, PBufObject> m_bound_attr_buffers;
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

}

#endif // PROGRAMOBJ_HPP
