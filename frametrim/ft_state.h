#ifndef STATE_H
#define STATE_H


#include "ft_objectstate.h"
#include <unordered_map>
#include <functional>

namespace frametrim {


using ft_callback = std::function<void(PCall)>;

class State
{
public:
   State();
   ~State();

   void target_frame_started();

   void call(PCall call);

private:
   struct StateImpl *impl;

};

}

#endif // STATE_H
