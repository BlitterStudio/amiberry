#ifndef GCN_UAECHECKBOX_HPP
#define GCN_UAECHECKBOX_HPP

#include <map>
#include <string>

#include "guisan/keylistener.hpp"
#include "guisan/mouselistener.hpp"
#include "guisan/platform.hpp"
#include "guisan/widget.hpp"
#include "guisan/widgets/checkbox.hpp"


namespace gcn
{
class GCN_CORE_DECLSPEC UaeCheckBox : public CheckBox
{
public:
    UaeCheckBox();

    UaeCheckBox(const std::string &caption,
                bool selected = false);

    virtual ~UaeCheckBox();

    virtual void draw(Graphics* graphics);

protected:
    virtual void drawBox(Graphics *graphics);

};
}


#endif // end GCN_UAECHECKBOX_HPP
