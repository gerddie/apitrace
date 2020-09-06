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


}