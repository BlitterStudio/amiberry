#ifndef GCN_UAERADIOBUTTON_HPP
#define GCN_UAERADIOBUTTON_HPP

#include <map>
#include <string>

#include <guisan/platform.hpp>
#include <guisan/widget.hpp>
#include <guisan/widgets/radiobutton.hpp>


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
