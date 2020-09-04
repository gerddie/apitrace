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

void GenObject::collect_calls(CallIdSet& calls)
{
    calls.insert(m_gen_call);
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