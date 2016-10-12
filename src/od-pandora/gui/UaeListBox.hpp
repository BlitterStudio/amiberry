#ifndef GCN_UAELISTBOX_HPP
#define GCN_UAELISTBOX_HPP

#include <list>

#include "guichan/keylistener.hpp"
#include "guichan/listmodel.hpp"
#include "guichan/mouselistener.hpp"
#include "guichan/platform.hpp"
#include "guichan/widget.hpp"
#include "guichan/widgets/listbox.hpp"


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
