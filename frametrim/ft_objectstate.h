#ifndef OBJECTSTATE_H
#define OBJECTSTATE_H

#include "trace_model.hpp"

#include <GL/gl.h>
#include <vector>
#include <memory>

namespace frametrim {

using PCall=std::shared_ptr<trace::Call>;

class ObjectState
{
public:
   ObjectState(GLint glID);

   void append_call(PCall call);
   void set_required();
   void set_active(bool active);

   void append_calls_to(std::vector<PCall>& list) const;

   bool required() const;
   bool active() const;

private:
   GLint m_glID;

   uint64_t m_goid;

   /* Call chain that leads to the current state of this object */
   std::vector<PCall> m_calls;
   bool m_required;
   bool m_active;

   static uint64_t g_next_object_id;
};

using PObjectState=std::shared_ptr<ObjectState>;

}

#endif // OBJECTSTATE_H
