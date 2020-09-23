#ifndef VERTEXARRAY_HPP
#define VERTEXARRAY_HPP

#include "ft_genobjectstate.hpp"

namespace frametrim {

class VertexArray : public GenObjectState
{
public:
    using GenObjectState::GenObjectState;
    using Pointer =std::shared_ptr< VertexArray>;

private:
    ObjectType type() const override { return bt_vertex_array;}

};

}

#endif // VERTEXARRAY_HPP
