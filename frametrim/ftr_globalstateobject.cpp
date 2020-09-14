#include "ftr_globalstateobject.hpp"

#include <GL/gl.h>
#include <GL/glext.h>

namespace frametrim_reverse {

void GlobalStateObject::record_bind(BindType type, PGenObject obj,
                                    unsigned id, unsigned tex_unit, unsigned callno)
{
    unsigned index = buffer_offset(type, id, tex_unit);
    auto& record = m_bind_timelines[index];
    record.push(callno, obj);
}

void
GlobalStateObject::collect_owned_obj(ObjectSet& required_objects,
                                     const TraceCallRange& range)
{
    for(auto&& timeline : m_bind_timelines) {
        timeline.second.collect_active_in_call_range(required_objects, range);
    }
}


unsigned GlobalStateObject::buffer_offset(BindType type, unsigned target,
                                          unsigned active_unit)
{
    switch (type) {
    case bt_buffer:
        switch (target) {
        case GL_ARRAY_BUFFER: return 1;
            case GL_ATOMIC_COUNTER_BUFFER: 	return 2;
            case GL_COPY_READ_BUFFER: return 3;
            case GL_COPY_WRITE_BUFFER: return 4;
            case GL_DISPATCH_INDIRECT_BUFFER: return 5;
            case GL_DRAW_INDIRECT_BUFFER: return 6;
            case GL_ELEMENT_ARRAY_BUFFER: return 7;
            case GL_PIXEL_PACK_BUFFER: return 8;
            case GL_PIXEL_UNPACK_BUFFER: return 9;
            case GL_QUERY_BUFFER: return 10;
            case GL_SHADER_STORAGE_BUFFER: return 11;
            case GL_TEXTURE_BUFFER: return 12;
            case GL_TRANSFORM_FEEDBACK_BUFFER: return 13;
            case GL_UNIFORM_BUFFER: return 14;
            default:
                assert(0 && "unknown buffer bind point");
            }
        case bt_program:
            return 15;
        case bt_renderbuffer:
            return 16;
        case bt_framebuffer:
            switch (target) {
            case GL_READ_FRAMEBUFFER: return 17;
            case GL_DRAW_FRAMEBUFFER: return 18;
            default:
                /* Since GL_FRAMEBUFFER may set both buffers we must handle this elsewhere */
                assert(0 && "GL_FRAMEBUFFER must be handled before calling buffer_offset");
            }
        case bt_texture: {
            unsigned base = active_unit * 11  + 19;
            switch (target) {
            case GL_TEXTURE_1D: return base;
            case GL_TEXTURE_2D: return base + 1;
            case GL_TEXTURE_3D: return base + 2;
            case GL_TEXTURE_1D_ARRAY: return base + 3;
            case GL_TEXTURE_2D_ARRAY: return base + 4;
            case GL_TEXTURE_RECTANGLE: return base + 5;
            case GL_TEXTURE_CUBE_MAP: return base + 6;
            case GL_TEXTURE_CUBE_MAP_ARRAY: return base + 7;
            case GL_TEXTURE_BUFFER: return base + 8;
            case GL_TEXTURE_2D_MULTISAMPLE: return base + 9;
            case GL_TEXTURE_2D_MULTISAMPLE_ARRAY: return base + 10;
            default:
                assert(0 && "unknown texture bind point");
            }
        }
        case bt_sampler:
            return 800 + target;
        case bt_vertex_array:
            return 900;
        case bt_legacy_program:
            switch (target) {
            case GL_VERTEX_PROGRAM_ARB:
                return 901;
            case GL_FRAGMENT_PROGRAM_ARB:
                return 902;
            default:
                assert(0 && "unknown legacy shader bind point");
            }
        }
        assert(0 && "Unsupported bind point");


}



}
