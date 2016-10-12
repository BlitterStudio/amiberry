#include "UaeCheckBox.hpp"
#include "guichan/widgets/checkbox.hpp"

#include "guichan/font.hpp"
#include "guichan/graphics.hpp"
#include "guichan/key.hpp"
#include "guichan/mouseinput.hpp"


namespace gcn
{

    UaeCheckBox::UaeCheckBox()
      : CheckBox()
    {
    }

    UaeCheckBox::UaeCheckBox(const std::string &caption,
                             bool selected)
      : CheckBox(caption, selected)
    {
      setId(caption);
    }

    UaeCheckBox::~UaeCheckBox()
    {
    }

    void UaeCheckBox::draw(Graphics* graphics)
    {
        drawBox(graphics);

        graphics->setFont(getFont());
        graphics->setColor(getForegroundColor());

        const int h = getHeight() + getHeight() / 2;

        graphics->drawText(getCaption(), h - 2, 0);

        if (isFocused())
        {
            graphics->setColor(Color(0x000000));
            graphics->drawRectangle(Rectangle(0,
                                              0,
                                              getWidth(),
                                              getHeight()));
        }
    }

    void UaeCheckBox::drawBox(Graphics *graphics)
    {
        const int h = getHeight() - 2;
        const int alpha = getBaseColor().a;
        Color faceColor = getBaseColor();
        faceColor.a = alpha;
        Color highlightColor = faceColor + 0x303030;
        highlightColor.a = alpha;
        Color shadowColor = faceColor - 0x303030;
        shadowColor.a = alpha;

        graphics->setColor(shadowColor);
        graphics->drawLine(1, 1, h, 1);
        graphics->drawLine(1, 1, 1, h);

        graphics->setColor(highlightColor);
        graphics->drawLine(h, 1, h, h);
        graphics->drawLine(1, h, h - 1, h);

        Color backCol = getBackgroundColor();
        if(!isEnabled())
          backCol = backCol  - 0x303030;
        graphics->setColor(backCol);
        graphics->fillRectangle(Rectangle(2, 2, h - 2, h - 2));

        graphics->setColor(getForegroundColor());

        if (mSelected)
        {
            graphics->drawLine(3, 9, 6, h - 2);
            graphics->drawLine(4, 9, 7, h - 2);

            graphics->drawLine(6, h - 2, h - 3, 4);
            graphics->drawLine(7, h - 2, h - 2, 4);
        }
    }

}
