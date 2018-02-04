#ifndef GCN_UAERADIOBUTTON_HPP
#define GCN_UAERADIOBUTTON_HPP

#include <map>
#include <string>

#ifdef USE_SDL1
#include "guichan/keylistener.hpp"
#include "guichan/mouselistener.hpp"
#include "guichan/platform.hpp"
#include "guichan/widget.hpp"
#include "guichan/widgets/radiobutton.hpp"
#elif USE_SDL2
#include <guisan/platform.hpp>
#include <guisan/widget.hpp>
#include <guisan/widgets/radiobutton.hpp>
#endif


namespace gcn
{
	class GCN_CORE_DECLSPEC UaeRadioButton : public RadioButton
	{
	public:
		UaeRadioButton();

		UaeRadioButton(const std::string& caption,
			const std::string& group,
			bool selected = false);

		virtual ~UaeRadioButton();

		void draw(Graphics* graphics) override;
	};
}


#endif // end GCN_UAERADIOBUTTON_HPP
