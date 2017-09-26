#ifndef GCN_SELECTORENTRY_HPP
#define GCN_SELECTORENTRY_HPP

#include <list>

#include "guichan/basiccontainer.hpp"
#include "guichan/graphics.hpp"
#include "guichan/platform.hpp"
#include "guichan/widgetlistener.hpp"


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
      
      virtual void draw(Graphics* graphics);
      
      void setInactiveColor(const Color& color);
      void setActiveColor(const Color& color);
      void setActive(bool active);
      bool getActive(void);
      
      virtual void widgetResized(const Event& event);
      
    protected:
      Container *container;
      Icon *icon;
      Label *label;

      Color inactiveColor;
      Color activeColor;
      
      bool active;
  };
}

#endif // end GCN_SELECTORENTRY_HPP
