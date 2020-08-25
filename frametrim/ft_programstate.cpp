#include "ft_programstate.hpp"
#include "ft_globalstate.hpp"

#include <iostream>

namespace frametrim {

using std::make_shared;

ShaderState::ShaderState(unsigned id, unsigned stage):
    ObjectState(id),
    m_stage(stage)
{
}

ShaderState::ShaderState(unsigned id, PCall call):
    ObjectState(id),
    m_stage(0)
{
    append_call(call);
}

void ShaderState::set_stage(unsigned stage)
{
    m_stage = stage;
}

unsigned ShaderState::stage() const
{
    return m_stage;
}

void ShaderStateMap::create(PCall call)
{
    GLint shader_id = call->ret->toUInt();
    GLint stage = call->arg(0).toUInt();
    auto shader = make_shared<ShaderState>(shader_id, stage);
    shader->append_call(call);
    set(shader_id, shader);
}

void ShaderStateMap::data(PCall call)
{
    auto shader = get_by_id(call->arg(0).toUInt());
    shader->append_call(call);
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
    m_uniforms[call->arg(0).toUInt()] = call;
}

void ProgramState::set_va(unsigned id, PObjectState va)
{
    m_va[id] = va;
}

void ProgramState::bind(PCall call)
{
    m_last_bind = call;
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

        for (auto& va: m_va)
            va.second->emit_calls_to_list(list);
    }
}


void ProgramStateMap::create(PCall call)
{
    uint64_t id = call->ret->toUInt();
    auto program = make_shared<ProgramState>(id);
    this->set(id, program);
    program->append_call(call);
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

void ProgramStateMap::attach_shader(PCall call, ShaderStateMap& shaders)
{
    auto program = get_by_id(call->arg(0).toUInt());
    assert(program);

    auto shader = shaders.get_by_id(call->arg(1).toUInt());
    assert(shader);

    program->attach_shader(shader);
    program->append_call(call);
}

void ProgramStateMap::bind_attr_location(PCall call)
{
    GLint program_id = call->arg(0).toSInt();
    auto prog = get_by_id(program_id);
    prog->append_call(call);
}

void ProgramStateMap::data(PCall call)
{
    auto prog = get_by_id(call->arg(0).toUInt());
    assert(prog);
    prog->append_call(call);
}

void ProgramStateMap::uniform(PCall call)
{
    assert(m_active_program);
    m_active_program->set_uniform(call);
}

void ProgramStateMap::set_va(unsigned attrid, PBufferState buf)
{
    if (m_active_program)
        m_active_program->set_va(attrid, buf);
}

void ProgramStateMap::do_emit_calls_to_list(CallSet& list) const
{
    if (m_active_program)
        m_active_program->emit_calls_to_list(list);
}

void LegacyProgramStateMap::program_string(PCall call)
{
    auto stage = call->arg(0).toUInt();
    auto shader = m_active_shaders[stage];
    assert(shader);
    shader->append_call(call);
}

void LegacyProgramStateMap::bind(PCall call)
{
    GLint stage = call->arg(0).toSInt();
    GLint shader_id = call->arg(1).toSInt();

    if (shader_id > 0) {
        auto prog = get_by_id(shader_id);
        if (!prog) {
            // some old programs don't call GenProgram
            auto prog = make_shared<ShaderState>(shader_id, stage);
            set(shader_id, prog);
        } else
            prog->set_stage(stage);
        prog->append_call(call);
        m_active_shaders[stage] = prog;
    } else {
        /* If the active program is still connected somewehere, we want the
         * unbind call to be used, so add it here, but then remove the
         * shader from the active list. */
        if (m_active_shaders[stage])
            m_active_shaders[stage]->append_call(call);
        m_active_shaders.erase(stage);
    }
}
void LegacyProgramStateMap::do_emit_calls_to_list(CallSet& list) const
{
    for(auto& s: m_active_shaders)
        s.second->emit_calls_to_list(list);
}

}