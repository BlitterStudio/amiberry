#include "UaeRadioButton.hpp"
#include "guisan/widgets/radiobutton.hpp"

#include "guisan/font.hpp"
#include "guisan/graphics.hpp"
#include "guisan/key.hpp"
#include "guisan/mouseinput.hpp"


namespace gcn
{

UaeRadioButton::UaeRadioButton()
    : RadioButton()
{
}

UaeRadioButton::UaeRadioButton(const std::string &caption,
                               const std::string &group,
                               bool selected)
    : RadioButton(caption, group, selected)
{
    setId(caption);
}

UaeRadioButton::~UaeRadioButton()
{
    // Remove us from the group list
    setGroup("");
}

void UaeRadioButton::draw(Graphics* graphics)
{
    graphics->pushClipArea(Rectangle(1,
                                     1,
                                     getWidth() - 1,
                                     getHeight() - 1));
    drawBox(graphics);
    graphics->popClipArea();


    graphics->setFont(getFont());
    graphics->setColor(getForegroundColor());

    if (isFocused())
    {
        graphics->setColor(Color(0x000000));
        graphics->drawRectangle(Rectangle(0,
                                          0,
                                          getWidth(),
                                          getHeight()));
    }

    int h = getHeight() + getHeight() / 2;

    graphics->drawText(getCaption(), h - 2, 0);
}

}
