#include "ft_programstate.hpp"
#include "ft_globalstate.hpp"

#include <iostream>

namespace frametrim {

using std::make_shared;

ShaderState::ShaderState(unsigned id, unsigned stage):
    ObjectWithBindState(id, nullptr),
    m_stage(stage),
    m_attach_count(0)
{
}

ShaderState::ShaderState(unsigned id, PCall call):
    ObjectWithBindState(id, call),
    m_stage(0),
    m_attach_count(0)
{
    append_call(trace2call(*call));
}

void ShaderState::set_stage(unsigned stage)
{
    m_stage = stage;
}

unsigned ShaderState::stage() const
{
    return m_stage;
}

void ShaderState::attach()
{
    ++m_attach_count;
}

void ShaderState::detach()
{
    if (m_attach_count > 0)
        --m_attach_count;
    else
        std::cerr << "Try to detach shader that is not attached\n";
}

bool ShaderState::is_attached() const
{
    return m_attach_count > 0;
}


bool ShaderState::is_active() const
{
    return bound() || is_attached();
}

void ShaderStateMap::create(PCall call)
{
    GLint shader_id = call->ret->toUInt();
    GLint stage = call->arg(0).toUInt();
    auto shader = make_shared<ShaderState>(shader_id, stage);
    shader->append_call(trace2call(*call));
    set(shader_id, shader);
}


void ShaderStateMap::data(PCall call)
{
    auto shader = get_by_id(call->arg(0).toUInt());
    shader->append_call(trace2call(*call));
}

void ShaderStateMap::do_emit_calls_to_list(CallSet& list) const
{
    (void)list;
}

ProgramState::ProgramState(unsigned id):
    ObjectState(id)
{
}


void ProgramState::attach_shader(PShaderState shader)
{
    m_shaders[shader->stage()] = shader;
}

void ProgramState::set_uniform(PCall call)
{
    m_uniforms[call->arg(0).toUInt()] = trace2call(*call);
}

void ProgramState::bind(PCall call)
{
    m_last_bind = trace2call(*call);
    m_uniforms.clear();
}

void ProgramState::do_emit_calls_to_list(CallSet& list) const
{
    for(auto& s : m_shaders)
        s.second->emit_calls_to_list(list);

    if (m_last_bind) {
        list.insert(m_last_bind);

        for (auto& s: m_uniforms)
            list.insert(s.second);
    }
}


void ProgramStateMap::create(PCall call)
{
    uint64_t id = call->ret->toUInt();
    auto program = make_shared<ProgramState>(id);
    this->set(id, program);
    program->append_call(trace2call(*call));
}

void ProgramStateMap::destroy(PCall call)
{
    this->clear(call->ret->toUInt());
}

void ProgramStateMap::use(PCall call)
{
    unsigned progid = call->arg(0).toUInt();
    bool new_program = false;

    if (!m_active_program ||
            m_active_program->id() != progid) {
        m_active_program = progid > 0 ? get_by_id(progid) : nullptr;
        new_program = true;
    }

    if (m_active_program && new_program)
        m_active_program->bind(call);

    if (m_active_program) {
        auto& gs = global_state();
        if (gs.in_target_frame())
            m_active_program->emit_calls_to_list(gs.global_callset());

        auto draw_fb = gs.draw_framebuffer();
        if (draw_fb)
            m_active_program->emit_calls_to_list(draw_fb->dependent_calls());
    }
}

void ProgramStateMap::set_state(PCall call, unsigned addr_params)
{
    auto program = get_by_id(call->arg(0).toUInt());
    if (!program) {
        std::cerr << "No program found in call " << call->no
                  << " target:" << call->arg(0).toUInt();
        assert(0);
    }
    program->set_state_call(call, addr_params);
}

void ProgramStateMap::attach_shader(PCall call, ShaderStateMap& shaders)
{
    auto program = get_by_id(call->arg(0).toUInt());
    assert(program);

    auto shader = shaders.get_by_id(call->arg(1).toUInt());
    assert(shader);

    program->attach_shader(shader);
    shader->attach();
    program->append_call(trace2call(*call));
}

void ProgramStateMap::bind_attr_location(PCall call)
{
    GLint program_id = call->arg(0).toSInt();
    auto prog = get_by_id(program_id);
    prog->append_call(trace2call(*call));
}

void ProgramStateMap::data(PCall call)
{
    auto prog = get_by_id(call->arg(0).toUInt());
    assert(prog);
    prog->append_call(trace2call(*call));
}

void ProgramStateMap::uniform(PCall call)
{
    assert(m_active_program);
    m_active_program->set_uniform(call);
}

void ProgramStateMap::do_emit_calls_to_list(CallSet& list) const
{
    if (m_active_program)
        m_active_program->emit_calls_to_list(list);
}

void LegacyProgramStateMap::program_string(PCall call)
{
    auto shader = bound_in_call(call);
    assert(shader);
    shader->append_call(trace2call(*call));
}

void LegacyProgramStateMap::do_emit_calls_to_list(CallSet& list) const
{
    (void)list;
}

}