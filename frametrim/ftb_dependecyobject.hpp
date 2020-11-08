#ifndef DEPENDECYOBJECT_HPP
#define DEPENDECYOBJECT_HPP

#include "ft_tracecall.hpp"

namespace frametrim {

class UsedObject {
public:
    using Pointer = std::shared_ptr<UsedObject>;

    UsedObject(unsigned id);

    unsigned id() const;

    void add_call(PTraceCall call);
    void set_call(PTraceCall call);

    void add_depenency(Pointer dep);
    void set_depenency(Pointer dep);

    void emit_calls_to(CallSet& out_list);
    bool emitted() const;

private:

    std::vector<PTraceCall> m_calls;
    std::vector<Pointer> m_dependencies;
    unsigned m_id;
    bool m_emitted;
};

class DependecyObjectMap {
public:
    void Generate(const trace::Call& call);
    void Destroy(const trace::Call& call);

    void Create(const trace::Call& call);
    void Delete(const trace::Call& call);

    UsedObject::Pointer Bind(const trace::Call& call, unsigned obj_id_param);
    void CallOnBoundObject(const trace::Call& call);
    UsedObject::Pointer BindWithCreate(const trace::Call& call, unsigned obj_id_param);
    void CallOnObjectBoundTo(const trace::Call& call, unsigned bindpoint);
    void CallOnNamedObject(const trace::Call& call);
    UsedObject::Pointer
    CallOnBoundObjectWithDep(const trace::Call& call,
                             DependecyObjectMap& other_objects, int dep_obj_param, bool reverse_dep_too);

    void
    CallOnBoundObjectWithDepBoundTo(const trace::Call& call, DependecyObjectMap& other_objects,
                                    int bindingpoint);

    void CallOnNamedObjectWithDep(const trace::Call& call,
                                  DependecyObjectMap& other_objects, int dep_obj_param, bool reverse_dep_too);

    UsedObject::Pointer get_by_id(unsigned id) const;
    void add_call(PTraceCall call);

    void emit_bound_objects(CallSet& out_calls);
    UsedObject::Pointer bound_to(unsigned target, unsigned index = 0);
protected:
    using ObjectMap=std::unordered_map<unsigned, UsedObject::Pointer>;

    UsedObject::Pointer bind(unsigned bindpoint, unsigned id);
    void add_object(unsigned id, UsedObject::Pointer obj);
    UsedObject::Pointer at_binding(unsigned index);

     ObjectMap::iterator begin();
     ObjectMap::iterator end();

private:

    virtual void emit_bound_objects_ext(CallSet& out_calls);
    virtual UsedObject::Pointer bind_target(unsigned id, unsigned bindpoint);
    virtual unsigned get_bindpoint_from_call(const trace::Call& call) const = 0;
    virtual unsigned get_bindpoint(unsigned target, unsigned index) const;

    ObjectMap m_objects;
    ObjectMap m_bound_object;

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
    void data(const trace::Call& call);
    void map(const trace::Call& call);
    void map_range(const trace::Call& call);
    void unmap(const trace::Call& call);
    void memcopy(const trace::Call& call);

    UsedObject::Pointer bound_to_target(unsigned target, unsigned index = 0);

    void add_ssbo_dependencies(UsedObject::Pointer dep);

private:
    unsigned get_bindpoint_from_call(const trace::Call& call) const override;

    unsigned get_bindpoint(unsigned target, unsigned index) const override;

    std::unordered_map<unsigned,
        std::unordered_map<unsigned, UsedObject::Pointer>> m_mapped_buffers;

    std::unordered_map<unsigned, unsigned> m_buffer_sizes;
    std::unordered_map<unsigned, std::pair<uint64_t, uint64_t>> m_buffer_mappings;

};

class VertexAttribObjectMap: public DependecyObjectWithDefaultBindPointMap {
public:
    void BindAVO(const trace::Call& call, BufferObjectMap& buffers);
    void BindVAOBuf(const trace::Call& call, BufferObjectMap& buffers, CallSet &out_list, bool emit_dependencies);
};

class TextureObjectMap: public DependecyObjectMap {
public:
    TextureObjectMap();
    void ActiveTexture(const trace::Call& call);
    UsedObject::Pointer BindMultitex(const trace::Call& call);
    void Copy(const trace::Call& call);
    void BindToImageUnit(const trace::Call& call);
    void add_image_dependencies(UsedObject::Pointer dep);
private:
    void emit_bound_objects_ext(CallSet& out_calls) override;
    unsigned get_bindpoint_from_call(const trace::Call& call) const override;
    unsigned get_bindpoint_from_target_and_unit(unsigned target, unsigned unit) const;
    unsigned m_active_texture;
    std::unordered_map<unsigned, UsedObject::Pointer> m_bound_images;
};

class FramebufferObjectMap: public DependecyObjectMap {
public:
    FramebufferObjectMap();
    void Blit(const trace::Call& call);
    void ReadBuffer(const trace::Call& call);
    void DrawFromBuffer(const trace::Call& call, BufferObjectMap &buffers);
private:
    UsedObject::Pointer
    bind_target(unsigned id, unsigned bindpoint) override;
    unsigned get_bindpoint_from_call(const trace::Call& call) const override;
};

}

#endif // DEPENDECYOBJECT_HPP
