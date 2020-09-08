#ifndef TEXOBJECT_HPP
#define TEXOBJECT_HPP

#include "ftr_boundobject.hpp"
#include "ftr_attachableobject.hpp"

#include <forward_list>
namespace frametrim_reverse {

class TexObject : public AttachableObject {
public:
    using AttachableObject::AttachableObject;
    using Pointer = std::shared_ptr<TexObject>;
    void state(const trace::Call& call, unsigned nparam);
private:
    unsigned evaluate_size(const trace::Call& call) override;
    void collect_state_calls(CallIdSet& calls, unsigned call_before) override;

    std::forward_list<std::pair<unsigned, std::string>> m_state_calls;
};

using PTexObject = TexObject::Pointer;

class TexObjectMap : public GenObjectMap<TexObject> {
public:
    TexObjectMap();
    PTraceCall active_texture(const trace::Call& call);
    PTraceCall bind_multitex(const trace::Call& call);
    PTraceCall allocation(const trace::Call& call);
    PTraceCall state(const trace::Call& call, unsigned num_param);
private:
    unsigned target_id_from_call(const trace::Call& call) const override;
    unsigned compose_target_id_with_unit(unsigned target,
                                         unsigned unit) const;

    unsigned m_active_texture_unit;
};


}

#endif // TEXOBJECT_HPP
