/*      _______   __   __   __   ______   __   __   _______   __   __
 *     / _____/\ / /\ / /\ / /\ / ____/\ / /\ / /\ / ___  /\ /  |\/ /\
 *    / /\____\// / // / // / // /\___\// /_// / // /\_/ / // , |/ / /
 *   / / /__   / / // / // / // / /    / ___  / // ___  / // /| ' / /
 *  / /_// /\ / /_// / // / // /_/_   / / // / // /\_/ / // / |  / /
 * /______/ //______/ //_/ //_____/\ /_/ //_/ //_/ //_/ //_/ /|_/ /
 * \______\/ \______\/ \_\/ \_____\/ \_\/ \_\/ \_\/ \_\/ \_\/ \_\/
 *
 * Copyright (c) 2007 - 2008 Josh Matthews and Olof Naessén
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

#include "guisan/widgets/adjustingcontainer.hpp"

#include <guisan.hpp>

namespace gcn
{
    namespace contrib
    {
        AdjustingContainer::AdjustingContainer()
                : mWidth(0),
                  mHeight(0),
                  mNumberOfColumns(1),
                  mNumberOfRows(1),
                  mPaddingLeft(0),
                  mPaddingRight(0),
                  mPaddingTop(0),
                  mPaddingBottom(0),
                  mVerticalSpacing(0),
                  mHorizontalSpacing(0)
            
            
        {
            mColumnWidths.push_back(0);
            mRowHeights.push_back(0);
        }
    
        AdjustingContainer::~AdjustingContainer()
        {

        }

        void AdjustingContainer::setNumberOfColumns(unsigned int numberOfColumns)
        {
            mNumberOfColumns = numberOfColumns;
        
            if (mColumnAlignment.size() < numberOfColumns)
            {
                while (mColumnAlignment.size() < numberOfColumns)
                {
                    mColumnAlignment.push_back(LEFT);
                }
            }
            else
            {
                while (mColumnAlignment.size() >  numberOfColumns)
                {
                    mColumnAlignment.pop_back();
                }
            }
        }

        void AdjustingContainer::setColumnAlignment(unsigned int column,
                                                    unsigned int alignment)
        {
            if (column < mColumnAlignment.size())
            {
                mColumnAlignment[column] = alignment;
            }
        }

        void AdjustingContainer::setPadding(unsigned int paddingLeft,
                                            unsigned int paddingRight,
                                            unsigned int paddingTop,
                                            unsigned int paddingBottom)
        {
            mPaddingLeft = paddingLeft;
            mPaddingRight = paddingRight;
            mPaddingTop = paddingTop;
            mPaddingBottom = paddingBottom;
        }

        void AdjustingContainer::setVerticalSpacing(unsigned int verticalSpacing)
        {
            mVerticalSpacing = verticalSpacing;
        }

        void AdjustingContainer::setHorizontalSpacing(unsigned int horizontalSpacing)
        {
            mHorizontalSpacing = horizontalSpacing;
        }

        void AdjustingContainer::logic()
        {
            Container::logic();
            adjustContent();
        }
    
        void AdjustingContainer::add(Widget *widget)
        {
            Container::add(widget);
            mContainedWidgets.push_back(widget);
        }
    
        void AdjustingContainer::add(Widget *widget, int x, int y)
        {
            add(widget);
        }
    
        void AdjustingContainer::clear()
        {
            Container::clear();
            mContainedWidgets.clear();
        }

        void AdjustingContainer::remove(Widget *widget)
        {
            Container::remove(widget);
            std::vector<gcn::Widget *>::iterator it;
            for(it = mContainedWidgets.begin(); it != mContainedWidgets.end(); it++)
            {
                if(*it == widget)
                {
                    mContainedWidgets.erase(it);
                    break;
                }
            }
        }

        void AdjustingContainer::adjustSize()
        {
            mNumberOfRows = mContainedWidgets.size()
            / mNumberOfColumns + mContainedWidgets.size() % mNumberOfColumns;

            mColumnWidths.clear();

            unsigned int i;
            for (i = 0; i < mNumberOfColumns; i++)
            {
                mColumnWidths.push_back(0);
            }
        
            mRowHeights.clear();

            for (i = 0; i < mNumberOfRows; i++)
            {
                mRowHeights.push_back(0);
            }

            for (i = 0; i < mNumberOfColumns; i++)
            {
                unsigned int j;
                for (j = 0; j < mNumberOfRows && mNumberOfColumns * j + i < mContainedWidgets.size(); j++)
                {
                    if ((unsigned int)mContainedWidgets[mNumberOfColumns * j + i]->getWidth() > mColumnWidths[i])
                    {
                        mColumnWidths[i] = mContainedWidgets[mNumberOfColumns * j + i]->getWidth();
                    }
                    if ((unsigned int)mContainedWidgets[mNumberOfColumns * j + i]->getHeight() > mRowHeights[j])
                    {
                        mRowHeights[j] = mContainedWidgets[mNumberOfColumns * j + i]->getHeight();
                    }
                }
            }

            mWidth = mPaddingLeft;

            for (i = 0; i < mColumnWidths.size(); i++)
            {
                mWidth += mColumnWidths[i] + mHorizontalSpacing;
            }

            mWidth -= mHorizontalSpacing;
            mWidth += mPaddingRight;

            mHeight = mPaddingTop;

            for (i = 0; i < mRowHeights.size(); i++)
            {
                mHeight += mRowHeights[i] + mVerticalSpacing;
            }
        
            mHeight -= mVerticalSpacing;
            mHeight += mPaddingBottom;

            setHeight(mHeight);
            setWidth(mWidth);
        }

        void AdjustingContainer::adjustContent()
        {
            adjustSize();

            unsigned int columnCount = 0;
            unsigned int rowCount = 0;
            unsigned int y = mPaddingTop;

            for (unsigned int i = 0; i < mContainedWidgets.size(); i++)
            {
                unsigned  basex;
                if (columnCount % mNumberOfColumns)
                {
                    basex = mPaddingLeft;
                    unsigned int j;

                    for (j = 0; j < columnCount; j++)
                    {
                        basex += mColumnWidths[j] + mHorizontalSpacing;
                    }
                }
                else
                {
                    basex = mPaddingLeft;
                }

                switch (mColumnAlignment[columnCount])
                {
                    case LEFT:
                        mContainedWidgets[i]->setX(basex);
                        break;
                    case CENTER:
                        mContainedWidgets[i]->setX(basex + (mColumnWidths[columnCount] - mContainedWidgets[i]->getWidth()) / 2);
                        break;
                    case RIGHT:
                        mContainedWidgets[i]->setX(basex + mColumnWidths[columnCount] - mContainedWidgets[i]->getWidth());
                        break;
                    default:
                        throw GCN_EXCEPTION("Unknown alignment.");
                }

                mContainedWidgets[i]->setY(y);
                columnCount++;

                if (columnCount == mNumberOfColumns)
                {
                    columnCount = 0;
                    y += mRowHeights[rowCount] + mVerticalSpacing;
                    rowCount++;
                }
            }
        }
    }
}