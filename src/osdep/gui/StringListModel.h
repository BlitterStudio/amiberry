//
// Created by midwan on 30/12/2023.
//

#ifndef STRINGLISTMODEL_H
#define STRINGLISTMODEL_H

#include <guisan.hpp>

namespace gcn {
	class StringListModel : public gcn::ListModel {
	public:
		StringListModel() = default;

		explicit StringListModel(const std::vector<std::string> &list) : ListModel()
		{
			mStrings = list;
		}

		int getNumberOfElements() override {
			return static_cast<int>(mStrings.size());
		}

		std::string getElementAt(int i) override {
			if (i < 0 || i >= mStrings.size())
				return "";
			return mStrings.at(i);
		}

		void add(const std::string &str) override {
			mStrings.push_back(str);
		}

		void clear() override {
			mStrings.clear();
		}

		void swap_first(const std::string &str) {
			mStrings.erase(mStrings.begin());
			mStrings.emplace(mStrings.begin(), str);
		}

	private:
		std::vector<std::string> mStrings;
	};
}
#endif //STRINGLISTMODEL_H
