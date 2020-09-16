#include "ftr_boundobject.hpp"

namespace frametrim_reverse {

void BoundObject::collect_bind_calls(CallSet& calls, unsigned call_before)
{
    collect_last_call_before(calls, m_bind_calls, call_before);
}

}
