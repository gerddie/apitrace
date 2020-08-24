#ifndef STATE_H
#define STATE_H


#include "ft_globalstate.hpp"
#include <unordered_map>
#include <functional>

namespace  trace {
class Writer;
};

namespace frametrim {

using ft_callback = std::function<void(PCall)>;

class State : public GlobalState
{
public:
    State();
    ~State();

    void target_frame_started();

    void call(PCall call);

    void write(trace::Writer& writer);

    bool in_target_frame() const override;

    CallSet global_callset() override;

    PObjectState draw_framebuffer() const  override;

    PObjectState read_framebuffer() const  override;

private:
    struct StateImpl *impl;

};

}

#endif // STATE_H
