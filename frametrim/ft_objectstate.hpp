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
   void append_call(PCall call);

   void append_calls_to(CallSet& list) const;



   CallSet& calls();

private:

   virtual void do_append_calls_to(CallSet& list) const;

   GLint m_glID;

   uint64_t m_goid;

   CallSet m_gen_calls;

   /* Call chain that leads to the current state of this object */
   CallSet m_calls;

   mutable bool m_submitted;

   static uint64_t g_next_object_id;
};

using PObjectState=std::shared_ptr<ObjectState>;

}

#endif // OBJECTSTATE_H
