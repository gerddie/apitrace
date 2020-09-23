#ifndef PROGRAMSTATE_HPP
#define PROGRAMSTATE_HPP

#include "ft_genobjectstate.hpp"

#include <unordered_map>

namespace frametrim {

class ShaderState: public ObjectWithBindState {
public:
    using Pointer = std::shared_ptr<ShaderState>;

    ShaderState(unsigned id, unsigned stage);
    ShaderState(unsigned id, PTraceCall call);

    /* For legacy shaders */
    void set_stage(unsigned stage);

    unsigned stage() const;

    void attach();
    void detach();
    bool is_attached() const;
private:
    bool is_active() const override;

    ObjectType type() const override {return bt_shader;}

    unsigned m_stage;
    int m_attach_count;
};

using PShaderState = ShaderState::Pointer;

class ShaderStateMap : public TObjStateMap<ShaderState>
{
public:
    using TObjStateMap<ShaderState>::TObjStateMap;

    PTraceCall create(const trace::Call& call);
    PTraceCall data(const trace::Call& call);
private:
    void do_emit_calls_to_list(CallSet& list) const override;
};

class ProgramState : public ObjectState
{
public:
    using Pointer = std::shared_ptr<ProgramState>;

    ProgramState(unsigned id);

    void  attach_shader(PShaderState shader);
    PTraceCall set_uniform(const trace::Call& call);

    PTraceCall bind(const trace::Call& call);
    PTraceCall unbind(const trace::Call& call);

private:
    ObjectType type() const override {return bt_program;}

    void do_emit_calls_to_list(CallSet& list) const override;

    std::unordered_map<unsigned, PShaderState> m_shaders;
    std::unordered_map<unsigned, PTraceCall> m_uniforms;

    PTraceCall m_last_bind;

};

using PProgramState = ProgramState::Pointer;

class ProgramStateMap : public TObjStateMap<ProgramState>
{
public:
    using TObjStateMap<ProgramState>::TObjStateMap;

    PTraceCall create(const trace::Call& call);
    PTraceCall destroy(const trace::Call& call);
    PTraceCall use(const trace::Call& call);
    PTraceCall attach_shader(const trace::Call& call, ShaderStateMap &shaders);
    PTraceCall bind_attr_location(const trace::Call& call);
    PTraceCall data(const trace::Call& call);
    PTraceCall uniform(const trace::Call& call);
    PTraceCall set_state(const trace::Call& call, unsigned addr_params);

private:
    void do_emit_calls_to_list(CallSet& list) const override;

    PProgramState m_active_program;
};

class LegacyProgramStateMap : public TGenObjStateMap<ShaderState>
{
public:
    using TGenObjStateMap<ShaderState>::TGenObjStateMap;

    PTraceCall program_string(const trace::Call& call);

private:
    void do_emit_calls_to_list(CallSet& list) const override;
};

}

#endif // PROGRAMSTATE_HPP
