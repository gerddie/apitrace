#ifndef TEXOBJECT_HPP
#define TEXOBJECT_HPP

#include "ftr_boundobject.hpp"

namespace frametrim_reverse {

class TexObject : public GenObject
{
public:
    using GenObject::GenObject;
};

class TexObjectMap : public GenObjectMap<TexObject> {
public:
    TexObjectMap();
    PTraceCall active_texture(const trace::Call& call);
    PTraceCall bind_multitex(const trace::Call& call);

private:
    unsigned target_id_from_call(const trace::Call& call) const override;
    unsigned compose_target_id_with_unit(unsigned target,
                                         unsigned unit) const;

    unsigned m_active_texture_unit;
};


}

#endif // TEXOBJECT_HPP
