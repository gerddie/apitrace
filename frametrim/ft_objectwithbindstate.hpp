#ifndef OBJECTWITHBINDSTATE_HPP
#define OBJECTWITHBINDSTATE_HPP

#include "ft_objectstate.hpp"

namespace frametrim {


class ObjectWithBindState : public ObjectState
{
public:
    ObjectWithBindState(GLint glID, PCall call);

    void bind(PCall call);
    void unbind(PCall call);

    bool bound() const;

protected:
    void emit_bind(CallSet& out_list) const;

private:
    bool is_active() const override;
    virtual void post_bind(PCall call);
    virtual void post_unbind(PCall call);

    bool m_bound;
    PCall m_bind_call;
};

template <typename T>
class TObjectWithBindStateMap: public TObjStateMap<T>  {
public:
    using TObjStateMap<T>::TObjStateMap;

    void bind(PCall call) {
        auto target = target_id_from_call(call);
        auto id = call->arg(1).toUInt();

        if (id > 0) {
            if (!m_bound_objects[target] ||
                    m_bound_objects[target]->id() != id) {

                if (m_bound_objects[target])
                            m_bound_objects[target]->unbind(call);

                auto obj = this->get_by_id(id);
                if (!obj) {
                    std::cerr << "Object " << id << " not found while binding\n";
                    return;
                }

                obj->bind(call);
                m_bound_objects[target] = obj;
                post_bind(call, m_bound_objects[target]);

                auto& gs = this->global_state();

                if (gs.in_target_frame()) {
                    obj->emit_calls_to_list(gs.global_callset());
                }
                auto draw_fb = gs.draw_framebuffer();
                if (draw_fb)
                    obj->emit_calls_to_list(draw_fb->dependent_calls());

            }
        } else {
            if (m_bound_objects[target])  {
                m_bound_objects[target]->unbind(call);
                post_unbind(call, m_bound_objects[target]);
            }
            m_bound_objects[target] = nullptr;
        }
    }

    typename T::Pointer bound_to(unsigned target) {
        target = composed_target_id(target);
        return m_bound_objects[target];
    }

    typename T::Pointer bound_in_call(const PCall& call) {
        unsigned target = target_id_from_call(call);
        return m_bound_objects[target];
    }

    void set_state(PCall call, unsigned addr_params) {
        auto obj = bound_in_call(call);
        if (!obj) {
            std::cerr << "No obj found in call " << call->no
                      << " target:" << call->arg(0).toUInt();
            assert(0);
        }
        obj->set_state_call(call, addr_params);
    }

private:
    virtual void post_bind(PCall call, typename T::Pointer obj) {
        (void)call;
        (void)obj;
    }

    virtual void post_unbind(PCall call, typename T::Pointer obj){
        (void)call;
        (void)obj;
    }

    void do_emit_calls_to_list(CallSet& list) const override {
        for (auto& obj: m_bound_objects) {
            if (obj.second)
                obj.second->emit_calls_to_list(list);
        }
    }

    virtual unsigned target_id_from_call(const PCall& call) const {
        return composed_target_id(call->arg(0).toUInt());
    }

    virtual unsigned composed_target_id(unsigned id) const {
        return id;
    }

    std::unordered_map<unsigned, typename T::Pointer> m_bound_objects;
};

}

#endif // OBJECTWITHBINDSTATE_HPP
