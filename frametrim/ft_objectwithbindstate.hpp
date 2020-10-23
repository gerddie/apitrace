#ifndef OBJECTWITHBINDSTATE_HPP
#define OBJECTWITHBINDSTATE_HPP

#include "ft_objectstate.hpp"

namespace frametrim {

class FramebufferState;

class ObjectWithBindState : public ObjectState
{
public:
    ObjectWithBindState(GLint glID, PTraceCall call);
    ObjectWithBindState(GLint glID);

    void bind(unsigned target, PTraceCall call);
    void unbind(PTraceCall call);
    bool bound() const;

    PTraceCall bind_call() const { return m_bind_call;}
protected:
    void emit_bind(CallSet& out_list) const;

private:
    bool is_active() const override;
    virtual void post_bind(unsigned target, const PTraceCall& call);
    virtual void post_unbind(const PTraceCall& call);
    void do_emit_calls_to_list(CallSet &list) const override;
    void post_set_state_call(PTraceCall call) override;

    bool m_bound;
    PTraceCall m_bind_call;
    bool m_bound_dirty;
};

template <typename T>
class TObjectWithBindStateMap: public TObjStateMap<T>  {
public:
    using TObjStateMap<T>::TObjStateMap;

    void bind_target(unsigned target, unsigned id, PTraceCall call) {

        if (id > 0) {
            if (m_bound_objects[target] &&
                m_bound_objects[target]->id() != id)
                m_bound_objects[target]->unbind(call);

            auto obj = this->get_by_id(id);
            if (!obj) {
                std::cerr << "Object " << id << " not found while binding\n";
                return;
            }

            obj->bind(target, call);
            m_bound_objects[target] = obj;
            post_bind(target, m_bound_objects[target]);
            m_unbind_calls.erase(target);
        } else {
            post_bind(target, nullptr);
            if (m_bound_objects[target])  {
                m_bound_objects[target]->unbind(call);
                post_unbind(target, call);
            }
            m_bound_objects[target] = nullptr;
            m_unbind_calls[target] = call;
        }
    }

    PTraceCall bind(const trace::Call& call, unsigned obj_id_index,
                    FramebufferState& fbo) {
        auto target = obj_id_index > 0 ? target_id_from_call(call) : 0;
        auto id = call.arg(obj_id_index).toUInt();
        PTraceCall c = trace2call(call);

        bind_target(target, id, c);

        if (m_bound_objects[target])
            m_bound_objects[target]->flush_state_cache(fbo);

        return c;
    }

    typename T::Pointer bound_to(unsigned target) {
        target = composed_target_id(target);
        return m_bound_objects[target];
    }

    typename T::Pointer bound_in_call(const trace::Call& call) {
        unsigned target = target_id_from_call(call);
        return m_bound_objects[target];
    }

    void set_state(const trace::Call& call, unsigned addr_params) {
        auto obj = bound_in_call(call);
        if (!obj) {
            std::cerr << "No obj found in call " << call.no
                      << " target:" << call.arg(0).toUInt();
            assert(0);
        }
        obj->set_state_call(call, addr_params);
    }

private:
    virtual void post_bind(unsigned target, typename T::Pointer obj) {
        (void)target;
        (void)obj;
    }

    virtual void post_unbind(unsigned target, PTraceCall call){
        (void)target;
        (void)call;
    }

    void emit_unbind_calls(CallSet& list) const override {
        for (auto&& [key, call] : m_unbind_calls) {
            if (call)
                list.insert(call);
        }
    }

    void do_emit_calls_to_list(CallSet& list) const override {
        for (auto& obj: m_bound_objects) {
            if (obj.second)
                obj.second->emit_calls_to_list(list);
        }
    }

    virtual unsigned target_id_from_call(const trace::Call& call) const {
        return composed_target_id(call.arg(0).toUInt());
    }

    virtual unsigned composed_target_id(unsigned id) const {
        return id;
    }

    std::unordered_map<unsigned, typename T::Pointer> m_bound_objects;
    std::unordered_map<unsigned, PTraceCall> m_unbind_calls;
};

}

#endif // OBJECTWITHBINDSTATE_HPP
