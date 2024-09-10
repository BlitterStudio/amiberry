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

#ifndef GCN_TEXT_HPP
#define GCN_TEXT_HPP

#include "guisan/platform.hpp"
#include "guisan/rectangle.hpp"

#include <string>
#include <vector>

namespace gcn
{
    class Font;

    /**
     * A utility class to ease working with text in widgets such as
     * TextBox and TextField. The class wraps common text operations
     * such as inserting and deleting text.
     *
     * @since 1.1.0
     */
    class GCN_CORE_DECLSPEC Text
    {
    public:
        /**
         * Constructor.
         * @since 1.1.0
         */
        Text();

        /**
         * Constructor.
         *
         * @param content The content of the text.
         * @since 1.1.0
         */
        Text(const std::string& content);

        /**
         * Destructor.
         * @since 1.1.0
         */
        virtual ~Text();

        /**
         * Sets the content of the text. Will completely remove
         * any previous text and reset the caret position.
         *
         * @param content The content of the text.
         * @since 1.1.0
         */
        virtual void setContent(const std::string& text);

        /**
         * Gets the content of the text.
         *
         * @return The content of the text.
         * @since 1.1.0
         */
        virtual std::string getContent() const;

        /**
         * Sets the content of a row.
         *
         * @param row The row to set the text of.
         * @throws Exception when the row does not exist.
         * @since 1.1.0
         */
        virtual void setRow(unsigned int row, const std::string& content);

        /**
         * Adds a row to the content. Calling this method will
         * not change the current caret position.
         *
         * @param row The row to add.
         * @since 1.1.0
         */
        virtual void addRow(const std::string& row);

        /**
         * Gets the content of a row.
         *
         * @param row The row to get the content of.
         * @return The content of a row.
         * @throws Exception when no such row exists.
         * @since 1.1.0
         */
        virtual std::string getRow(unsigned int row) const;

        /**
         * Inserts a character at the current caret position.
         *
         * @param character The character to insert.
         * @since 1.1.0
         */
        virtual void insert(int character);

        /**
         * Removes a given number of characters at starting
         * at the current caret position.
         *
         * If the number of characters to remove is negative
         * characters will be removed left of the caret position.
         * If the number is positive characters will be removed
         * right of the caret position. If a line feed is
         * removed the row with the line feed will be merged
         * with the row above the line feed.
         *
         * @param numberOfCharacters The number of characters to remove.
         * @since 1.1.0
         */
        virtual void remove(int numberOfCharacters);

        /**
         * Gets the caret position.
         *
         * @return The caret position.
         * @since 1.1.0
         */
        virtual int getCaretPosition() const;

        /**
         * Sets the caret position. The position will be
         * clamp to the dimension of the content.
         *
         * @param position The position of the caret.
         * @since 1.1.0
         */
        virtual void setCaretPosition(int position);

        /**
         * Sets the caret position given an x and y coordinate in pixels
         * relative to the text. The coordinates will be clamp to the content.
         *
         * @param x The x coordinate of the caret.
         * @param y The y coordinate of the caret.
         * @param font The font to use when calculating the position.
         * @since 1.1.0
         */
        virtual void setCaretPosition(int x, int y, Font* font);

        /**
         * Gets the column the caret is currently in.
         *
         * @return The column the caret is currently in.
         * @since 1.1.0
         */
        virtual int getCaretColumn() const;

        /**
         * Gets the row the caret is currently in.
         *
         * @return The row the caret is currently in.
         * @since 1.1.0
         */
        virtual int getCaretRow() const;

        /**
         * Sets the column the caret should be in. The column
         * will be clamp to the current row.
         *
         * @param column The column the caret should be in.
         * @since 1.1.0
         */
        virtual void setCaretColumn(int column);

        /**
         * Sets the row the caret should be in. If the row lies
         * outside of the text, the row will be set to zero or the
         * maximum row depending on where the row lies outside of the
         * text.
         *
         * Calling this function trigger a recalculation of the caret
         * column.
         *
         * @param row The row the caret should be in.
         * @since 1.1.0
         */
        virtual void setCaretRow(int row);

        /**
         * Gets the x coordinate of the caret in pixels given a font.
         *
         * @param font The font to use when calculating the x coordinate.
         * @return The x coorinate of the caret in pixels.
         * @since 1.1.0
         */
        virtual int getCaretX(Font* font) const;

        /**
         * Gets the y coordinate of the caret in pixels given a font.
         *
         * @param font The font to use when calculating the y coordinate.
         * @return The y coorinate of the caret in pixels.
         * @since 1.1.0
         */
        virtual int getCaretY(Font* font) const;

        /**
         * Gets the dimension in pixels of the text given a font. If there
         * is no text present a dimension of a white space will be returned.
         *
         * @param font The font to use when calculating the dimension.
         * @return The dimension in pixels of the text given a font.
         * @since 1.1.0
         */
        virtual Rectangle getDimension(Font* font) const;

        /**
         * Gets the caret dimension relative to this text.
         * The returned dimension is perfect for use with Widget::showPart
         * so the caret is always shown.
         *
         * @param font The font to use when calculating the dimension.
         * @return The dimension of the caret.
         * @since 1.1.0
         */
        virtual Rectangle getCaretDimension(Font* font) const;

        /**
         * Gets the width in pixels of a row. If the row is not
         * present in the text zero will be returned.
         *
         * @param row The row to get the width of.
         * @return The width in pixels of a row.
         * @since 1.1.0
         */
        virtual int getWidth(int row, Font* font) const;

        /**
         * Gets the maximum row the caret can be in.
         *
         * @return The maximum row the caret can be in.
         * @since 1.1.0
         */
        virtual unsigned int getMaximumCaretRow() const;

        /**
         * Gets the maximum column of a row the caret can be in.
         *
         * @param row The row of the caret.
         * @return The maximum column of a row the caret can be in.
         * @since 1.1.0
         */
        virtual unsigned int getMaximumCaretRow(unsigned int row) const;

        /**
         * Gets the number of rows in the text.
         *
         * @return The number of rows in the text.
         * @since 1.1.0
         */
        virtual unsigned int getNumberOfRows() const;

        /**
         * Gets the number of characters in the text.
         *
         * @return The number of characters in the text.
         * @since 1.1.0
         */
        virtual unsigned int getNumberOfCharacters() const;

        /**
         * Gets the number of characters in a certain row in the text.
         * If the row does not exist, zero will be returned.
         *
         * @param row The row to get the number of characters in.
         * @return The number of characters in a certain row, or zero
         *         if the row does not exist.
         * @since 1.1.0
         */
        virtual unsigned int getNumberOfCharacters(unsigned int row) const;

    protected:
        /**
         * Calculates the caret position from the caret row and caret column.
         */
        void calculateCaretPositionFromRowAndColumn();

        /**
         * Holds the text row by row.
         */
        std::vector<std::string> mRows;

        /**
         * Holds the position of the caret. This variable should
         * always be valid.
         */
        unsigned int mCaretPosition;

        /**
         * Holds the row the caret is in. This variable should always
         * be valid.
         */
        unsigned int mCaretRow;

        /**
         * Holds the column the caret is in. This variable should always
         * be valid.
         */
        unsigned int mCaretColumn;
    };
} // namespace gcn
#endif
