#include "ft_bufferstate.hpp"
#include <limits>
#include <cstring>

#include <GL/gl.h>
#include <GL/glext.h>

namespace frametrim {

using std::pair;
using std::make_pair;

struct BufferSubRange {

    BufferSubRange() = default;
    BufferSubRange(uint64_t start, uint64_t end, PTraceCall call, bool memcopy);
    bool overlap_with(const BufferSubRange& range) const;
    bool split(const BufferSubRange& range, BufferSubRange& second_split);

    uint64_t m_start;
    uint64_t m_end;
    PTraceCall m_call;
    bool m_is_memcopy;
};

struct BufferMap {

    BufferMap();
    BufferMap(const trace::Call& call);

    bool contains(uint64_t start);
    void append_copy_call(const trace::Call& call);

    uint64_t buffer_base;
    uint64_t range_begin;
    uint64_t range_end;
};

struct BufferStateImpl {
    BufferState *m_owner;

    CallSet m_data_upload_set;
    CallSet m_data_use_set;
    bool m_last_bind_call_dirty;

    uint64_t m_buffer_size;

    CallSet m_sub_data_bind_calls;
    std::vector<BufferSubRange> m_sub_buffers;

    BufferMap m_mapping;

    std::vector<PTraceCall> m_map_calls;
    std::vector<PTraceCall> m_unmap_calls;

    std::unordered_map<unsigned, PTraceCall> m_last_bind_calls;

    BufferStateImpl(BufferState *owner);

    void post_bind(unsigned target, const PTraceCall& call);
    void post_unbind(const PTraceCall& call);
    PTraceCall data(const trace::Call& call);
    PTraceCall append_data(const trace::Call& call);
    PTraceCall map(const trace::Call& call);
    PTraceCall map_range(const trace::Call& call);
    PTraceCall memcopy(const trace::Call& call);
    PTraceCall unmap(const trace::Call& call);
    bool in_mapped_range(uint64_t address) const;
    PTraceCall add_sub_range(uint64_t start, uint64_t end, PTraceCall call, bool memcopy);

    PTraceCall use(const trace::Call& call);
    void emit_calls_to_list(CallSet& list) const;
    CallSet clean_bind_calls() const;
    PTraceCall flush(const trace::Call& call);

    PTraceCall bind_target_call(unsigned target) const;
};


#define FORWARD_CALL(method) \
    PTraceCall BufferState:: method (const trace::Call& call) \
{ \
    return impl->method(call); \
}

FORWARD_CALL(data)
FORWARD_CALL(append_data)
FORWARD_CALL(use)
FORWARD_CALL(map)
FORWARD_CALL(map_range)
FORWARD_CALL(memcopy)
FORWARD_CALL(unmap)
FORWARD_CALL(flush)

void BufferState::post_bind(unsigned target, const PTraceCall& call)
{    
    impl->post_bind(target, call);
}

void BufferState::post_unbind(const PTraceCall& call)
{
    impl->post_unbind(call);
}

bool BufferState::in_mapped_range(uint64_t address) const
{
    return impl->m_mapping.contains(address);
}

PTraceCall BufferState::bind_target_call(unsigned target) const
{
    unsigned real_target = BufferStateMap::composed_target_id(target, 0);
    return impl->m_last_bind_calls[real_target];
}

void BufferState::do_emit_calls_to_list(CallSet& list) const
{
    /* Todo : correct the use set, theer are probably some links
     * missing */
    if (true || !impl->m_data_use_set.empty()) {
        emit_gen_call(list);
        impl->emit_calls_to_list(list);
    }
}

BufferState::BufferState(GLint glID, PTraceCall gen_call):
    GenObjectState(glID, gen_call)
{
    impl = new BufferStateImpl(this);
}

BufferState::~BufferState()
{
    delete impl;
}

BufferStateImpl::BufferStateImpl(BufferState *owner):
    m_owner(owner)
{

}

void BufferStateImpl::post_bind(unsigned target, const PTraceCall& call)
{
    (void)call;
    m_last_bind_call_dirty = true;
    m_last_bind_calls[target] = call;
}

void BufferStateImpl::post_unbind(const PTraceCall& call)
{
    (void)call;
    m_last_bind_call_dirty = true;
}


PTraceCall BufferStateImpl::data(const trace::Call& call)
{
    auto c = trace2call(call);
    m_data_upload_set.clear();

    m_owner->emit_bind(m_data_upload_set);

    m_last_bind_call_dirty = false;
    m_data_upload_set.insert(c);
    m_sub_buffers.clear();

    m_buffer_size = call.arg(1).toUInt();
    m_owner->dirty_cache();
    return c;
}

PTraceCall BufferStateImpl::append_data(const trace::Call& call)
{
    if (m_last_bind_call_dirty) {
        m_owner->emit_bind(m_data_upload_set);
        m_last_bind_call_dirty = false;
    }

    uint64_t start = call.arg(0).toUInt();
    uint64_t end =  start + call.arg(1).toUInt();

    auto c = trace2call(call);
    add_sub_range(start, end, c, false);
    return c;
}

PTraceCall
BufferStateImpl::add_sub_range(uint64_t start, uint64_t end, PTraceCall call, bool memcopy)
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
    m_owner->dirty_cache();
    return call;
}

PTraceCall BufferStateImpl::flush(const trace::Call& call)
{
    auto c = trace2call(call);
    m_data_upload_set.insert(c);
    return c;
}

PTraceCall BufferStateImpl::use(const trace::Call& call)
{
    m_data_use_set.clear();
    m_owner->emit_bind(m_data_use_set);
    auto c = trace2call(call);
    m_data_use_set.insert(c);
    m_owner->dirty_cache();
    return c;
}

void BufferStateImpl::emit_calls_to_list(CallSet& list) const
{
    list.insert(clean_bind_calls());
    list.insert(m_data_upload_set);
    /* TODO: for some reason the initial data
       call is not always submitted  */
    list.insert(m_data_use_set);

}

CallSet BufferStateImpl::clean_bind_calls() const
{
    unsigned oldest_sub_buffer_call = std::numeric_limits<unsigned>::max();
    unsigned oldest_memcopy_call = std::numeric_limits<unsigned>::max();

    CallSet retval;
    for(auto& b : m_sub_buffers) {
        if (oldest_sub_buffer_call > b.m_call->call_no())
            oldest_sub_buffer_call = b.m_call->call_no();
        if (b.m_is_memcopy)
            if (oldest_memcopy_call > b.m_call->call_no())
                oldest_memcopy_call = b.m_call->call_no();
        retval.insert(b.m_call);
    }

    unsigned first_needed_bind_call_no = 0;
    for(auto& b : m_sub_data_bind_calls) {
        if (b->call_no() < oldest_sub_buffer_call &&
            b->call_no() > first_needed_bind_call_no)
            first_needed_bind_call_no = b->call_no();
    }

    unsigned first_needed_map_call_no = 0;
    for(auto&& b : m_map_calls) {
        if (b->call_no() < oldest_memcopy_call &&
            b->call_no() > first_needed_map_call_no) {
            first_needed_map_call_no = b->call_no();
        }
    }

    for (auto&& b: m_sub_data_bind_calls) {
        if (b->call_no() >= first_needed_bind_call_no)
            retval.insert(b);
    }

    unsigned last_unmap_call = 0;

    for(auto&& b : m_unmap_calls) {
        if (b->call_no() > first_needed_map_call_no) {
            retval.insert(b);
            if (last_unmap_call < b->call_no())
                last_unmap_call = b->call_no();
        }
    }

    for(auto&& b : m_map_calls) {
        if (b->call_no() >= first_needed_map_call_no &&
            (last_unmap_call > b->call_no() ||
             b->test_flag(tc_persistent_mapping))) {
            retval.insert(b);
        }
    }

    return retval;
}

PTraceCall BufferStateImpl::map(const trace::Call& call)
{
    if (m_last_bind_call_dirty) {
        m_owner->emit_bind(m_sub_data_bind_calls);
        m_last_bind_call_dirty = false;
    }

    auto c = trace2call(call);

    if (call.arg(3).toUInt() & GL_MAP_PERSISTENT_BIT )
        c->set_flag(tc_persistent_mapping);

    m_mapping.range_begin = m_mapping.buffer_base = call.ret->toUInt();
    m_mapping.range_end = m_mapping.range_begin + call.arg(0).toUInt();

    m_map_calls.push_back(c);
    m_owner->dirty_cache();
    return c;
}


PTraceCall BufferStateImpl::map_range(const trace::Call& call)
{
    if (m_last_bind_call_dirty) {
        m_owner->emit_bind(m_sub_data_bind_calls);
        m_last_bind_call_dirty = false;
    }
    m_mapping = BufferMap(call);
    auto c = trace2call(call);
    m_map_calls.push_back(c);
    m_owner->dirty_cache();
    return c;
}

PTraceCall BufferStateImpl::memcopy(const trace::Call& call)
{
    // @remark: we track only copies TO buffers, have to see whether we
    // actually also need to track copying FROM buffers

    uint64_t start_ptr = call.arg(0).toUInt();
    assert(m_mapping.contains(start_ptr));

    uint64_t start = start_ptr - m_mapping.buffer_base;
    uint64_t end = call.arg(2).toUInt() + start;

    m_owner->dirty_cache();
    return add_sub_range(start, end, trace2call(call), true);
}

PTraceCall BufferStateImpl::unmap(const trace::Call& call)
{
    auto c = trace2call(call);

    if (m_last_bind_call_dirty)
        c->set_required_call(m_owner->bind_call());

    m_unmap_calls.push_back(c);

    m_mapping.buffer_base =
            m_mapping.range_begin =
            m_mapping.range_end = 0;
    m_owner->dirty_cache();
    return c;
}

BufferSubRange::BufferSubRange(uint64_t start, uint64_t end, PTraceCall call, bool memcopy):
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

BufferMap::BufferMap(const trace::Call& call)
{
    uint64_t offset = call.arg(1).toUInt();
    uint64_t size = call.arg(2).toUInt();
    uint64_t map = call.ret->toUInt();

    buffer_base = map - offset;
    range_begin = map;
    range_end = map + size;
}

bool BufferMap::contains(uint64_t start)
{
    return start >= range_begin && start < range_end;
}

PTraceCall BufferStateMap::data(const trace::Call& call)
{
    assert((call.arg(0).toUInt() != GL_PIXEL_UNPACK_BUFFER) &&
           "Copying texture data from a buffer is not yet implemented");
    bound_to(target_id_from_call(call))->data(call);
    return trace2call(call);
}

PTraceCall BufferStateMap::sub_data(const trace::Call& call)
{
    bound_to(target_id_from_call(call))->append_data(call);
    return trace2call(call);
}

PTraceCall BufferStateMap::map(const trace::Call& call)
{
    unsigned target = target_id_from_call(call);
    auto buf = bound_to(target);
    PTraceCall c = trace2call(call);
    if (buf) {
        m_mapped_buffers[target][buf->id()] = buf;
        c = buf->map(call);
    }
    return c;
}

PTraceCall BufferStateMap::flush(const trace::Call& call)
{
    unsigned target = target_id_from_call(call);
    auto buf = bound_to(target);
    PTraceCall c = trace2call(call);
    if (buf) {
        m_mapped_buffers[target][buf->id()] = buf;
        c = buf->flush(call);
    } else {
        std::cerr << "Error: no buffer mapped at " << target << " when flushing\n";
    }
    return c;
}


PTraceCall BufferStateMap::map_range(const trace::Call& call)
{
    unsigned target = target_id_from_call(call);
    auto buf = bound_to(target);
    PTraceCall c = trace2call(call);
    if (buf) {
        m_mapped_buffers[target][buf->id()] = buf;
        c = buf->map_range(call);
    }
    return c;
}

PTraceCall
BufferStateMap::memcpy(const trace::Call& call)
{
    auto dest_address = call.arg(0).toUInt();

    for(auto& bt : m_mapped_buffers) {
        for(auto& b: bt.second) {
            if (b.second->in_mapped_range(dest_address)) {
                return b.second->memcopy(call);
            }
        }
    }
    return trace2call(call);
}

PTraceCall
BufferStateMap::unmap(const trace::Call& call)
{
    unsigned target = target_id_from_call(call);
    auto buf = bound_to(target);
    auto c = trace2call(call);
    if (buf) {
        auto& mapped = m_mapped_buffers[target];
        c = buf->unmap(call);
        mapped.erase(buf->id());
    }
    return c;
}

unsigned BufferStateMap::composed_target_id(unsigned target, unsigned index)
{
    switch (target) {
    case GL_ARRAY_BUFFER:
        return 1;
    case GL_ATOMIC_COUNTER_BUFFER:
        return 2 + 16 * index;
    case GL_COPY_READ_BUFFER:
        return 3;
    case GL_COPY_WRITE_BUFFER:
        return 4;
    case GL_DISPATCH_INDIRECT_BUFFER:
        return 5;
    case GL_DRAW_INDIRECT_BUFFER:
        return 6;
    case GL_ELEMENT_ARRAY_BUFFER:
        return 7;
    case GL_PIXEL_PACK_BUFFER:
        return 8;
    case GL_PIXEL_UNPACK_BUFFER:
        return 9;
    case GL_QUERY_BUFFER:
        return 10;
    case GL_SHADER_STORAGE_BUFFER:
        return 11 + 16 * index;
    case GL_TEXTURE_BUFFER:
        return 12;
    case GL_TRANSFORM_FEEDBACK_BUFFER:
        return 13  + 16 * index;
    case GL_UNIFORM_BUFFER:
        return 14 + 16 * index;
    }
    assert(0 && "Unknown buffer binding target");
    return 0;
}

unsigned BufferStateMap::target_id_from_call(const trace::Call& call) const
{
    unsigned target = call.arg(0).toUInt();
    unsigned index = 0;

    if (!strcmp(call.name(), "glBindBufferRange")) {
        index = call.arg(1).toUInt();
    }
    return composed_target_id(target, index);
}

}
