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

#ifndef GCN_WINDOW_HPP
#define GCN_WINDOW_HPP

#include <string>

#include "guisan/mouselistener.hpp"
#include "guisan/platform.hpp"
#include "guisan/widgets/container.hpp"

namespace gcn
{
    /**
     * An implementation of a movable window that can contain other widgets.
     */
    class GCN_CORE_DECLSPEC Window : public Container,
                                     public MouseListener
    {
    public:
        /**
         * Constructor.
         */
        Window();

        /**
         * Constructor. The window will be automatically resized in height
         * to fit the caption.
         *
         * @param caption the caption of the window.
         */
        Window(const std::string& caption);

        /**
         * Destructor.
         */
        virtual ~Window();

        /**
         * Sets the caption of the window.
         *
         * @param caption The caption of the window.
         * @see getCaption
         */
        void setCaption(const std::string& caption);

        /**
         * Gets the caption of the window.
         *
         * @return the caption of the window.
         * @see setCaption
         */
        const std::string& getCaption() const;

        /**
         * Sets the alignment of the caption.
         *
         * @param alignment The alignment of the caption.
         * @see getAlignment, Graphics
         */
        void setAlignment(Graphics::Alignment alignment);

        /**
         * Gets the alignment of the caption.
         *
         * @return The alignment of caption.
         * @see setAlignment, Graphics
         */
        Graphics::Alignment getAlignment() const;

        /**
         * Sets the padding of the window. The padding is the distance between the
         * window border and the content.
         *
         * @param padding The padding of the window.
         * @see getPadding
         */
        void setPadding(unsigned int padding);

        /**
         * Gets the padding of the window. The padding is the distance between the
         * window border and the content.
         *
         * @return The padding of the window.
         * @see setPadding
         */
        unsigned int getPadding() const;

        /**
         * Sets the title bar height.
         *
         * @param height The title height value.
         * @see getTitleBarHeight
         */
        void setTitleBarHeight(unsigned int height);

        /**
         * Gets the title bar height.
         *
         * @return The title bar height.
         * @see setTitleBarHeight
         */
        unsigned int getTitleBarHeight();

        /**
         * Sets the window to be moveble or not.
         *
         * @param movable True if the window should be movable, false otherwise.
         * @see isMovable
         */
        void setMovable(bool movable);

        /**
         * Checks if the window is movable.
         *
         * @return True if the window is movable, false otherwise.
         * @see setMovable
         */
        bool isMovable() const;

        /**
         * Sets the window to be opaque or not. An opaque window will draw its background
         * and its content. A non opaque window will only draw its content.
         *
         * @param opaque True if the window should be opaque, false otherwise.
         * @see isOpaque
         */
        void setOpaque(bool opaque);

        /**
         * Checks if the window is opaque.
         *
         * @return True if the window is opaque, false otherwise.
         * @see setOpaque
         */
        bool isOpaque();

        /**
         * Resizes the window to fit the content.
         */
        virtual void resizeToContent();


        // Inherited from BasicContainer

        virtual Rectangle getChildrenArea();


        // Inherited from Widget

        virtual void draw(Graphics* graphics);

		virtual void drawFrame(Graphics* graphics);

        // Inherited from MouseListener

        virtual void mousePressed(MouseEvent& mouseEvent);

        virtual void mouseDragged(MouseEvent& mouseEvent);

        virtual void mouseReleased(MouseEvent& mouseEvent);

    protected:
        /**
         * Holds the caption of the window.
         */
        std::string mCaption;

        /**
         * Holds the alignment of the caption.
         */
        Graphics::Alignment mAlignment;

        /**
         * Holds the padding of the window.
         */
        unsigned int mPadding;

        /**
         * Holds the title bar height of the window.
         */
        unsigned int mTitleBarHeight;

        /**
         * True if the window is movable, false otherwise.
         */
        bool mMovable;

        /**
         * True if the window is opaque, false otherwise.
         */
        bool mOpaque;

        /**
         * Holds a drag offset as an x coordinate where the drag of the window
         * started if the window is being dragged. It's used to move the window 
         * correctly when dragged.
         */
        int mDragOffsetX;

        /**
         * Holds a drag offset as an y coordinate where the drag of the window
         * started if the window is being dragged. It's used to move the window 
         * correctly when dragged.
         */
        int mDragOffsetY;

        /**
         * True if the window is being moved, false otherwise.
         */
        bool mMoved;
    };
}

#endif // end GCN_WINDOW_HPP
