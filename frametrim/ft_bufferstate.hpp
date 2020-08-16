#ifndef BUFFERSTATE_HPP
#define BUFFERSTATE_HPP

#include "ft_objectstate.h"

namespace frametrim {

class BufferState : public ObjectState
{
public:
   BufferState(GLint glID, PCall gen_call);

   void bind(PCall call);
   void data(PCall call);
   void use(PCall call);

private:
   virtual void do_append_calls_to(CallSet& list) const;

   PCall m_gen_call;
   PCall m_last_bind_call;
   CallSet m_data_upload_set;
   CallSet m_data_use_set;
};

using PBufferState = std::shared_ptr<BufferState>;

}

#endif // BUFFERSTATE_HPP
