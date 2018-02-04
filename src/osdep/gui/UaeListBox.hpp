#ifndef GCN_UAELISTBOX_HPP
#define GCN_UAELISTBOX_HPP

#include <list>

#ifdef USE_SDL1
#include "guichan/keylistener.hpp"
#include "guichan/listmodel.hpp"
#include "guichan/mouselistener.hpp"
#include "guichan/platform.hpp"
#include "guichan/widget.hpp"
#include "guichan/widgets/listbox.hpp"
#elif USE_SDL2
#include <guisan/listmodel.hpp>
#include <guisan/platform.hpp>
#include <guisan/widget.hpp>
#include <guisan/widgets/listbox.hpp>
#endif

namespace gcn
{
	class GCN_CORE_DECLSPEC UaeListBox : public ListBox
	{
	public:
		UaeListBox();

		explicit UaeListBox(ListModel* listModel);

		virtual ~UaeListBox();

		void draw(Graphics* graphics) override;
	};
}


#endif // end GCN_UAELISTBOX_HPP
