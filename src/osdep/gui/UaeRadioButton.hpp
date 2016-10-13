#ifndef GCN_UAERADIOBUTTON_HPP
#define GCN_UAERADIOBUTTON_HPP

#include <map>
#include <string>

#include "guichan/keylistener.hpp"
#include "guichan/mouselistener.hpp"
#include "guichan/platform.hpp"
#include "guichan/widget.hpp"
#include "guichan/widgets/radiobutton.hpp"


namespace gcn
{
  class GCN_CORE_DECLSPEC UaeRadioButton : public RadioButton
  {
    public:
      UaeRadioButton();

      UaeRadioButton(const std::string &caption,
                  const std::string &group,
                  bool selected = false);

      virtual ~UaeRadioButton();

      virtual void draw(Graphics* graphics);

  };
}


#endif // end GCN_UAERADIOBUTTON_HPP
