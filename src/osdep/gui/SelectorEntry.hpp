#ifndef GCN_SELECTORENTRY_HPP
#define GCN_SELECTORENTRY_HPP

#ifdef USE_SDL1
#include "guichan/basiccontainer.hpp"
#include "guichan/graphics.hpp"
#include "guichan/platform.hpp"
#include "guichan/widgetlistener.hpp"
#elif USE_SDL2
#include <guisan/basiccontainer.hpp>
#include <guisan/platform.hpp>
#include <guisan/widgetlistener.hpp>
#endif
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

		virtual ~SelectorEntry();

		void draw(Graphics* graphics) override;

		void setInactiveColor(const Color& color);
		void setActiveColor(const Color& color);
		void setActive(bool active);
		bool getActive() const;

		void widgetResized(const Event& event) override;

	protected:
		Container* container;
		Icon* icon;
		Label* label;

		Color inactiveColor;
		Color activeColor;

		bool active;
	};
}

#endif // end GCN_SELECTORENTRY_HPP
