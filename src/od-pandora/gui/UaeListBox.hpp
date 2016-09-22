#ifndef GCN_UAELISTBOX_HPP
#define GCN_UAELISTBOX_HPP

#include <list>

#include "guisan/keylistener.hpp"
#include "guisan/listmodel.hpp"
#include "guisan/mouselistener.hpp"
#include "guisan/platform.hpp"
#include "guisan/widget.hpp"
#include "guisan/widgets/listbox.hpp"


namespace gcn
{
class GCN_CORE_DECLSPEC UaeListBox : public ListBox
{
public:
    UaeListBox();

    UaeListBox(ListModel *listModel);

    virtual ~UaeListBox();

    virtual void draw(Graphics* graphics);
};
}


#endif // end GCN_UAELISTBOX_HPP
