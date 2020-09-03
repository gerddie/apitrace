#include "ftr_genobject.hpp"
namespace frametrim_reverse {

using std::make_shared;

GenObject::GenObject(unsigned id):
    m_id(id),
    m_visited(false)
{

}

void GenObject::record(CallIdSet& calls, Queue& objects)
{
    record_owned_obj(calls, objects);
    record_dependend_obj(calls, objects);
}

void GenObject::record_owned_obj(CallIdSet& calls, Queue& objects)
{
    (void)calls;
    (void)objects;
}

void GenObject::record_dependend_obj(CallIdSet& calls, Queue& objects)
{
    (void)calls;
    (void)objects;
}

}