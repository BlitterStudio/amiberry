#pragma once
#include <guisan/widget.hpp>
#include <guisan/platform.hpp>
#include <guisan/widgetlistener.hpp>
#include <guisan/image.hpp>

#include <string>

namespace gcn
{
	class Container;
	class Icon;
	class Label;
	class Color;
	class WidgetListener;


	class GCN_CORE_DECLSPEC SelectorEntry :
		public Widget,
		public WidgetListener
	{
	public:
		SelectorEntry(const std::string& caption, const std::string& imagepath);

		~SelectorEntry() override;

		void draw(Graphics* graphics) override;

		void setInactiveColor(Color color);
		void setActiveColor(Color color);
		void setActive(bool active);
		[[nodiscard]] bool getActive() const;

		void widgetResized(const Event& event) override;

	protected:
		Container* container;
		Icon* icon;
		Label* label;
        gcn::Image* image;

		Color inactiveColor;
		Color activeColor;

		bool active;
	};
}

