#include "ftr_state.hpp"
#include "ftr_tracecall.hpp"
#include "ftr_genobject.hpp"

#include <algorithm>
#include <functional>

namespace frametrim_reverse {

using ftr_callback = std::function<PTraceCall(trace::Call&)>;
using std::bind;
using std::placeholders::_1;
using std::placeholders::_2;

struct TraceMirrorImpl {

    void process(const trace::Call& call, bool required);

    PTraceCall buffer_call(trace::Call &call);

    void register_buffer_calls();
    void register_texture_calls();
    void register_program_calls();
    void register_framebuffer_calls();

    GenObjectMap m_buffers;
    GenObjectMap m_renderbuffers;
    GenObjectMap m_framebuffers;

    using CallTable = std::multimap<const char *, ftr_callback, frametrim::string_part_less>;
    CallTable m_call_table;


    LightTrace trace;
};



TraceMirror::TraceMirror()
{
    impl = new TraceMirrorImpl;


}


TraceMirror::~TraceMirror()
{
    delete impl;
}


void TraceMirror::process(const trace::Call& call, bool required)
{
    impl->process(call, required);
}

PTraceCall TraceMirrorImpl::buffer_call(trace::Call& call)
{
    auto bound_obj = m_buffers.bound_to_call_target(call);
    return PTraceCall(new TraceCallOnBoundObj(call, bound_obj));
}


#define MAP(name, call) m_call_table.insert(std::make_pair(#name, bind(&TraceMirrorImpl::call, this, _1)))



#define MAP_GENOBJ(name, obj, call) \
    m_call_table.insert(std::make_pair(#name, bind(&call, &obj, _1)))
#define MAP_GENOBJ_DATA(name, obj, call, data) \
    m_call_table.insert(std::make_pair(#name, bind(&call, &obj, _1, data)))

void TraceMirrorImpl::register_buffer_calls()
{
    MAP_GENOBJ(glGenBuffers, m_buffers, GenObjectMap::generate);
    MAP_GENOBJ(glDeleteBuffers, m_buffers, GenObjectMap::destroy);
    MAP_GENOBJ_DATA(glBindBuffer, m_buffers, GenObjectMap::bind, 1);

    MAP(glBufferData, buffer_call);
    MAP(glBufferSubData, buffer_call);
    MAP(glMapBuffer, buffer_call);
    MAP(glMapBufferRange, buffer_call);
    MAP(memcpy, buffer_call);
    MAP(glUnmapBuffer, buffer_call);

}

void TraceMirrorImpl::register_texture_calls()
{

}

void TraceMirrorImpl::register_program_calls()
{

}

void TraceMirrorImpl::register_framebuffer_calls()
{

}

}
