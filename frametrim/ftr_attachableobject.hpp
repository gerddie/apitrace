#ifndef ATTACHABLEOBJECT_HPP
#define ATTACHABLEOBJECT_HPP

#include "ftr_boundobject.hpp"

namespace frametrim_reverse {

class AttachableObject : public GenObject {
public:
    using Pointer = std::shared_ptr<AttachableObject>;
    using GenObject::GenObject;

    void allocate(const trace::Call& call);
    void attach_to(PGenObject obj);
    void detach_from(unsigned fbo_id);

    unsigned width(unsigned level) const { return m_width[level];}
    unsigned heigth(unsigned level) const { return m_heigth[level];}
protected:
    void set_size(unsigned level, unsigned w, unsigned h);
private:
    virtual void evaluate_size(const trace::Call& call) = 0;

    std::vector<unsigned> m_width;
    std::vector<unsigned> m_heigth;
    unsigned m_allocation_call;

    std::unordered_map<unsigned, PGenObject> m_attached_to;
};

using PAttachableObject = AttachableObject::Pointer;

}

#endif // ATTACHABLEOBJECT_HPP
