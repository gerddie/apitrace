#include "ft_programstate.hpp"
#include "ft_framebuffer.hpp"

#include <iostream>

namespace frametrim {

using std::make_shared;

ShaderState::ShaderState(unsigned id, unsigned stage):
    ObjectWithBindState(id),
    m_stage(stage),
    m_attach_count(0)
{
}

ShaderState::ShaderState(unsigned id, PTraceCall call):
    ObjectWithBindState(id, call),
    m_stage(0),
    m_attach_count(0)
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

PTraceCall ShaderStateMap::create(const trace::Call& call)
{
    GLint shader_id = call.ret->toUInt();
    GLint stage = call.arg(0).toUInt();
    auto shader = make_shared<ShaderState>(shader_id, stage);
    set(shader_id, shader);
    return shader->append_call(trace2call(call));
}


PTraceCall ShaderStateMap::data(const trace::Call& call)
{
    auto shader = get_by_id(call.arg(0).toUInt());
    return shader->append_call(trace2call(call));
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

PTraceCall
ProgramState::set_uniform(const trace::Call& call)
{
    m_uniforms[call.arg(0).toUInt()] = trace2call(call);
    return m_uniforms[call.arg(0).toUInt()];
}

PTraceCall
ProgramState::bind(const trace::Call& call)
{
    m_last_bind = trace2call(call);
    return m_last_bind;
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


PTraceCall
ProgramStateMap::create(const trace::Call& call)
{
    uint64_t id = call.ret->toUInt();
    auto program = make_shared<ProgramState>(id);
    this->set(id, program);
    return program->append_call(trace2call(call));
}

PTraceCall
ProgramStateMap::destroy(const trace::Call& call)
{
    this->clear(call.ret->toUInt());
    return trace2call(call);
}

PTraceCall
ProgramStateMap::use(const trace::Call& call, FramebufferState& fbo)
{
    unsigned progid = call.arg(0).toUInt();
    bool new_program = false;

    if (!m_active_program ||
            m_active_program->id() != progid) {
        m_active_program = progid > 0 ? get_by_id(progid) : nullptr;
        new_program = true;
    }

    if (m_active_program && new_program) {
        m_active_program->bind(call);  
        m_active_program->flush_state_cache(fbo);
        std::cerr << "Flushed program state of "
                  << m_active_program->id()
                  << " to framebuffer " << fbo.id()
                  << "\n";
    }
    return trace2call(call);
}

PTraceCall
ProgramStateMap::set_state(const trace::Call& call, unsigned addr_params)
{
    auto program = get_by_id(call.arg(0).toUInt());
    if (!program) {
        std::cerr << "No program found in call " << call.no
                  << " target:" << call.arg(0).toUInt();
        assert(0);
    }
    return program->set_state_call(call, addr_params);
}

PTraceCall
ProgramStateMap::attach_shader(const trace::Call& call, ShaderStateMap& shaders)
{
    auto program = get_by_id(call.arg(0).toUInt());
    assert(program);

    auto shader = shaders.get_by_id(call.arg(1).toUInt());
    assert(shader);

    program->attach_shader(shader);
    shader->attach();
    return program->append_call(trace2call(call));
}

PTraceCall
ProgramStateMap::bind_attr_location(const trace::Call& call)
{
    GLint program_id = call.arg(0).toSInt();
    auto prog = get_by_id(program_id);
    return prog->append_call(trace2call(call));
}

PTraceCall
ProgramStateMap::data(const trace::Call& call)
{
    auto prog = get_by_id(call.arg(0).toUInt());
    assert(prog);
    return prog->append_call(trace2call(call));
}

PTraceCall
ProgramStateMap::uniform(const trace::Call& call)
{
    assert(m_active_program);
    return m_active_program->set_uniform(call);
}

void ProgramStateMap::do_emit_calls_to_list(CallSet& list) const
{
    if (m_active_program)
        m_active_program->emit_calls_to_list(list);
}

PTraceCall
LegacyProgramStateMap::program_string(const trace::Call& call)
{
    auto shader = bound_in_call(call);
    assert(shader);
    return shader->append_call(trace2call(call));
}

void
LegacyProgramStateMap::do_emit_calls_to_list(CallSet& list) const
{
    (void)list;
}

}