/*      _______   __   __   __   ______   __   __   _______   __   __
 *     / _____/\ / /\ / /\ / /\ / ____/\ / /\ / /\ / ___  /\ /  |\/ /\
 *    / /\____\// / // / // / // /\___\// /_// / // /\_/ / // , |/ / /
 *   / / /__   / / // / // / // / /    / ___  / // ___  / // /| ' / /
 *  / /_// /\ / /_// / // / // /_/_   / / // / // /\_/ / // / |  / /
 * /______/ //______/ //_/ //_____/\ /_/ //_/ //_/ //_/ //_/ /|_/ /
 * \______\/ \______\/ \_\/ \_____\/ \_\/ \_\/ \_\/ \_\/ \_\/ \_\/
 *
 * Copyright (c) 2004 - 2008 Olof Naessén and Per Larsson
 *
 *
 * Per Larsson a.k.a finalman
 * Olof Naessén a.k.a jansem/yakslem
 *
 * Visit: http://guichan.sourceforge.net
 *
 * License: (BSD)
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Guichan nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * For comments regarding functions please see the header file.
 */

#include "guisan/text.hpp"

#include "guisan/exception.hpp"
#include "guisan/font.hpp"

namespace gcn
{
    Text::Text() : mCaretPosition(0), mCaretColumn(0), mCaretRow(0)
    {
        mRows.emplace_back();
    }

    Text::Text(const std::string& content) : mCaretPosition(0), mCaretColumn(0), mCaretRow(0)
    {
        std::string::size_type pos, lastPos = 0;
        int length;
        do
        {
            pos = content.find("\n", lastPos);

            if (pos != std::string::npos)
                length = pos - lastPos;
            else
                length = content.size() - lastPos;

            std::string sub = content.substr(lastPos, length);
            mRows.push_back(sub);
            lastPos = pos + 1;
        } while (pos != std::string::npos);
    }

    Text::~Text()
    {}

    void Text::setContent(const std::string& content)
    {
        mRows.clear();
        std::string::size_type pos, lastPos = 0;
        int length;
        do
        {
            pos = content.find("\n", lastPos);

            if (pos != std::string::npos)
                length = pos - lastPos;
            else
                length = content.size() - lastPos;

            std::string sub = content.substr(lastPos, length);
            mRows.push_back(sub);
            lastPos = pos + 1;
        } while (pos != std::string::npos);
    }

    std::string Text::getContent() const
    {
        if (mRows.empty()) return std::string("");

        std::string result;
        unsigned int i;
        for (i = 0; i < mRows.size() - 1; ++i)
            result = result + mRows[i] + "\n";

        result = result + mRows[i];

        return result;
    }

    void Text::setRow(unsigned int row, const std::string& content)
    {
        if (row >= mRows.size()) throw GCN_EXCEPTION("Row out of bounds!");

        mRows[row] = content;
    }

    void Text::addRow(const std::string& row)
    {
        for (unsigned int i = 0; i < row.size(); i++)
        {
            if (row[i] == '\n')
                throw GCN_EXCEPTION("Line feed not allowed in the row to be added!");
        }

        mRows.push_back(row);
    }

    std::string Text::getRow(unsigned int row) const
    {
        if (row >= mRows.size()) throw GCN_EXCEPTION("Row out of bounds!");

        return mRows[row];
    }

    void Text::insert(int character)
    {
        char c = (char) character;

        if (mRows.empty())
        {
            if (c == '\n')
                mRows.push_back("");
            else
                mRows.push_back(std::string(1, c));
        }
        else
        {
            if (c == '\n')
            {
                mRows.insert(
                    mRows.begin() + mCaretRow + 1,
                    mRows[mCaretRow].substr(mCaretColumn, mRows[mCaretRow].size() - mCaretColumn));
                mRows[mCaretRow].resize(mCaretColumn);
            }
            else
                mRows[mCaretRow].insert(mCaretColumn, std::string(1, c));
        }

        setCaretPosition(getCaretPosition() + 1);
    }

    void Text::remove(int numberOfCharacters)
    {
        if (mRows.empty() || numberOfCharacters == 0) return;

        // We should remove characters left of the caret position.
        if (numberOfCharacters < 0)
        {
            while (numberOfCharacters != 0)
            {
                // If the caret position is zero there is nothing
                // more to do.
                if (mCaretPosition == 0) break;

                // If we are at the end of the row
                // and the row is not the first row we
                // need to merge two rows.
                if (mCaretColumn == 0 && mCaretRow != 0)
                {
                    mRows[mCaretRow - 1] += mRows[mCaretRow];
                    mRows.erase(mRows.begin() + mCaretRow);
                    setCaretRow(mCaretRow - 1);
                    setCaretColumn(getNumberOfCharacters(mCaretRow));
                }
                else
                {
                    mRows[mCaretRow].erase(mCaretColumn - 1, 1);
                    setCaretPosition(mCaretPosition - 1);
                }

                numberOfCharacters++;
            }
        }
        // We should remove characters right of the caret position.
        else if (numberOfCharacters > 0)
        {
            while (numberOfCharacters != 0)
            {
                // If all rows have been removed there is nothing
                // more to do.
                if (mRows.empty()) break;

                // If we are at the end of row and the row
                // is not the last row we need to merge two
                // rows.
                if (mCaretColumn == mRows[mCaretRow].size() && mCaretRow < (mRows.size() - 1))
                {
                    mRows[mCaretRow] += mRows[mCaretRow + 1];
                    mRows.erase(mRows.begin() + mCaretRow + 1);
                }
                else
                {
                    mRows[mCaretRow].erase(mCaretColumn, 1);
                }

                numberOfCharacters--;
            }
        }
    }

    int Text::getCaretPosition() const
    {
        return mCaretPosition;
    }

    void Text::setCaretPosition(int position)
    {
        if (mRows.empty() || position < 0)
        {
            mCaretPosition = 0;
            mCaretRow = 0;
            mCaretColumn = 0;
            return;
        }

        // Loop through all rows until we find the
        // position of the caret.
        unsigned int total = 0;
        for (unsigned int i = 0; i < mRows.size(); i++)
        {
            if (position <= (int) (total + mRows[i].size()))
            {
                mCaretRow = i;
                mCaretColumn = position - total;
                mCaretPosition = position;
                return;
            }

            // Add one for the line feed.
            total += mRows[i].size() + 1;
        }

        // The position is beyond the content.

        // Remove one as the last line doesn't have a line feed.
        mCaretPosition = total - 1;
        mCaretRow = mRows.size() - 1;
        mCaretColumn = mRows[mCaretRow].size();
    }

    void Text::setCaretPosition(int x, int y, Font* font)
    {
        if (mRows.empty()) return;

        setCaretRow(y / font->getHeight());
        setCaretColumn(font->getStringIndexAt(mRows[mCaretRow], x));
    }

    int Text::getCaretColumn() const
    {
        return mCaretColumn;
    }

    int Text::getCaretRow() const
    {
        return mCaretRow;
    }

    void Text::setCaretColumn(int column)
    {
        if (mRows.empty() || column < 0)
            mCaretColumn = 0;
        else if (column > (int) mRows[mCaretRow].size())
            mCaretColumn = mRows[mCaretRow].size();
        else
            mCaretColumn = column;

        calculateCaretPositionFromRowAndColumn();
    }

    void Text::setCaretRow(int row)
    {
        if (mRows.empty() || row < 0)
            mCaretRow = 0;
        else if (row >= (int) mRows.size())
            mCaretRow = mRows.size() - 1;
        else
            mCaretRow = row;

        setCaretColumn(mCaretColumn);
    }

    int Text::getCaretX(Font* font) const
    {
        if (mRows.empty()) return 0;

        return font->getWidth(mRows[mCaretRow].substr(0, mCaretColumn));
    }

    int Text::getCaretY(Font* font) const
    {
        return mCaretRow * font->getHeight();
        ;
    }

    Rectangle Text::getDimension(Font* font) const
    {
        if (mRows.empty()) return Rectangle(0, 0, font->getWidth(" "), font->getHeight());

        int width = 0;
        for (unsigned int i = 0; i < mRows.size(); ++i)
        {
            int w = font->getWidth(mRows[i]);
            if (width < w) width = w;
        }

        return Rectangle(0, 0, width + font->getWidth(" "), font->getHeight() * mRows.size());
    }

    Rectangle Text::getCaretDimension(Font* font) const
    {
        Rectangle dim;
        dim.x = font->getWidth(mRows[mCaretRow].substr(0, mCaretColumn));
        dim.y = font->getHeight() * mCaretRow;
        dim.width = font->getWidth(" ");
        // We add two for some extra spacing to be sure the whole caret is visible.
        dim.height = font->getHeight() + 2;
        return dim;
    }

    int Text::getWidth(int row, Font* font) const
    {
        return 0;
    }

    unsigned int Text::getMaximumCaretRow() const
    {
        return 0;
    }

    unsigned int Text::getMaximumCaretRow(unsigned int row) const
    {
        return 0;
    }

    unsigned int Text::getNumberOfCharacters() const
    {
        unsigned int result = 0;
        for (unsigned int i = 0; i < mRows.size(); ++i)
            result += mRows[i].size() + 1;

        return result;
    }

    unsigned int Text::getNumberOfRows() const
    {
        return mRows.size();
    }

    unsigned int Text::getNumberOfCharacters(unsigned int row) const
    {
        if (row >= mRows.size()) return 0;

        return mRows[row].size();
    }

    void Text::calculateCaretPositionFromRowAndColumn()
    {
        unsigned int total = 0;
        for (unsigned int i = 0; i < mCaretRow; i++)
            // Add one for the line feed.
            total += mRows[i].size() + 1;

        mCaretPosition = total + mCaretColumn;
    }
} // namespace gcn
