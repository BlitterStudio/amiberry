#ifndef GCN_UAEDROPDOWN_HPP
#define GCN_UAEDROPDOWN_HPP

#include <map>
#include <string>

#include "guisan/keylistener.hpp"
#include "guisan/mouselistener.hpp"
#include "guisan/platform.hpp"
#include "guisan/widget.hpp"
#include "guisan/widgets/dropdown.hpp"


namespace gcn
{
class GCN_CORE_DECLSPEC UaeDropDown : public DropDown
{
public:
    UaeDropDown(ListModel *listModel = NULL,
                ScrollArea *scrollArea = NULL,
                ListBox *listBox = NULL);

    virtual ~UaeDropDown();

    virtual void keyPressed(KeyEvent& keyEvent);

    virtual void setEnabled(bool enabled);

    void clearSelected(void);

    bool isDroppedDown(void);

protected:
    Color mBackgroundColorBackup;

};
}


#endif // end GCN_UAEDROPDOWN_HPP
