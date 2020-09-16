#ifndef TEXOBJECT_HPP
#define TEXOBJECT_HPP

#include "ftr_boundobject.hpp"
#include "ftr_attachableobject.hpp"

#include <list>
namespace frametrim_reverse {

class TexSubImageCall : public TraceCall {
public:
    TexSubImageCall(const trace::Call& call,
                    unsigned layer,
                    unsigned x, unsigned y, unsigned z,
                    unsigned w, unsigned h, unsigned d,
                    PGenObject read_buffer):
        TraceCall(call),
        m_level(layer),
        m_x(x),
        m_y(y),
        m_z(z),
        m_width(w),
        m_height(h),
        m_depth(d),
        m_read_buffer(read_buffer)
    {
    }

    unsigned m_level;
    unsigned m_x;
    unsigned m_y;
    unsigned m_z;
    unsigned m_width;
    unsigned m_height;
    unsigned m_depth;
    bool m_read_buffer;
};
using PTexSubImageCall = std::shared_ptr<TexSubImageCall>;

class TexObject : public AttachableObject {
public:
    TexObject(unsigned gl_id, PTraceCall gen_call);
    using Pointer = std::shared_ptr<TexObject>;
    PTraceCall state(const trace::Call& call, unsigned nparam);
    PTraceCall sub_image(const trace::Call& call, PGenObject read_buffer);
private:
    unsigned evaluate_size(const trace::Call& call) override;
    void collect_state_calls(CallSet& calls, unsigned call_before) override;
    void collect_data_calls(CallSet& calls, unsigned call_before) override;

    unsigned m_dimensions;
    unsigned m_max_level;
    std::list<std::pair<unsigned, PTraceCall>> m_state_calls;
    std::list<PTexSubImageCall> m_data_calls;
};

using PTexObject = TexObject::Pointer;

class TexObjectMap : public GenObjectMap<TexObject> {
public:
    TexObjectMap();
    PTraceCall active_texture(const trace::Call& call);
    PTraceCall bind_multitex(const trace::Call& call);
    PTraceCall sub_image(const trace::Call& call, PGenObject read_buffer);
    PTraceCall allocation(const trace::Call& call);
    PTraceCall state(const trace::Call& call, unsigned num_param);
    unsigned active_unit() const;
private:
    unsigned target_id_from_call(const trace::Call& call) const override;
    unsigned compose_target_id_with_unit(unsigned target,
                                         unsigned unit) const;

    unsigned m_active_texture_unit;
};

}

#endif // TEXOBJECT_HPP
