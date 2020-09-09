#ifndef ATTACHABLEOBJECT_HPP
#define ATTACHABLEOBJECT_HPP

#include "ftr_boundobject.hpp"

namespace frametrim_reverse {

class AttachableObject : public BoundObject {
public:
    using Pointer = std::shared_ptr<AttachableObject>;
    using BoundObject::BoundObject;

    PTraceCall allocate(const trace::Call& call);
    void attach_to(PGenObject obj);
    void detach_from(unsigned fbo_id);

    unsigned width(unsigned level) const { return m_width[level];}
    unsigned heigth(unsigned level) const { return m_heigth[level];}
protected:
    void set_size(unsigned level, unsigned w, unsigned h);
private:
    virtual unsigned evaluate_size(const trace::Call& call) = 0;
    void collect_allocation_call(CallIdSet& calls) override;
    void collect_dependend_obj(Queue& objects) override;

    std::vector<unsigned> m_width;
    std::vector<unsigned> m_heigth;
    std::vector<PTraceCall> m_allocation_call;

    std::unordered_map<unsigned, PGenObject> m_attached_to;
};

using PAttachableObject = AttachableObject::Pointer;

}

#endif // ATTACHABLEOBJECT_HPP
