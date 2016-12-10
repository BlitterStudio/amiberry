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

#ifndef GCN_DROPDOWN_HPP
#define GCN_DROPDOWN_HPP

#include "guisan/actionlistener.hpp"
#include "guisan/basiccontainer.hpp"
#include "guisan/deathlistener.hpp"
#include "guisan/focushandler.hpp"
#include "guisan/focuslistener.hpp"
#include "guisan/keylistener.hpp"
#include "guisan/listmodel.hpp"
#include "guisan/mouselistener.hpp"
#include "guisan/platform.hpp"
#include "guisan/selectionlistener.hpp"
#include "guisan/widgets/listbox.hpp"
#include "guisan/widgets/scrollarea.hpp"

namespace gcn
{
    /**
     * An implementation of a drop downable list from which an item can be selected.
     * The drop down consists of an internal ScrollArea and an internal ListBox. 
     * The drop down also uses an internal FocusHandler to handle the focus of the 
     * internal ScollArea and the internal ListBox. The scroll area and the list box
     * can be passed to the drop down if a custom scroll area and or a custom list box
     * is preferable.
     *
     * To be able display a list the drop down uses a user provided list model. 
     * A list model can be any class that implements the ListModel interface. 
     *
     * If an item is selected in the drop down a select event will be sent to all selection 
     * listeners of the drop down. If an item is selected by using a mouse click or by using 
     * the enter or space key an action event will be sent to all action listeners of the 
     * drop down.
     */
    class GCN_CORE_DECLSPEC DropDown :
        public ActionListener,
        public BasicContainer,
        public KeyListener,
        public MouseListener,
        public FocusListener,
        public SelectionListener
    {
    public:
        /**
         * Contructor.
         *
         * @param listModel the ListModel to use.
         * @param scrollArea the ScrollArea to use.
         * @param listBox the listBox to use.
         * @see ListModel, ScrollArea, ListBox.
         */
        DropDown(ListModel *listModel = NULL,
                 ScrollArea *scrollArea = NULL,
                 ListBox *listBox = NULL);

        /**
         * Destructor.
         */
        virtual ~DropDown();

        /**
         * Gets the selected item as an index in the list model.
         *
         * @return the selected item as an index in the list model.
         * @see setSelected
         */
        int getSelected() const;

        /**
         * Sets the selected item. The selected item is represented by
         * an index from the list model.
         *
         * @param selected the selected item as an index from the list model.
         * @see getSelected
         */
        void setSelected(int selected);

        /**
         * Sets the list model to use when displaying the list.
         *
         * @param listModel the list model to use.
         * @see getListModel
         */
        void setListModel(ListModel *listModel);

        /**
         * Gets the list model used.
         *
         * @return the ListModel used.
         * @see setListModel
         */
        ListModel *getListModel();

        /**
         * Adjusts the height of the drop down to fit the height of the
         * drop down's parent's height. It's used to not make the drop down
         * draw itself outside of it's parent if folded down.
         */
        void adjustHeight();

        /**
         * Adds a selection listener to the drop down. When the selection
         * changes an event will be sent to all selection listeners of the
         * drop down.
         *
         * @param selectionListener the selection listener to add.
         * @since 0.8.0
         */
        void addSelectionListener(SelectionListener* selectionListener);

        /**
         * Removes a selection listener from the drop down.
         *
         * @param selectionListener the selection listener to remove.
         * @since 0.8.0
         */
        void removeSelectionListener(SelectionListener* selectionListener);


        // Inherited from Widget

        virtual void draw(Graphics* graphics);

        virtual void drawBorder(Graphics* graphics);

        void setBaseColor(const Color& color);

        void setBackgroundColor(const Color& color);

        void setForegroundColor(const Color& color);

        void setFont(Font *font);

        void setSelectionColor(const Color& color);


        // Inherited from BasicContainer

        virtual Rectangle getChildrenArea();


        // Inherited from FocusListener

        virtual void focusLost(const Event& event);


        // Inherited from ActionListener

        virtual void action(const ActionEvent& actionEvent);


        // Inherited from DeathListener

        virtual void death(const Event& event);


        // Inherited from KeyListener

        virtual void keyPressed(KeyEvent& keyEvent);


        // Inherited from MouseListener

        virtual void mousePressed(MouseEvent& mouseEvent);

        virtual void mouseReleased(MouseEvent& mouseEvent);

        virtual void mouseWheelMovedUp(MouseEvent& mouseEvent);

        virtual void mouseWheelMovedDown(MouseEvent& mouseEvent);

        virtual void mouseDragged(MouseEvent& mouseEvent);


	// Inherited from SelectionListener

        virtual void valueChanged(const SelectionEvent& event);


    protected:
        /**
         * Draws the button with the little down arrow.
         *
         * @param graphics a Graphics object to draw with.
         */
        virtual void drawButton(Graphics *graphics);

        /**
         * Sets the DropDown Widget to dropped-down mode.
         */
        virtual void dropDown();

        /**
         * Sets the DropDown Widget to folded-up mode.
         */
        virtual void foldUp();

        bool mDroppedDown;

        /**
         * Distributes a value changed event to all selection listeners
         * of the drop down.
         *
         * @since 0.8.0
         */
        void distributeValueChangedEvent();

        bool mPushed;

        /**
         * Holds what the height is if the drop down is folded up. Used when
         * checking if the list of the drop down was clicked or if the upper part
         * of the drop down was clicked on a mouse click
         */
        int mFoldedUpHeight;
        
        /**
         * The scroll area used.
         */
        ScrollArea* mScrollArea;
        ListBox* mListBox;
        FocusHandler mInternalFocusHandler;
        bool mInternalScrollArea;
        bool mInternalListBox;
        bool mIsDragged;

        /**
         * Typedef.
         */
        typedef std::list<SelectionListener*> SelectionListenerList;
        
        /**
         * The selection listener's of the drop down.
         */
        SelectionListenerList mSelectionListeners;
        
        /**
         * Typedef.
         */
        typedef SelectionListenerList::iterator SelectionListenerIterator;

    };
}

#endif // end GCN_DROPDOWN_HPP
