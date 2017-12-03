#ifndef GCN_UAEDROPDOWN_HPP
#define GCN_UAEDROPDOWN_HPP

#include <string>

#include <guisan/keylistener.hpp>
#include <guisan/platform.hpp>
#include <guisan/widget.hpp>
#include <guisan/widgets/dropdown.hpp>


namespace gcn
{
	class GCN_CORE_DECLSPEC UaeDropDown : public DropDown
	{
	public:
		UaeDropDown(ListModel* listModel = nullptr,
		            ScrollArea* scrollArea = nullptr,
		            ListBox* listBox = nullptr);

		virtual ~UaeDropDown();

		void keyPressed(KeyEvent& keyEvent) override;

		void setEnabled(bool enabled);

		bool getEnabled(void) const;

		void clearSelected(void) const;

		bool isDroppedDown(void) const;

	protected:
		Color mBackgroundColorBackup;
	};
}


#endif // end GCN_UAEDROPDOWN_HPP
