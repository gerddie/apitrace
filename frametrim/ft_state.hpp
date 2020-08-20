#ifndef STATE_H
#define STATE_H


#include "ft_objectstate.hpp"
#include <unordered_map>
#include <functional>

namespace  trace {
class Writer;
};

namespace frametrim {

using ft_callback = std::function<void(PCall)>;

class State
{
public:
    State();
    ~State();

    void target_frame_started();

    void call(PCall call);

    void write(trace::Writer& writer);

private:
    struct StateImpl *impl;

};

}

#endif // STATE_H
