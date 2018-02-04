#ifndef GCN_UAECHECKBOX_HPP
#define GCN_UAECHECKBOX_HPP

#include <map>
#include <string>

#ifdef USE_SDL1
#include "guichan/keylistener.hpp"
#include "guichan/mouselistener.hpp"
#include "guichan/platform.hpp"
#include "guichan/widget.hpp"
#include "guichan/widgets/checkbox.hpp"
#elif USE_SDL2
#include <guisan/platform.hpp>
#include <guisan/widget.hpp>
#include <guisan/widgets/checkbox.hpp>
#endif

namespace gcn
{
	class GCN_CORE_DECLSPEC UaeCheckBox : public CheckBox
	{
	public:
		UaeCheckBox();

		UaeCheckBox(const std::string& caption,
		            bool selected = false);

		virtual ~UaeCheckBox();

		void draw(Graphics* graphics) override;

	protected:
		void drawBox(Graphics* graphics) override;
	};
}


#endif // end GCN_UAECHECKBOX_HPP
