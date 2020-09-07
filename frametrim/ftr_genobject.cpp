#include "ftr_genobject.hpp"
namespace frametrim_reverse {

using std::make_shared;

GenObject::GenObject(unsigned id, unsigned gen_call):
    m_id(id),
    m_gen_call(gen_call),
    m_visited(false)
{

}

void GenObject::collect_objects(Queue& objects)
{
    collect_owned_obj(objects);
    collect_dependend_obj(objects);
}

void GenObject::collect_calls(CallIdSet& calls, unsigned call_before)
{
    calls.insert(m_gen_call);
    collect_allocation_call(calls);
    collect_data_calls(calls, call_before);
    collect_bind_calls(calls, call_before);
    collect_state_calls(calls, call_before);
}

void GenObject::collect_allocation_call(CallIdSet& calls)
{
    (void)calls;
}

void GenObject::collect_data_calls(CallIdSet& calls, unsigned call_before)
{
    (void)calls;
    (void)call_before;
}

void GenObject::collect_state_calls(CallIdSet& calls, unsigned call_before)
{
    (void)calls;
    (void)call_before;
}

void GenObject::collect_dependend_obj(Queue& objects)
{
    (void)objects;
}

void GenObject::collect_owned_obj(Queue& objects)
{
    (void)objects;
}

void GenObject::collect_last_call_before(CallIdSet& calls,
                                         const std::vector<unsigned>& call_list,
                                         unsigned call_before)
{
    for (auto c = call_list.rbegin(); c != call_list.rend(); ++c)
        if (*c < call_before) {
            calls.insert(*c);
            return;
        }
}

void GenObject::collect_all_calls_before(CallIdSet& calls,
                                         const std::vector<unsigned>& call_list,
                                         unsigned call_before)
{
    for (auto& c : call_list)
        if (c < call_before)
            calls.insert(c);
}

void GenObject::collect_bind_calls(CallIdSet& calls, unsigned call_before)
{
    (void)calls;
    (void)call_before;
}

void BoundObject::collect_bind_calls(CallIdSet& calls, unsigned call_before)
{
    collect_last_call_before(calls, m_bind_calls, call_before);
}

}