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

#ifndef GCN_TABBEDAREA_HPP
#define GCN_TABBEDAREA_HPP

#include <map>
#include <string>
#include <vector>

#include "guisan/actionlistener.hpp"
#include "guisan/basiccontainer.hpp"
#include "guisan/keylistener.hpp"
#include "guisan/mouselistener.hpp"
#include "guisan/platform.hpp"

namespace gcn
{
    class Container;
    class Tab;

    /**
     * An implementation of a tabbed area where a user can display a widget by
     * selecting a tab.
     *
     * @since 0.8.0
     */
    class GCN_CORE_DECLSPEC TabbedArea:
        public ActionListener,
        public BasicContainer,
        public KeyListener,
        public MouseListener
    {
        friend class Tab;

    public:

        /**
         * Constructor.
         */
        TabbedArea();

        /**
         * Destructor.
         */
        virtual ~TabbedArea();

        /**
         * Sets the tabbed area to be opaque or not. If the tabbed area is
         * opaque its background will be drawn, if it's not opaque its
         * background will not be drawn. By default, a tabbed area is not
         * opaque.
         *
         * The tabbed area's background is normally only visible behind the
         * tabs, since the container holding the tab contents is opaque by
         * default.
         *
         * @param opaque True if the tabbed area should be opaque, false
         *               otherwise.
         * @see isOpaque
         */
        void setOpaque(bool opaque);

        /**
         * Checks if the tabbed area is opaque or not.
         *
         * @return true if the tabbed area is opaque, false otherwise.
         * @see setOpaque
         */
        bool isOpaque() const;

        /**
         * Adds a tab to the tabbed area. The newly created tab will be
         * automatically deleted by the tabbed area when it is removed.
         *
         * @param caption The caption of the tab to add.
         * @param widget The widget to view when the tab is selected.
         * @see removeTab, removeTabWithIndex
         */
        virtual void addTab(const std::string& caption, Widget* widget);

        /**
         * Adds a tab to the tabbed area. The tab will not be deleted by the
         * tabbed area when it is removed.
         *
         * @param tab The tab widget for the tab.
         * @param widget The widget to view when the tab is selected.
         * @see removeTab, removeTabWithIndex
         */
        virtual void addTab(Tab* tab, Widget* widget);

        /**
         * Removes a tab from the tabbed area.
         *
         * @param index The index of the tab to remove.
         * @see addTab
         */
        virtual void removeTabWithIndex(unsigned int index);

        /**
         * Removes a tab from the tabbed area.
         *
         * @param index The tab to remove.
         * @see addTab
         */
        virtual void removeTab(Tab* tab);

        /**
         * Checks if a tab given an index is selected or not.
         *
         * @param index The index of the tab to check.
         * @return True if the tab is selected, false otherwise.
         * @see setSelectedTab
         */
        virtual bool isTabSelected(unsigned int index) const;

        /**
         * Checks if a tab is selected or not.
         *
         * @param tab The tab to check.
         * @return True if the tab is selected, false otherwise.
         * @see setSelectedTab
         */
        virtual bool isTabSelected(Tab* tab);

        /**
         * Sets a tab given an index to be selected.
         *
         * @param index The index of the tab to be selected.
         * @see isTabSelected, getSelectedTab
         */
        virtual void setSelectedTab(unsigned int index);

        /**
         * Sets a tab to be selected or not.
         *
         * @param index The tab to be selected.
         * @see isTabSelected, getSelectedTab
         */
        virtual void setSelectedTab(Tab* tab);

        /**
         * Gets the index of the selected tab.
         *
         * @return The index of the selected tab.
         *         If no tab is selected -1 will be returned.
         * @see isTabSelected, setSelectedTab
         */
        virtual int getSelectedTabIndex() const;

        /**
         * Gets the selected tab.
         *
         * @return The selected tab.
         * @see isTabSelected, setSelectedTab
         */
        Tab* getSelectedTab();


        // Inherited from Widget

        virtual void draw(Graphics *graphics);

        void setWidth(int width);

        void setHeight(int height);

        void setSize(int width, int height);

        void setDimension(const Rectangle& dimension);

        void setBaseColor(const Color& color);

        // Inherited from ActionListener

        void action(const ActionEvent& actionEvent);

        // Inherited from DeathListener

        virtual void death(const Event& event);


        // Inherited from KeyListener

        virtual void keyPressed(KeyEvent& keyEvent);


        // Inherited from MouseListener
        
        virtual void mousePressed(MouseEvent& mouseEvent);

        
    protected:
        /**
         * Adjusts the size of the tab container and the widget container.
         */
        void adjustSize();

        /**
         * Adjusts the positions of the tabs.
         */
        void adjustTabPositions();

        /**
         * Holds the selected tab.
         */
        Tab* mSelectedTab;

        /**
         * Holds the container for the tabs.
         */
        Container* mTabContainer;

        /**
         * Holds the container for the widgets.
         */
        Container* mWidgetContainer;

        /**
         * Holds a vector of tabs to delete in the destructor.
         * A tab that is to be deleted is a tab that has been
         * internally created by the tabbed area.
         */
        std::vector<Tab*> mTabsToDelete;

        /**
         * A map between a tab and a widget to display when the
         * tab is selected.
         */
        std::vector<std::pair<Tab*, Widget*> > mTabs;

        /**
         * True if the tabbed area is opaque, false otherwise.
         */
        bool mOpaque;
    };
}

#endif // end GCN_TABBEDAREA_HPP
