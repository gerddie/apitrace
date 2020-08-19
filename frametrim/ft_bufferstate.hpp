#ifndef BUFFERSTATE_HPP
#define BUFFERSTATE_HPP

#include "ft_genobjectstate.hpp"

namespace frametrim {

class BufferState : public GenObjectState
{
public:
   using GenObjectState::GenObjectState;

   void bind(PCall call);
   void data(PCall call);
   void use(PCall call);

private:
   virtual void do_emit_calls_to_list(CallSet& list) const;

   PCall m_last_bind_call;
   CallSet m_data_upload_set;
   CallSet m_data_use_set;
};

using PBufferState = std::shared_ptr<BufferState>;

}

#endif // BUFFERSTATE_HPP
