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

    bool in_target_frame() const override;

    CallSet& global_callset() override;

    PFramebufferState draw_framebuffer() const  override;

    PFramebufferState read_framebuffer() const  override;

    void collect_state_calls(CallSet& list) override;

    std::vector<unsigned> get_sorted_call_ids() const;

private:
    struct StateImpl *impl;

};

}

#endif // STATE_H
