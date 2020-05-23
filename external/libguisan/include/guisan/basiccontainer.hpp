/*      _______   __   __   __   ______   __   __   _______   __   __
 *     / _____/\ / /\ / /\ / /\ / ____/\ / /\ / /\ / ___  /\ /  |\/ /\
 *    / /\____\// / // / // / // /\___\// /_// / // /\_/ / // , |/ / /
 *   / / /__   / / // / // / // / /    / ___  / // ___  / // /| ' / /
 *  / /_// /\ / /_// / // / // /_/_   / / // / // /\_/ / // / |  / /
 * /______/ //______/ //_/ //_____/\ /_/ //_/ //_/ //_/ //_/ /|_/ /
 * \______\/ \______\/ \_\/ \_____\/ \_\/ \_\/ \_\/ \_\/ \_\/ \_\/
 *
 * Copyright (c) 2004, 2005, 2006, 2007 Olof Naess�n and Per Larsson
 *
 *                                                         Js_./
 * Per Larsson a.k.a finalman                          _RqZ{a<^_aa
 * Olof Naess�n a.k.a jansem/yakslem                _asww7!uY`>  )\a//
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

#ifndef GCN_BASICCONTAINER_HPP
#define GCN_BASICCONTAINER_HPP

#include <list>

#include "guisan/deathlistener.hpp"
#include "guisan/platform.hpp"
#include "guisan/widget.hpp"

namespace gcn
{
	/**
	 * Implements basic container behaviour. Most container will suffice by
	 * inheriting from this class.
	 *
	 * @see Container
	 */
	class GCN_CORE_DECLSPEC BasicContainer : public Widget, public DeathListener
	{
	public:
		/**
		 * Destructor
		 */
		virtual ~BasicContainer();


		// Inherited from Widget

		void moveToTop(Widget* widget) override;

		void moveToBottom(Widget* widget) override;

		Rectangle getChildrenArea() override;

		void focusNext() override;

		void focusPrevious() override;

		void logic() override;

		void _setFocusHandler(FocusHandler* focusHandler) override;

		void setInternalFocusHandler(FocusHandler* focusHandler) override;

		void showWidgetPart(Widget* widget, Rectangle area) override;

		Widget* getWidgetAt(int x, int y) override;


		// Inherited from DeathListener

		void death(const Event& event) override;

	protected:
		/**
		 * Adds a widget to the basic container.
		 *
		 * @param widget the widget to add.
		 */
		virtual void add(Widget* widget);

		/**
		 * Removes a widget from the basic container.
		 *
		 * @param widget the widget to remove.
		 */
		virtual void remove(Widget* widget);

		/**
		 * Clears the basic container from all widgets.
		 */
		virtual void clear();

		/**
		 * Draws children widgets.
		 *
		 * @param graphics a Graphics object to draw with.
		 */
		virtual void drawChildren(Graphics* graphics);

		/**
		 * Calls logic for children widgets.
		 */
		virtual void logicChildren();

		/**
		 * Finds a widget given an id.
		 *
		 * @param id the id to find a widget by.
		 * @return the widget with the corresponding id, 
				   NULL of no widget is found.
		 */
		virtual Widget* findWidgetById(const std::string& id);

		typedef std::list<Widget*> WidgetList;
		typedef WidgetList::iterator WidgetListIterator;
		typedef WidgetList::reverse_iterator WidgetListReverseIterator;

		WidgetList mWidgets;
	};
}

#endif // end GCN_BASICCONTAINER_HPP
