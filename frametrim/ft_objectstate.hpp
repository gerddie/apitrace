#ifndef OBJECTSTATE_H
#define OBJECTSTATE_H

#include "trace_model.hpp"

#include <GL/gl.h>
#include <vector>
#include <memory>
#include <set>

namespace frametrim {

using PCall=std::shared_ptr<trace::Call>;

struct pcall_less {
   bool operator () (PCall lhs, PCall rhs)
   {
      return lhs->no < rhs->no;
   }
};

using CallSet = std::set<PCall>;

class ObjectState
{
public:
   ObjectState(GLint glID);

   unsigned id() const;

   void append_gen_call(PCall call);

   void emit_calls_to_list(CallSet& list) const;

   void append_call(PCall call);

protected:

   void reset_callset();

private:

   virtual void do_emit_calls_to_list(CallSet& list) const;

   GLint m_glID;

   CallSet m_gen_calls;

   CallSet m_calls;

   mutable bool m_emitting;
};

using PObjectState=std::shared_ptr<ObjectState>;

}

#endif // OBJECTSTATE_H
