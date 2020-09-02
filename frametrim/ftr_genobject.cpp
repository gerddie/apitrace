#include "ftr_genobject_impl.hpp"
namespace frametrim_reverse {

using std::make_shared;

GenObject::GenObject(unsigned id):m_id(id)
{

}

unsigned GenObject::id() const
{
    return m_id;
}

template class GenObjectMap<GenObject>;

}