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

#ifndef GCN_LISTBOX_HPP
#define GCN_LISTBOX_HPP

#include <list>

#include "guisan/keylistener.hpp"
#include "guisan/listmodel.hpp"
#include "guisan/mouselistener.hpp"
#include "guisan/platform.hpp"
#include "guisan/widget.hpp"

namespace gcn
{
    class SelectionListener;

    /**
     * An implementation of a list box where an item can be selected.
     *
     * To be able display a list the list box uses a user provided list model. 
     * A list model can be any class that implements the ListModel interface.
     *     
     * If an item is selected in the list box a select event will be sent to all selection 
     * listeners of the list box. If an item is selected by using a mouse click or by using 
     * the enter or space key an action event will be sent to all action listeners of the 
     * list box.
     */
    class GCN_CORE_DECLSPEC ListBox :
        public Widget,
        public MouseListener,
        public KeyListener
    {
    public:
        /**
         * Constructor.
         */
        ListBox();

        /**
         * Constructor.
         *
         * @param listModel the list model to use.
         */
        ListBox(ListModel *listModel);

        /**
         * Destructor.
         */
        virtual ~ListBox() { }

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
         * Sets the list model to use.
         *
         * @param listModel the list model to use.
         * @see getListModel
         */
        void setListModel(ListModel *listModel);

        /**
         * Gets the list model used.
         *
         * @return the list model used.
         * @see setListModel
         */
        ListModel *getListModel();

        /**
         * Adjusts the size of the list box to fit it's list model.
         */
        void adjustSize();

        /**
         * Checks whether the list box wraps when selecting items with a keyboard.
         *
         * Wrapping means that the selection of items will be wrapped. That is, if 
         * the first item is selected and up is pressed, the last item will get 
         * selected. If the last item is selected and down is pressed, the first item 
         * will get selected.
         *
         * @return true if wrapping is enabled, fasle otherwise.
         * @see setWrappingEnabled
         */
        bool isWrappingEnabled() const;

        /**
         * Sets the list box to wrap or not when selecting items with a keyboard.
         *
         * Wrapping means that the selection of items will be wrapped. That is, if 
         * the first item is selected and up is pressed, the last item will get 
         * selected. If the last item is selected and down is pressed, the first item 
         * will get selected.
         * 
         * @see isWrappingEnabled
         */
        void setWrappingEnabled(bool wrappingEnabled);

        /**
         * Adds a selection listener to the list box. When the selection
         * changes an event will be sent to all selection listeners of the
         * list box.
         *
         * @param selectionListener The selection listener to add.
         * @since 0.8.0
         */
        void addSelectionListener(SelectionListener* selectionListener);

        /**
         * Removes a selection listener from the list box.
         *
         * @param selectionListener The selection listener to remove.
         * @since 0.8.0
         */
        void removeSelectionListener(SelectionListener* selectionListener);


        // Inherited from Widget

        virtual void draw(Graphics* graphics);

        virtual void drawBorder(Graphics* graphics);

        virtual void logic();


        // Inherited from KeyListener

        virtual void keyPressed(KeyEvent& keyEvent);


        // Inherited from MouseListener

        virtual void mousePressed(MouseEvent& mouseEvent);

        virtual void mouseWheelMovedUp(MouseEvent& mouseEvent);

        virtual void mouseWheelMovedDown(MouseEvent& mouseEvent);
        
        virtual void mouseDragged(MouseEvent& mouseEvent);

        
    protected:
        /**
         * Distributes a value changed event to all selection listeners
         * of the list box.
         * 
         * @since 0.8.0
         */
        void distributeValueChangedEvent();

        /**
         * The list model to use.
         */
        ListModel *mListModel;

        /**
         * The selected item as an index in the list model.
         */
        int mSelected;

        /**
         * True if wrapping is enabled, false otherwise.
         */
        bool mWrappingEnabled;

        /**
         * Typdef.
         */ 
        typedef std::list<SelectionListener*> SelectionListenerList;
        
        /**
         * The selection listeners of the list box.
         */
        SelectionListenerList mSelectionListeners;

        /**
         * Typedef.
         */
        typedef SelectionListenerList::iterator SelectionListenerIterator;
    };
}

#endif // end GCN_LISTBOX_HPP
