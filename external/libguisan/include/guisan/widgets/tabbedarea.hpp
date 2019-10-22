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
     * With the tabbed area widget several widgets can share the same
     * space. The widget to view is selected by the user by using tabs.
     */
    class GCN_CORE_DECLSPEC TabbedArea:
        public ActionListener,
        public BasicContainer,
        public KeyListener,
        public MouseListener
    {
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
         * Adds a tab to the tabbed area.
         *
         * @param caption The caption of the tab.
         * @param widget The widget to view when the tab is selected.         
         */
        virtual void addTab(const std::string& caption, Widget* widget);

        /**
         * Adds a tab to the tabbed area.
         *
         * @param tab The tab widget for the tab.
         * @param widget The widget to view when the tab is selected.         
         */
        virtual void addTab(Tab* tab, Widget* widget);

        /**
         * Removes a tab from the tabbed area.
         *
         * @param index The index of the tab to remove.
         */
        virtual void removeTabWithIndex(unsigned int index);
        
        /**
         * Removes a tab from the tabbed area.
         *
         * @param index The tab to remove.
         */
        virtual void removeTab(Tab* tab);

        /**
         * Checks whether a tab given an index is selected.
         *
         * @param index The index of the tab to check.
         * @return True if the tab is selected, false otherwise.
         */
        virtual bool isTabSelected(unsigned int index) const;
        
        /**
         * Checks whether a tab is selected or not.
         *
         * @param index The tab to check.
         * @return True if the tab is selected, false otherwise.
         */
        virtual bool isTabSelected(Tab* tab);

        /**
         * Sets a tab given an index to be selected.
         *
         * @param index The index of the tab to be selected.
         */
        virtual void setSelectedTabWithIndex(unsigned int index);

        /**
         * Sets a tab to be selected or not.
         *
         * @param index The tab to be selected.
         */
        virtual void setSelectedTab(Tab* tab);

        /**
         * Gets the index of the selected tab.
         *
         * @return The undex of the selected tab.
         *         If no tab is selected -1 will be returned.
         */
        virtual int getSelectedTabIndex() const;
        
        /**
         * Gets the selected tab.
         *
         * @return The selected tab.
         */
        Tab* getSelectedTab();


        // Inherited from Widget

        virtual void draw(Graphics *graphics);

        virtual void drawBorder(Graphics* graphics);

        virtual void logic();

        void setWidth(int width);

        void setHeight(int height);

        void setSize(int width, int height);

        void setDimension(const Rectangle& dimension);

        
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
         * Adjusts the size of the tabbed area.
         */
        void adjustSize();
        void adjustTabPositions();
        
        Tab* mSelectedTab;
        Container* mTabContainer;
        Container* mWidgetContainer;
        std::vector<Tab*> mTabsToCleanUp;
        std::vector<std::pair<Tab*, Widget*> > mTabs;
        
    };
}

#endif // end GCN_TABBEDAREA_HPP
