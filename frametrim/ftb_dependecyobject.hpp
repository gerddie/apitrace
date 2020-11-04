#ifndef DEPENDECYOBJECT_HPP
#define DEPENDECYOBJECT_HPP

#include "ft_tracecall.hpp"

namespace frametrim {

class DependecyObject
{
public:
    using Pointer = std::shared_ptr<DependecyObject>;

    DependecyObject(unsigned id);

    unsigned id() const;

    void add_call(PTraceCall call);
    void add_depenency(Pointer dep);

    void append_calls(CallSet& out_list);

private:

    std::vector<PTraceCall> m_calls;
    std::vector<Pointer> m_dependencies;
    unsigned m_id;

};

class DependecyObjectMap {
public:
    PTraceCall Generate(const trace::Call& call);
    PTraceCall Destroy(const trace::Call& call);

    PTraceCall Create(const trace::Call& call);
    PTraceCall Delete(const trace::Call& call);

    DependecyObject::Pointer Bind(const trace::Call& call, unsigned obj_id_param);
    PTraceCall CallOnBoundObject(const trace::Call& call);
    PTraceCall CallOnObjectBoundTo(const trace::Call& call, unsigned bindpoint);
    PTraceCall CallOnNamedObject(const trace::Call& call);
    PTraceCall CallOnBoundObjectWithDep(const trace::Call& call, int dep_obj_param,
                                        DependecyObjectMap& other_objects);
    PTraceCall CallOnNamedObjectWithDep(const trace::Call& call, int dep_obj_param,
                                        DependecyObjectMap& other_objects);


    DependecyObject::Pointer get_by_id(unsigned id) const;
    void add_call(PTraceCall call);

    void emit_bound_objects(CallSet& out_calls);
protected:
    DependecyObject::Pointer bind(unsigned bindpoint, unsigned id);
    DependecyObject::Pointer bound_to(unsigned bindpoint);
    void add_object(unsigned id, DependecyObject::Pointer obj);
private:
    virtual DependecyObject::Pointer bind_target(unsigned id, unsigned bindpoint);
    virtual unsigned get_bindpoint_from_call(const trace::Call& call) const = 0;

    std::unordered_map<unsigned, DependecyObject::Pointer> m_objects;
    std::unordered_map<unsigned, DependecyObject::Pointer> m_bound_object;

    std::vector<PTraceCall> m_calls;
};

class DependecyObjectWithSingleBindPointMap: public DependecyObjectMap {
private:
    unsigned get_bindpoint_from_call(const trace::Call& call) const override;
};

class DependecyObjectWithDefaultBindPointMap: public DependecyObjectMap {
private:
    unsigned get_bindpoint_from_call(const trace::Call& call) const override;
};

class BufferObjectMap: public DependecyObjectMap {
public:
    PTraceCall data(const trace::Call& call);
    PTraceCall map(const trace::Call& call);
    PTraceCall map_range(const trace::Call& call);
    PTraceCall unmap(const trace::Call& call);
    PTraceCall memcopy(const trace::Call& call);

private:
    unsigned get_bindpoint_from_call(const trace::Call& call) const override;

    std::unordered_map<unsigned,
        std::unordered_map<unsigned, DependecyObject::Pointer>> m_mapped_buffers;

    std::unordered_map<unsigned, unsigned> m_buffer_sizes;
    std::unordered_map<unsigned, std::pair<uint64_t, uint64_t>> m_buffer_mappings;

};

class TextureObjectMap: public DependecyObjectMap {
public:
    TextureObjectMap();
    PTraceCall ActiveTexture(const trace::Call& call);

private:
    unsigned get_bindpoint_from_call(const trace::Call& call) const override;
    unsigned get_bindpoint_from_target_and_unit(unsigned target, unsigned unit) const;
    unsigned m_active_texture;
};

class FramebufferObjectMap: public DependecyObjectMap {
public:
    FramebufferObjectMap();
    PTraceCall Blit(const trace::Call& call);
private:
    DependecyObject::Pointer
    bind_target(unsigned id, unsigned bindpoint) override;
    unsigned get_bindpoint_from_call(const trace::Call& call) const override;
};

}

#endif // DEPENDECYOBJECT_HPP
