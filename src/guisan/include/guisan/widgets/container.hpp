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

#ifndef GCN_CONTAINER_HPP
#define GCN_CONTAINER_HPP

#include <list>

#include "guisan/basiccontainer.hpp"
#include "guisan/graphics.hpp"
#include "guisan/platform.hpp"

namespace gcn
{
    /**
     * An implementation of a container able to contain other widgets. A widget's 
     * position in the container is relative to the container itself and not the screen. 
     * A container is the most common widget to use as the Gui's top widget as makes the Gui
     * able to contain more than one widget.
     *
     * @see Gui::setTop
     */
    class GCN_CORE_DECLSPEC Container: public BasicContainer
    {
    public:

        /**
         * Constructor. A container is opauqe as default, if you want a
         * none opaque container call setQpaque(false).
         *
         * @see setOpaque, isOpaque
         */
        Container();

        /**
         * Destructor.
         */
        virtual ~Container();

        /**
         * Sets the container to be opaque or not. If the container
         * is opaque it's background will be drawn, if it's not opaque 
         * it's background will not be drawn, and thus making the container
         * completely transparent.
         *
         * NOTE: This is not the same as to set visibility. A non visible
         *       container will not itself nor will it draw it's content.
         *
         * @param opaque True if the container should be opaque, false otherwise.
         * @see isOpaque
         */
        void setOpaque(bool opaque);

        /**
         * Checks if the container is opaque or not.
         *
         * @return true if the container is opaque, false otherwise.
         * @see setOpaque
         */
        bool isOpaque() const;

        /**
         * Adds a widget to the container.
         *
         * @param widget The widget to add.
         * @see remove, clear
         */
        virtual void add(Widget* widget);

        /**
         * Adds a widget to the container and also specifices the widget's
         * postion in the container. The position is relative to the container
         * and not relative to the screen.
         *
         * @param widget The widget to add.
         * @param x The x coordinat for the widget.
         * @param y The y coordinat for the widget.
         * @see remove, clear
         */
        virtual void add(Widget* widget, int x, int y);

        /**
         * Removes a widget from the Container.
         *
         * @param widget The widget to remove.
         * @throws Exception when the widget has not been added to the
         *                   container.
         * @see add, clear
         */
        virtual void remove(Widget* widget);

        /**
         * Clears the container of all widgets.
         *
         * @see add, remove
         */
        virtual void clear();

        /**
         * Finds a widget given an id.
         *
         * @param id The id to find a widget by.
         * @return A widget with a corrosponding id, NULL if no widget 
         *         is found.
         * @see Widget::setId
         */
        virtual Widget* findWidgetById(const std::string &id);


        // Inherited from Widget

        virtual void draw(Graphics* graphics);

        virtual void drawBorder(Graphics* graphics);

    protected:
        /**
         * True if the container is opaque, false otherwise.
         */
        bool mOpaque;
    };
}

#endif // end GCN_CONTAINER_HPP
