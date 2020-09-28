#ifndef GENOBJECTSTATE_HPP
#define GENOBJECTSTATE_HPP

#include "ft_objectwithbindstate.hpp"

#include <unordered_map>

namespace frametrim {

class GenObjectState : public ObjectWithBindState
{
public:     
    using Pointer = std::shared_ptr<GenObjectState>;

    GenObjectState(GLint glID, PTraceCall gen_call);
protected:

    void emit_gen_call(CallSet& list) const;

    PTraceCall gen_call() const  override {return m_gen_call; }

private:

    PTraceCall m_gen_call;
};

using PGenObjectState = GenObjectState::Pointer;

template <typename T>
class TGenObjStateMap : public TObjectWithBindStateMap<T> {

public:
    using TObjectWithBindStateMap<T>::TObjectWithBindStateMap;

    PTraceCall generate(const trace::Call &call) {
        auto c = trace2call(call);
        const auto ids = (call.arg(1)).toArray();
        for (auto& v : ids->values) {
            auto obj = std::make_shared<T>(v->toUInt(), c);
            this->set(v->toUInt(), obj);
        }
        return c;
    }

    PTraceCall destroy(const trace::Call &call) {
        if (call.args.size() < 2)
            std::cerr << "call " << call.no << " " << call.name() << "will fail\n";

        const auto ids = (call.arg(1)).toArray();
        for (auto& v : ids->values) {
            auto state = this->get_by_id(v->toUInt());
            /* We erase the state here, but it might be referenced
             * elsewhere, so we have to record the call with the state */
            if (state)
                state->append_call(trace2call(call));
            this->clear(v->toUInt());
        }
        return trace2call(call);
    }
private:
    void do_emit_calls_to_list(CallSet& list) const override {
            this->emit_all_states(list);
    }
};

class SizedObjectState : public GenObjectState
{
public:
    using Pointer = std::shared_ptr<SizedObjectState>;

    enum EAttachmentType {
        texture,
        renderbuffer
    };

    SizedObjectState(GLint glID, PTraceCall gen_call, EAttachmentType at);

    unsigned width(unsigned level = 0) const;
    unsigned height(unsigned level = 0) const;

    friend bool operator == (const SizedObjectState& lhs,
                             const SizedObjectState& rhs);

    void unattach();
    void attach();
    bool is_attached() const;

protected:

    void set_size(unsigned level, unsigned w, unsigned h);

private:


    std::vector<std::pair<unsigned, unsigned>> m_size;

    EAttachmentType m_attachment_type;
    int m_attach_count;

};

using PSizedObjectState = std::shared_ptr<SizedObjectState>;

bool operator == (const SizedObjectState& lhs,
                  const SizedObjectState& rhs);


}

#endif // GENOBJECTSTATE_HPP
