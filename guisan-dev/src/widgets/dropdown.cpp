/*      _______   __   __   __   ______   __   __   _______   __   __
 *     / _____/\ / /\ / /\ / /\ / ____/\ / /\ / /\ / ___  /\ /  |\/ /\
 *    / /\____\// / // / // / // /\___\// /_// / // /\_/ / // , |/ / /
 *   / / /__   / / // / // / // / /    / ___  / // ___  / // /| ' / /
 *  / /_// /\ / /_// / // / // /_/_   / / // / // /\_/ / // / |  / /
 * /______/ //______/ //_/ //_____/\ /_/ //_/ //_/ //_/ //_/ /|_/ /
 * \______\/ \______\/ \_\/ \_____\/ \_\/ \_\/ \_\/ \_\/ \_\/ \_\/
 *
 * Copyright (c) 2004, 2005, 2006, 2007 Olof Naessén and Per Larsson
 *
 *                                                         Js_./
 * Per Larsson a.k.a finalman                          _RqZ{a<^_aa
 * Olof Naessén a.k.a jansem/yakslem                _asww7!uY`>  )\a//
 *                                                 _Qhm`] _f "'c  1!5m
 * Visit: http://guichan.darkbits.org             )Qk<P ` _: :+' .'  "{[
 *                                               .)j(] .d_/ '-(  P .   S
 * License: (BSD)                                <Td/Z <fP"5(\"??"\a.  .L
 * Redistribution and use in source and          _dV>ws?a-?'      ._/L  #'
 * binary forms, with or without                 )4d[#7r, .   '     )d`)[
 * modification, are permitted provided         _Q-5'5W..j/?'   -?!\)cam'
 * that the following conditions are met:       j<<WP+k/);.        _W=j f
 * 1. Redistributions of source code must       .$%w\/]Q  . ."'  .  mj$
 *    retain the above copyright notice,        ]E.pYY(Q]>.   a     J@\
 *    this list of conditions and the           j(]1u<sE"L,. .   ./^ ]{a
 *    following disclaimer.                     4'_uomm\.  )L);-4     (3=
 * 2. Redistributions in binary form must        )_]X{Z('a_"a7'<a"a,  ]"[
 *    reproduce the above copyright notice,       #}<]m7`Za??4,P-"'7. ).m
 *    this list of conditions and the            ]d2e)Q(<Q(  ?94   b-  LQ/
 *    following disclaimer in the                <B!</]C)d_, '(<' .f. =C+m
 *    documentation and/or other materials      .Z!=J ]e []('-4f _ ) -.)m]'
 *    provided with the distribution.          .w[5]' _[ /.)_-"+?   _/ <W"
 * 3. Neither the name of Guichan nor the      :$we` _! + _/ .        j?
 *    names of its contributors may be used     =3)= _f  (_yQmWW$#(    "
 *    to endorse or promote products derived     -   W,  sQQQQmZQ#Wwa]..
 *    from this software without specific        (js, \[QQW$QWW#?!V"".
 *    prior written permission.                    ]y:.<\..          .
 *                                                 -]n w/ '         [.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT       )/ )/           !
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY         <  (; sac    ,    '
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING,               ]^ .-  %
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF            c <   r
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR            aga<  <La
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE          5%  )P'-3L
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR        _bQf` y`..)a
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,          ,J?4P'.P"_(\?d'.,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES               _Pa,)!f/<[]/  ?"
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT      _2-..:. .r+_,.. .
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     ?a.<%"'  " -'.a_ _,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION)                     ^
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "guisan/widgets/dropdown.hpp"

#include "guisan/exception.hpp"
#include "guisan/font.hpp"
#include "guisan/graphics.hpp"
#include "guisan/key.hpp"
#include "guisan/listmodel.hpp"
#include "guisan/mouseinput.hpp"
#include "guisan/widgets/listbox.hpp"
#include "guisan/widgets/scrollarea.hpp"

namespace gcn
{
    DropDown::DropDown(ListModel *listModel,
                       ScrollArea *scrollArea,
                       ListBox *listBox)
    {
        setWidth(100);
        setFocusable(true);
        mDroppedDown = false;
        mPushed = false;
        mIsDragged = false;

        setInternalFocusHandler(&mInternalFocusHandler);

        mInternalScrollArea = (scrollArea == NULL);
        mInternalListBox = (listBox == NULL);

        if (mInternalScrollArea)
        {
            mScrollArea = new ScrollArea();
        }
        else
        {
            mScrollArea = scrollArea;
        }

        if (mInternalListBox)
        {
            mListBox = new ListBox();
        }
        else
        {
            mListBox = listBox;
        }

        mScrollArea->setContent(mListBox);
        add(mScrollArea);

        mListBox->addActionListener(this);
        mListBox->addSelectionListener(this);

        setListModel(listModel);

        if (mListBox->getSelected() < 0)
        {
            mListBox->setSelected(0);
        }

        addMouseListener(this);
        addKeyListener(this);
        addFocusListener(this);

        adjustHeight();
        setBorderSize(1);
    }

    DropDown::~DropDown()
    {
	if (widgetExists(mListBox))
		mListBox->removeSelectionListener(this);

	if (mInternalScrollArea)
		delete mScrollArea;

	if (mInternalListBox)
		delete mListBox;

	setInternalFocusHandler(NULL);
    }

    void DropDown::draw(Graphics* graphics)
    {
        int h;

        if (mDroppedDown)
        {
            h = mFoldedUpHeight;
        }
        else
        {
            h = getHeight();
        }

        int alpha = getBaseColor().a;
        Color faceColor = getBaseColor();
        faceColor.a = alpha;
        Color highlightColor = faceColor + 0x303030;
        highlightColor.a = alpha;
        Color shadowColor = faceColor - 0x303030;
        shadowColor.a = alpha;


        graphics->setColor(getBackgroundColor());
        graphics->fillRectangle(Rectangle(0, 0, getWidth(), h));

        graphics->setColor(getForegroundColor());
        graphics->setFont(getFont());

        if (isFocused())
        {
            graphics->setColor(getSelectionColor());
            graphics->fillRectangle(Rectangle(0, 0, getWidth() - h, h));
            graphics->setColor(getForegroundColor());
        }

        if (mListBox->getListModel() && mListBox->getSelected() >= 0)
        {
            graphics->drawText(mListBox->getListModel()->getElementAt(mListBox->getSelected()), 1, 0);
        }

        drawButton(graphics);

         if (mDroppedDown)
         {
             drawChildren(graphics);

            // Draw two lines separating the ListBox with se selected
            // element view.
            graphics->setColor(highlightColor);
            graphics->drawLine(0, h, getWidth(), h);
            graphics->setColor(shadowColor);
            graphics->drawLine(0, h + 1,getWidth(),h + 1);
         }
    }

    void DropDown::drawBorder(Graphics* graphics)
    {
        Color faceColor = getBaseColor();
        Color highlightColor, shadowColor;
        int alpha = getBaseColor().a;
        int width = getWidth() + getBorderSize() * 2 - 1;
        int height = getHeight() + getBorderSize() * 2 - 1;
        highlightColor = faceColor + 0x303030;
        highlightColor.a = alpha;
        shadowColor = faceColor - 0x303030;
        shadowColor.a = alpha;

        unsigned int i;
        for (i = 0; i < getBorderSize(); ++i)
        {
            graphics->setColor(shadowColor);
            graphics->drawLine(i,i, width - i, i);
            graphics->drawLine(i,i + 1, i, height - i - 1);
            graphics->setColor(highlightColor);
            graphics->drawLine(width - i,i + 1, width - i, height - i);
            graphics->drawLine(i,height - i, width - i - 1, height - i);
        }
    }

    void DropDown::drawButton(Graphics *graphics)
    {
        Color faceColor, highlightColor, shadowColor;
        int offset;
        int alpha = getBaseColor().a;

        if (mPushed)
        {
            faceColor = getBaseColor() - 0x303030;
            faceColor.a = alpha;
            highlightColor = faceColor - 0x303030;
            highlightColor.a = alpha;
            shadowColor = faceColor + 0x303030;
            shadowColor.a = alpha;
            offset = 1;
        }
        else
        {
            faceColor = getBaseColor();
            faceColor.a = alpha;
            highlightColor = faceColor + 0x303030;
            highlightColor.a = alpha;
            shadowColor = faceColor - 0x303030;
            shadowColor.a = alpha;
            offset = 0;
        }

        int h;
        if (mDroppedDown)
        {
            h = mFoldedUpHeight;
        }
        else
        {
            h = getHeight();
        }
        int x = getWidth() - h;
        int y = 0;

        graphics->setColor(faceColor);
        graphics->fillRectangle(Rectangle(x + 1, 
                                          y + 1, 
                                          h - 2, 
                                          h - 2));

        graphics->setColor(highlightColor);
        graphics->drawLine(x, 
                           y, 
                           x + h - 1, 
                           y);
        graphics->drawLine(x, 
                           y + 1, 
                           x, 
                           y + h - 1);

        graphics->setColor(shadowColor);
        graphics->drawLine(x + h - 1, 
                           y + 1, 
                           x + h - 1, 
                           y + h - 1);
        graphics->drawLine(x + 1, 
                           y + h - 1, 
                           x + h - 2, 
                           y + h - 1);
        graphics->setColor(getForegroundColor());

        int i;
        int hh = h / 3;
        int hx = x + h / 2;
        int hy = y + (h * 2) / 3;
        for (i = 0; i < hh; i++)
        {
            graphics->drawLine(hx - i + offset,
                               hy - i + offset,
                               hx + i + offset,
                               hy - i + offset);
        }
    }

    int DropDown::getSelected() const
    {
        return mListBox->getSelected();
    }

    void DropDown::setSelected(int selected)
    {
        if (selected >= 0)
        {
            mListBox->setSelected(selected);
        }
    }

    void DropDown::keyPressed(KeyEvent& keyEvent)
    {
        Key key = keyEvent.getKey();

        if ((key.getValue() == Key::ENTER || key.getValue() == Key::SPACE)
            && !mDroppedDown)
        {
            dropDown();
            keyEvent.consume();
        }
        else if (key.getValue() == Key::UP)
        {
            setSelected(getSelected() - 1);
            keyEvent.consume();
        }
        else if (key.getValue() == Key::DOWN)
        {
            setSelected(getSelected() + 1);
            keyEvent.consume();
        }
    }

    void DropDown::mousePressed(MouseEvent& mouseEvent)
    {        
        // If we have a mouse press on the widget.
        if (0 <= mouseEvent.getY()
            && mouseEvent.getY() < getHeight()
            && mouseEvent.getX() >= 0
            && mouseEvent.getX() < getWidth()
            && mouseEvent.getButton() == MouseEvent::LEFT
            && !mDroppedDown
            && mouseEvent.getSource() == this)
        {
            mPushed = true;
            dropDown();
            requestModalMouseInputFocus();
        }
        // Fold up the listbox if the upper part is clicked after fold down
        else if (0 <= mouseEvent.getY()
                 && mouseEvent.getY() < mFoldedUpHeight
                 && mouseEvent.getX() >= 0
                 && mouseEvent.getX() < getWidth()
                 && mouseEvent.getButton() == MouseEvent::LEFT
                 && mDroppedDown
                 && mouseEvent.getSource() == this)
        {
            mPushed = false;
            foldUp();
            releaseModalMouseInputFocus();
        }
        // If we have a mouse press outside the widget
        else if (0 > mouseEvent.getY()
                 || mouseEvent.getY() >= getHeight()
                 || mouseEvent.getX() < 0
                 || mouseEvent.getX() >= getWidth())
        {
            mPushed = false;
            foldUp();
        }
    }

    void DropDown::mouseReleased(MouseEvent& mouseEvent)
    {
        if (mIsDragged)
        {
            mPushed = false;
        }

        // Released outside of widget. Can happen when we have modal input focus.
        if (0 > mouseEvent.getY()
            || mouseEvent.getY() >= getHeight()
            || mouseEvent.getX() < 0
            || mouseEvent.getX() >= getWidth()
            && mouseEvent.getButton() == MouseEvent::LEFT
            && hasModalMouseInputFocus())
        {
            releaseModalMouseInputFocus();

            if (mIsDragged)
            {
                foldUp();
            }
        }
        else if (mouseEvent.getButton() == MouseEvent::LEFT)
        {
            mPushed = false;
        }

        mIsDragged = false;
    }

    void DropDown::mouseDragged(MouseEvent& mouseEvent)
    {
        mIsDragged = true;

        mouseEvent.consume();
    }

    void DropDown::setListModel(ListModel *listModel)
    {
        mListBox->setListModel(listModel);

        if (mListBox->getSelected() < 0)
        {
            mListBox->setSelected(0);
        }

        adjustHeight();
    }

    ListModel *DropDown::getListModel()
    {
        return mListBox->getListModel();
    }

    void DropDown::adjustHeight()
    {
        if (mScrollArea == NULL)
            throw GCN_EXCEPTION("Scroll Area has been deleted.");

	if (mListBox == NULL)
		throw GCN_EXCEPTION("List box has been deleted.");

        int listBoxHeight = mListBox->getHeight();
        int h2 = getFont()->getHeight();

        setHeight(h2);

        // The addition/subtraction of 2 compensates for the seperation lines
        // seperating the selected element view and the scroll area.

        if (mDroppedDown && getParent())
        {
            int h = getParent()->getChildrenArea().height - getY();

            if (listBoxHeight > h - h2 - 2)
            {
                mScrollArea->setHeight(h - h2 - 2);
                setHeight(h);
            }
            else
            {
                setHeight(listBoxHeight + h2 + 2);
                mScrollArea->setHeight(listBoxHeight);
            }
        }

        mScrollArea->setWidth(getWidth());
        // Resize the ListBox to exactly fit the ScrollArea.
        mListBox->setWidth(mScrollArea->getChildrenArea().width);
        mScrollArea->setPosition(0, 0);
    }

    void DropDown::dropDown()
    {
        if (!mDroppedDown)
        {
            mDroppedDown = true;
            mFoldedUpHeight = getHeight();
            adjustHeight();

            if (getParent())
            {
                getParent()->moveToTop(this);
            }
        }

        mListBox->requestFocus();
    }

    void DropDown::foldUp()
    {
        if (mDroppedDown)
        {
            mDroppedDown = false;
            adjustHeight();
            mInternalFocusHandler.focusNone();
        }
    }

    void DropDown::focusLost(const Event& event)
    {
        foldUp();
        mInternalFocusHandler.focusNone();
    }


    void DropDown::death(const Event& event)
    {
        if (event.getSource() == mScrollArea)
        {
            mScrollArea = NULL;
        }
        BasicContainer::death(event);
    }

    void DropDown::action(const ActionEvent& actionEvent)
    {
        foldUp();
        releaseModalMouseInputFocus();
        generateAction();
    }

    Rectangle DropDown::getChildrenArea()
    {
        if (mDroppedDown)
        {
            return Rectangle(0, 
                             mFoldedUpHeight + 2, 
                             getWidth(), 
                             getHeight() - mFoldedUpHeight);
        }

        return Rectangle();
    }

    void DropDown::setBaseColor(const Color& color)
    {
        if (mInternalScrollArea)
        {
            mScrollArea->setBaseColor(color);
        }

        if (mInternalListBox)
        {
            mListBox->setBaseColor(color);
        }

        Widget::setBaseColor(color);
    }

    void DropDown::setBackgroundColor(const Color& color)
    {
        if (mInternalScrollArea)
        {
            mScrollArea->setBackgroundColor(color);
        }

        if (mInternalListBox)
        {
            mListBox->setBackgroundColor(color);
        }

        Widget::setBackgroundColor(color);
    }

    void DropDown::setForegroundColor(const Color& color)
    {
        if (mInternalScrollArea)
        {
            mScrollArea->setForegroundColor(color);
        }

        if (mInternalListBox)
        {
            mListBox->setForegroundColor(color);
        }

        Widget::setForegroundColor(color);
    }

	void DropDown::setFont(Font *font)
	{
		if (mInternalScrollArea)
        {
            mScrollArea->setFont(font);
        }

        if (mInternalListBox)
        {
            mListBox->setFont(font);
        }

        Widget::setFont(font);
	}

	void DropDown::mouseWheelMovedUp(MouseEvent& mouseEvent)
	{
        if (isFocused() && mouseEvent.getSource() == this)
        {                   
            mouseEvent.consume();

            if (mListBox->getSelected() > 0)
            {
                mListBox->setSelected(mListBox->getSelected() - 1);
            }
        }
    }

    void DropDown::mouseWheelMovedDown(MouseEvent& mouseEvent)
    {
        if (isFocused() && mouseEvent.getSource() == this)
        {            
            mouseEvent.consume();

            mListBox->setSelected(mListBox->getSelected() + 1);
        }
    }

    void DropDown::setSelectionColor(const Color& color)
    {
        Widget::setSelectionColor(color);
        
        if (mInternalListBox)
        {
            mListBox->setSelectionColor(color);
        }       
    }

    void DropDown::valueChanged(const SelectionEvent& event)
    {
        distributeValueChangedEvent();
    }

    void DropDown::addSelectionListener(SelectionListener* selectionListener)
    {
        mSelectionListeners.push_back(selectionListener);
    }
   
    void DropDown::removeSelectionListener(SelectionListener* selectionListener)
    {
        mSelectionListeners.remove(selectionListener);
    }

    void DropDown::distributeValueChangedEvent()
    {
        SelectionListenerIterator iter;

        for (iter = mSelectionListeners.begin(); iter != mSelectionListeners.end(); ++iter)
        {
            SelectionEvent event(this);
            (*iter)->valueChanged(event);
        }
    }
}

