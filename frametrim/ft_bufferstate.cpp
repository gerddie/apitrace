#include "ft_bufferstate.hpp"

namespace frametrim {

using std::pair;
using std::make_pair;

struct BufferSubRange {

   BufferSubRange() = default;
   BufferSubRange(unsigned start, unsigned end, PCall call);
   bool overlap_with(const BufferSubRange& range) const;
   bool split(const BufferSubRange& range, BufferSubRange& second_split);

   unsigned m_start;
   unsigned m_end;
   PCall m_call;
};

struct BufferStateImpl {

   PCall m_last_bind_call;
   CallSet m_data_upload_set;
   CallSet m_data_use_set;
   bool m_last_bind_call_dirty;

   std::vector<BufferSubRange> m_sub_buffers;

   void bind(PCall call);
   void data(PCall call);
   void append_data(PCall call);
   void use(PCall call = nullptr);
   void emit_calls_to_list(CallSet& list) const;
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
}

void BufferStateImpl::append_data(PCall call)
{
   if (m_last_bind_call_dirty) {
      m_data_upload_set.insert(m_last_bind_call);
      m_last_bind_call_dirty = false;
   }

   unsigned start = call->arg(0).toUInt();
   unsigned end =  start + call->arg(1).toUInt();

   BufferSubRange bsr(start, end, call);

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
   list.insert(m_data_upload_set);
   list.insert(m_data_use_set);
}

BufferSubRange::BufferSubRange(unsigned start, unsigned end, PCall call):
   m_start(start), m_end(end), m_call(call)
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


}
