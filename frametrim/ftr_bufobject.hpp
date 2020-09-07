#ifndef BUFOBJECT_HPP
#define BUFOBJECT_HPP

#include "ftr_boundobject.hpp"

#include <unordered_set>

namespace frametrim_reverse {

class BufObject : public BoundObject
{
public:
    using Pointer = std::shared_ptr<BufObject>;
    using BoundObject::BoundObject;

    void data(trace::Call& call);
    void map(trace::Call& call);
    void map_range(trace::Call& call);
    void unmap(trace::Call& call);
    bool address_in_mapped_range(uint64_t addr) const;

private:
    uint64_t m_size;
    std::pair<uint64_t, uint64_t> m_mapped_range;
};

using PBufObject = BufObject::Pointer;

struct BufferIdHash {
    std::size_t operator () (const PBufObject& obj) const {
        return std::hash<unsigned>{}(obj->id());
    }
};

class BufObjectMap : public GenObjectMap<BufObject> {
public:
    using GenObjectMap::GenObjectMap;

    PTraceCall data(trace::Call& call);
    PTraceCall map(trace::Call& call);
    PTraceCall map_range(trace::Call& call);
    PTraceCall unmap(trace::Call& call);
    PTraceCall memcopy(trace::Call& call);

private:
    std::unordered_set<PBufObject, BufferIdHash> m_mapped_buffers;

};


}
#endif // BUFOBJECT_HPP
