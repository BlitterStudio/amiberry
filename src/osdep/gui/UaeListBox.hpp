#ifndef GCN_UAELISTBOX_HPP
#define GCN_UAELISTBOX_HPP

#include <list>

#include "guisan/listmodel.hpp"
#include "guisan/platform.hpp"
#include "guisan/widget.hpp"
#include "guisan/widgets/listbox.hpp"


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
