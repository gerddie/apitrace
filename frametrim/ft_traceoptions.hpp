#ifndef TRACEOPTIONS_HPP
#define TRACEOPTIONS_HPP

namespace frametrim {

class TraceOptions
{

public:
    static TraceOptions& instance();

    void set_in_required_frame(bool value);
    bool in_required_frame() const;

private:
    TraceOptions();

    bool m_in_required_frame;
};

}

#endif // TRACEOPTIONS_HPP
