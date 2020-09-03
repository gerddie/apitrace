#include "ftr_genobject_impl.hpp"
namespace frametrim_reverse {

using std::make_shared;

GenObject::GenObject(unsigned id):
    m_id(id),
    m_visited(false)
{

}

template class CreateObjectMap<GenObject>;
template class GenBoundObjectMap<GenObject>;
template class GenObjectMap<GenObject>;


}