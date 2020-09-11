#ifndef TRACECALLEXT_HPP
#define TRACECALLEXT_HPP

#include "ftr_tracecall.hpp"
#include "ftr_genobject.hpp"

namespace frametrim_reverse {


class StateCall : public TraceCall {
public:
    StateCall(const trace::Call& call, unsigned num_param_ids);
private:
    static std::string combined_name(const trace::Call& call,
                                     unsigned num_param_ids);
};

class StateEnableCall : public TraceCall {
public:
    StateEnableCall(const trace::Call& call, const char *basename);
private:
    static std::string combined_name(const trace::Call& call, const char *basename);
};


class TraceCallOnBoundObj : public TraceCall {
public:
    TraceCallOnBoundObj(const trace::Call& call, PGenObject obj);
    TraceCallOnBoundObj(const trace::Call& call,
                        PGenObject obj,
                        PGenObject dep);
private:
    void add_dependend_objects(ObjectSet& out_set) const override;
    std::vector<PGenObject> m_dependencys;
};

class BufferSubrangeCall : public TraceCall {
public:
    BufferSubrangeCall(const trace::Call& call, uint64_t start, uint64_t end);
    uint64_t start() const {return m_start;}
    uint64_t end() const {return m_end;}
private:
    uint64_t m_start, m_end;
};

using PBufferSubrangeCall = std::shared_ptr<BufferSubrangeCall>;




}

#endif // DUMMY_HPP
