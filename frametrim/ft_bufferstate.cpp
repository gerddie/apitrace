#include "ft_bufferstate.hpp"
#include <limits>

namespace frametrim {

using std::pair;
using std::make_pair;

struct BufferSubRange {

    BufferSubRange() = default;
    BufferSubRange(uint64_t start, uint64_t end, PCall call, bool memcopy);
    bool overlap_with(const BufferSubRange& range) const;
    bool split(const BufferSubRange& range, BufferSubRange& second_split);

    uint64_t m_start;
    uint64_t m_end;
    PCall m_call;
    bool m_is_memcopy;
};

struct BufferMap {

    BufferMap();
    BufferMap(PCall call);

    bool contains(uint64_t start);
    void append_copy_call(PCall call);

    uint64_t buffer_base;
    uint64_t range_begin;
    uint64_t range_end;
};

struct BufferStateImpl {

    PCall m_last_bind_call;
    CallSet m_data_upload_set;
    CallSet m_data_use_set;
    bool m_last_bind_call_dirty;

    uint64_t m_buffer_size;

    CallSet m_sub_data_bind_calls;
    std::vector<BufferSubRange> m_sub_buffers;

    BufferMap m_mapping;

    std::vector<PCall> m_map_calls;
    std::vector<PCall> m_unmap_calls;

    void bind(PCall call);
    void data(PCall call);
    void append_data(PCall call);
    void map(PCall call);
    void map_range(PCall call);
    void memcopy(PCall call);
    void unmap(PCall call);
    bool in_mapped_range(uint64_t address) const;
    void add_sub_range(uint64_t start, uint64_t end, PCall call, bool memcopy);

    void use(PCall call = nullptr);
    void emit_calls_to_list(CallSet& list) const;
    CallSet clean_bind_calls() const;
};


#define FORWARD_CALL(method) \
    void BufferState:: method (PCall call) \
{ \
    impl->method(call); \
}

FORWARD_CALL(bind)
FORWARD_CALL(data)
FORWARD_CALL(append_data)
FORWARD_CALL(use)
FORWARD_CALL(map)
FORWARD_CALL(map_range)
FORWARD_CALL(memcopy)
FORWARD_CALL(unmap)

bool BufferState::in_mapped_range(uint64_t address) const
{
    return impl->m_mapping.contains(address);
}

void BufferState::do_emit_calls_to_list(CallSet& list) const
{
    if (!impl->m_data_use_set.empty()) {
        emit_gen_call(list);
        impl->emit_calls_to_list(list);
    }
}

BufferState::BufferState(GLint glID, PCall gen_call):
    GenObjectState(glID, gen_call)
{
    impl = new BufferStateImpl;
}

BufferState::~BufferState()
{
    delete impl;
}

void BufferStateImpl::bind(PCall call)
{
    m_last_bind_call = call;
    m_last_bind_call_dirty = true;
}

void BufferStateImpl::data(PCall call)
{
    m_data_upload_set.clear();

    m_data_upload_set.insert(m_last_bind_call);
    m_last_bind_call_dirty = false;
    m_data_upload_set.insert(call);
    m_sub_buffers.clear();

    m_buffer_size = call->arg(1).toUInt();
}

void BufferStateImpl::append_data(PCall call)
{
    if (m_last_bind_call_dirty) {
        m_sub_data_bind_calls.insert(m_last_bind_call);
        m_last_bind_call_dirty = false;
    }

    uint64_t start = call->arg(0).toUInt();
    uint64_t end =  start + call->arg(1).toUInt();

    add_sub_range(start, end, call, false);
}

void BufferStateImpl::add_sub_range(uint64_t start, uint64_t end, PCall call, bool memcopy)
{
    BufferSubRange bsr(start, end, call, memcopy);

    for(auto& b : m_sub_buffers) {
        if (b.overlap_with(bsr)) {
            BufferSubRange split;
            if (b.split(bsr, split)) {
                m_sub_buffers.push_back(split);
            }
        }
    }
    m_sub_buffers.push_back(bsr);

    auto b = m_sub_buffers.end();
    do {
        --b;
        if (b->m_start >= b->m_end)
            m_sub_buffers.erase(b);

    } while (b != m_sub_buffers.begin());
}

void BufferStateImpl::use(PCall call)
{
    m_data_use_set.clear();
    m_data_use_set.insert(m_last_bind_call);
    if (call)
        m_data_use_set.insert(call);
}

void BufferStateImpl::emit_calls_to_list(CallSet& list) const
{
    list.insert(clean_bind_calls());
    list.insert(m_data_upload_set);
    list.insert(m_data_use_set);
}

CallSet BufferStateImpl::clean_bind_calls() const
{
    unsigned oldest_sub_buffer_call = std::numeric_limits<unsigned>::max();
    unsigned oldest_memcopy_call = std::numeric_limits<unsigned>::max();

    for(auto& b : m_sub_buffers) {
        if (oldest_sub_buffer_call > b.m_call->no)
            oldest_sub_buffer_call = b.m_call->no;
        if (b.m_is_memcopy)
            if (oldest_memcopy_call > b.m_call->no)
                oldest_memcopy_call = b.m_call->no;
    }

    unsigned first_needed_bind_call_no = 0;
    for(auto& b : m_sub_data_bind_calls) {
        if (b->no < oldest_sub_buffer_call &&
                b->no > first_needed_bind_call_no)
            first_needed_bind_call_no = b->no;
    }

    unsigned first_needed_map_call_no = 0;
    for(auto& b : m_map_calls) {
        if (b->no < oldest_memcopy_call &&
                b->no > first_needed_map_call_no)
            first_needed_map_call_no = b->no;
    }

    CallSet retval;
    for (auto b: m_sub_data_bind_calls) {
        if (b->no > first_needed_bind_call_no)
            retval.insert(b);
    }

    for(auto& b : m_map_calls) {
        if (b->no > first_needed_map_call_no)
            retval.insert(b);
    }

    for(auto& b : m_unmap_calls) {
        if (b->no > first_needed_map_call_no)
            retval.insert(b);
    }

    return retval;
}

void BufferStateImpl::map(PCall call)
{
    if (m_last_bind_call_dirty) {
        m_sub_data_bind_calls.insert(m_last_bind_call);
        m_last_bind_call_dirty = false;
    }

    assert(0);
    m_mapping.range_begin = m_mapping.buffer_base = call->ret->toUInt();
    m_mapping.range_end = m_mapping.range_begin + call->arg(0).toUInt();
    m_map_calls.push_back(call);
}


void BufferStateImpl::map_range(PCall call)
{
    if (m_last_bind_call_dirty) {
        m_sub_data_bind_calls.insert(m_last_bind_call);
        m_last_bind_call_dirty = false;
    }
    m_mapping = BufferMap(call);
    m_map_calls.push_back(call);
}

void BufferStateImpl::memcopy(PCall call)
{
    // @remark: we track only copies TO buffers, have to see whether we
    // actually also need to track copying FROM buffers

    uint64_t start_ptr = call->arg(0).toUInt();
    assert(m_mapping.contains(start_ptr));

    uint64_t start = start_ptr - m_mapping.buffer_base;
    uint64_t end = call->arg(2).toUInt() + start;

    add_sub_range(start, end, call, true);
}

void BufferStateImpl::unmap(PCall call)
{
    m_unmap_calls.push_back(call);

    m_mapping.buffer_base =
            m_mapping.range_begin =
            m_mapping.range_end = 0;

}

BufferSubRange::BufferSubRange(uint64_t start, uint64_t end, PCall call, bool memcopy):
    m_start(start), m_end(end), m_call(call), m_is_memcopy(memcopy)
{
}

bool BufferSubRange::overlap_with(const BufferSubRange& range) const
{
    // m_end is one past the range
    if (m_start >= range.m_end || m_end <= range.m_start)
        return false;
    return true;
}

bool BufferSubRange::split(const BufferSubRange& range, BufferSubRange& second_split)
{
    bool retval = false;
    if (m_start <= range.m_start) {
        // new range is inside the old range
        if (m_end > range.m_end)  {
            second_split.m_call = m_call;
            second_split.m_end = m_end;
            second_split.m_start = range.m_end;
            retval = true;
        }
        m_end = range.m_start;
    } else {
        m_start = range.m_end;
    }
    return retval;
}

BufferMap::BufferMap():
    buffer_base(0),
    range_begin(0),
    range_end(0)
{
}

BufferMap::BufferMap(PCall call)
{
    uint64_t offset = call->arg(1).toUInt();
    uint64_t size = call->arg(2).toUInt();
    uint64_t map = call->ret->toUInt();

    buffer_base = map - offset;
    range_begin = map;
    range_end = map + size;
}

bool BufferMap::contains(uint64_t start)
{
    return start >= range_begin && start < range_end;
}

}
