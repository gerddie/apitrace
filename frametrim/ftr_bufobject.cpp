#include "ftr_bufobject.hpp"
#include "ftr_tracecallext.hpp"
#include "ftr_boundobject.hpp"


namespace frametrim_reverse {

using std::make_shared;

void BufObject::data(trace::Call& call)
{
    m_size = call.arg(1).toUInt();
}

void BufObject::map(trace::Call& call)
{
    m_mapped_range.first = call.ret->toUInt();
    m_mapped_range.second = m_mapped_range.first + m_size;
}

void BufObject::map_range(trace::Call& call)
{
    m_mapped_range.first = call.ret->toUInt();
    m_mapped_range.second = m_mapped_range.first + call.arg(2).toUInt();
}

void BufObject::unmap(trace::Call& call)
{
    (void)call;
    m_mapped_range.first = m_mapped_range.second = 0;
}

bool BufObject::address_in_mapped_range(uint64_t addr) const
{
    return addr >= m_mapped_range.first &&
            addr < m_mapped_range.second;
}

#define FORWARD_Call(FUNC) \
    PTraceCall BufObjectMap:: FUNC (trace::Call& call) \
    { \
        PBufObject obj = this->bound_to_call_target(call); \
        obj-> FUNC (call); \
        return make_shared<TraceCallOnBoundObj>(call, obj); \
    }

FORWARD_Call(data)
FORWARD_Call(map)
FORWARD_Call(map_range)
FORWARD_Call(unmap)

PTraceCall BufObjectMap::memcopy(trace::Call& call)
{
    auto dest_addr = call.arg(0).toUInt();
    for (auto& buf : m_mapped_buffers) {
        if (buf->address_in_mapped_range(dest_addr)) {
            return make_shared<TraceCallOnBoundObj>(call, buf);
        }
    }
    std::cerr << "Memcpy: no mapped buffer found for addess " << dest_addr << "\n";
    return make_shared<TraceCallOnBoundObj>(call, nullptr);
}

}
