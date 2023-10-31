#include "SelectorEntry.hpp"
#include "guisan/widgets/container.hpp"
#include "guisan/widgets/icon.hpp"
#include "guisan/widgets/label.hpp"

namespace gcn
{
	SelectorEntry::SelectorEntry(const std::string& caption, const std::string& imagepath)
	{
		addWidgetListener(this);

		active = false;

		container = new Container();
		container->setOpaque(true);

		label = new Label(caption);
		label->setHeight(16);

		image = Image::load(imagepath);
		icon = new Icon(image);
		icon->setSize(16, 16);

		container->add(icon, 4, 4);
		container->add(label, 24, 4);

		setFocusable(true);

		setId(caption);
	}

	SelectorEntry::~SelectorEntry()
	{
		removeWidgetListener(this);
		delete container;
		delete label;
        image->free();
		delete icon;
	}

	void SelectorEntry::draw(Graphics* graphics)
	{
		container->draw(graphics);
		if (isFocused())
			graphics->drawRectangle(Rectangle(2, 2, getWidth() - 4, getHeight() - 4));
	}

	void SelectorEntry::setInactiveColor(const Color& color)
	{
		inactiveColor = color;
		if (!active)
			container->setBaseColor(color);
	}

	void SelectorEntry::setActiveColor(const Color& color)
	{
		activeColor = color;
		if (active)
			container->setBaseColor(color);
	}

	void SelectorEntry::setActive(const bool active)
	{
		this->active = active;
		if (active)
			container->setBaseColor(activeColor);
		else
			container->setBaseColor(inactiveColor);
	}

	bool SelectorEntry::getActive() const
	{
		return active;
	}

	void SelectorEntry::widgetResized(const Event& event)
	{
		if (container->getWidth() != getWidth())
			container->setWidth(getWidth());
		if (container->getHeight() != getHeight())
			container->setHeight(getHeight());

		if (label->getWidth() != getWidth() - label->getX() - 4)
			label->setWidth(getWidth() - label->getX() - 4);
	}
}
