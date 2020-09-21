#ifndef PROGRAMSTATE_HPP
#define PROGRAMSTATE_HPP

#include "ft_bufferstate.hpp"

#include <unordered_map>

namespace frametrim {

class ShaderState: public ObjectWithBindState {
public:
    using Pointer = std::shared_ptr<ShaderState>;

    ShaderState(unsigned id, unsigned stage);
    ShaderState(unsigned id, PCall call);

    /* For legacy shaders */
    void set_stage(unsigned stage);

    unsigned stage() const;

    void attach();
    void detach();
    bool is_attached() const;
private:
    bool is_active() const override;

    unsigned m_stage;
    int m_attach_count;
};

using PShaderState = ShaderState::Pointer;

class ShaderStateMap : public TObjStateMap<ShaderState>
{
public:
    using TObjStateMap<ShaderState>::TObjStateMap;

    void create(PCall call);
    void data(PCall call);
private:
    void do_emit_calls_to_list(CallSet& list) const override;
};

class ProgramState : public ObjectState
{
public:
    using Pointer = std::shared_ptr<ProgramState>;

    ProgramState(unsigned id);

    void attach_shader(PShaderState shader);
    void set_uniform(PCall call);

    void bind(PCall call);
    void unbind(PCall call);

private:

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

    void create(PCall call);
    void destroy(PCall call);
    void use(PCall call);
    void attach_shader(PCall call, ShaderStateMap &shaders);
    void bind_attr_location(PCall call);
    void data(PCall call);
    void uniform(PCall call);
    void set_va(unsigned attrid, PBufferState buf);
    void set_state(PCall call, unsigned addr_params);

private:
    void do_emit_calls_to_list(CallSet& list) const override;

    PProgramState m_active_program;
};

class LegacyProgramStateMap : public TGenObjStateMap<ShaderState>
{
public:
    using TGenObjStateMap<ShaderState>::TGenObjStateMap;

    void program_string(PCall call);

private:
    void do_emit_calls_to_list(CallSet& list) const override;

};

}

#endif // PROGRAMSTATE_HPP
