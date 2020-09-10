#include "ftr_bufobject.hpp"
#include "ftr_tracecallext.hpp"
#include "ftr_boundobject.hpp"


namespace frametrim_reverse {

using std::make_shared;

PTraceCall BufObject::data(trace::Call& call)
{
    m_size = call.arg(1).toUInt();
    m_allocation_call = make_shared<TraceCall>(call);
    return m_allocation_call;
}

PTraceCall BufObject::sub_data(trace::Call& call)
{
    unsigned start = call.arg(1).toUInt();
    unsigned end = call.arg(2).toUInt() + start;
    auto data_call = make_shared<BufferSubrangeCall>(call, start, end);
    m_sub_data_calls.push_front(data_call);
    return data_call;
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

struct BufferSubRange {

    BufferSubRange() = default;
    BufferSubRange(uint64_t start, uint64_t end);
    bool can_merge(const BufferSubRange& range) const;
    bool included_in(const BufferSubRange& range) const;
    void merge(const BufferSubRange& range);

    uint64_t m_start;
    uint64_t m_end;
};

class Subrangemerger {
public:
    Subrangemerger(uint64_t size);

    bool merge(uint64_t start, uint64_t end);

private:
    uint64_t m_size;
    bool m_all_written;
    std::vector<BufferSubRange> m_subranges;
};

void
BufObject::collect_data_calls(CallIdSet& calls, unsigned call_before)
{
    unsigned required_bind_before = call_before;
    if (m_allocation_call->call_no() < call_before) {
        calls.insert(*m_allocation_call);
        required_bind_before = m_allocation_call->call_no();
    }
    collect_last_call_before(calls, m_bind_calls,
                             required_bind_before);

    Subrangemerger merger(m_size);
    for(auto&& c : m_sub_data_calls) {
        if (c->call_no() >= call_before)
            continue;
        if (merger.merge(c->start(), c->end())) {
            required_bind_before = c->call_no();
            calls.insert(*c);
        }
    }
    collect_last_call_before(calls, m_bind_calls,
                             required_bind_before);
}

Subrangemerger::Subrangemerger(uint64_t size):
    m_size(size),
    m_all_written(false)
{

}

bool Subrangemerger::merge(uint64_t start, uint64_t end)
{
    BufferSubRange bsr(start, end);

    for(auto& b : m_subranges) {
        if (b.included_in(bsr))
            return false;
        if (b.can_merge(bsr)) {
            b.merge(bsr);
            return true;
        }
    }
    m_subranges.push_back(bsr);
    /* TODO: merge ajacent subranges, so that in the end we have one
     * big range */
    return true;
}

PTraceCall BufObjectMap::map (trace::Call& call)
{
    PBufObject obj = this->bound_to_call_target(call);
    obj->map(call);
    m_mapped_buffers.insert(obj);
    return make_shared<TraceCallOnBoundObj>(call, obj); \
}

PTraceCall BufObjectMap::map_range (trace::Call& call)
{
    PBufObject obj = this->bound_to_call_target(call);
    obj->map_range(call);
    m_mapped_buffers.insert(obj);
    return make_shared<TraceCallOnBoundObj>(call, obj); \
}

PTraceCall BufObjectMap::unmap(trace::Call& call)
{
    PBufObject obj = this->bound_to_call_target(call);
    obj->unmap(call);
    m_mapped_buffers.erase(obj);
    return make_shared<TraceCallOnBoundObj>(call, obj); \
}

PTraceCall BufObjectMap::data (trace::Call& call)
{
    PBufObject obj = this->bound_to_call_target(call);
    return obj->data(call);
}

PTraceCall BufObjectMap::sub_data(trace::Call& call)
{
    PBufObject obj = this->bound_to_call_target(call);
    assert(obj);
    return obj->sub_data(call);
}


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

BufferSubRange::BufferSubRange(uint64_t start, uint64_t end):
    m_start(start), m_end(end)
{
}

bool BufferSubRange::can_merge(const BufferSubRange& range) const
{
    // m_end is one past the range
    return  (m_start <= range.m_end || range.m_start <= m_end );
}

bool BufferSubRange::included_in(const BufferSubRange& range) const
{
    return (m_start <= range.m_start && m_end >= range.m_end);
}

void BufferSubRange::merge(const BufferSubRange& range)
{
    m_start =std::min(m_start, range.m_start);
    m_end = std::max(m_end, range.m_end);

}

}
