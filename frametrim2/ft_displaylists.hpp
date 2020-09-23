#ifndef DISPLAYLISTS_HPP
#define DISPLAYLISTS_HPP

#include "ft_objectstate.hpp"

namespace frametrim {

class DisplayLists : public ObjectState {
public:
    using ObjectState::ObjectState;
private:
    ObjectType type() const override { return bt_display_list;}
};

}

#endif // DISPLAYLISTS_HPP
