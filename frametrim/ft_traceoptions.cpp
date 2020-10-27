#include "ft_traceoptions.hpp"

namespace frametrim {

TraceOptions& TraceOptions::instance()
{
    static TraceOptions me;
    return me;
}

TraceOptions::TraceOptions():
    m_in_required_frame(false)
{

}

void TraceOptions::set_in_required_frame(bool value)
{
    m_in_required_frame = value;
}

bool TraceOptions::in_required_frame() const
{
    return m_in_required_frame;
}

}