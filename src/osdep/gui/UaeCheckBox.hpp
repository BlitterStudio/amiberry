#ifndef GCN_UAECHECKBOX_HPP
#define GCN_UAECHECKBOX_HPP

#include <map>
#include <string>

#include <guisan/platform.hpp>
#include <guisan/widget.hpp>
#include <guisan/widgets/checkbox.hpp>

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
