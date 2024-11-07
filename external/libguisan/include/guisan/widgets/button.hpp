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

#ifndef GCN_BUTTON_HPP
#define GCN_BUTTON_HPP

#include <string>

#include "guisan/focuslistener.hpp"
#include "guisan/keylistener.hpp"
#include "guisan/graphics.hpp"
#include "guisan/mouseevent.hpp"
#include "guisan/mouselistener.hpp"
#include "guisan/platform.hpp"
#include "guisan/widget.hpp"

namespace gcn
{
    /**
     * A regular button. Add an ActionListener to it to know when it
     * has been clicked.
     *
     * NOTE: You can only have text (a caption) on the button. If you want it
     *       to handle, for instance images, you can implement an ImageButton
     *       of your own and overload member functions from Button.
     */
    class GCN_CORE_DECLSPEC Button : public Widget,
                                     public MouseListener,
                                     public KeyListener,
                                     public FocusListener
    {
    public:
        /**
         * Constructor.
         */
        Button();

        /**
         * Constructor.
         *
         * @param caption the caption of the Button.
         */
        Button(std::string caption);

        /**
         * Sets the Button caption.
         *
         * @param caption the Button caption.
         */
        void setCaption(const std::string& caption);

        /**
         * Gets the Button caption.
         *
         * @return the Button caption.
         */
        const std::string& getCaption() const;

        /**
         * Sets the alignment of the caption.
         *
         * @param alignment The alignment of the caption.
         * @see Graphics
         */
        void setAlignment(Graphics::Alignment alignment);

        /**
         * Gets the alignment of the caption.
         *
         * @return alignment of caption.
         */
        Graphics::Alignment getAlignment() const;

        /**
         * Sets the spacing between the border of the button and its caption.
         *
         * @param spacing is a number between 0 and 255. The default value for 
                          spacing is 4 and can be changed using this method.
         */
        void setSpacing(unsigned int spacing);

        /**
         * Gets the spacing between the border of the button and its caption.
         *
         * @return spacing.
         */
        unsigned int getSpacing() const;

        /**
         * Adjusts the buttons size to fit the content.
         */
        void adjustSize();

        //Inherited from Widget

        void draw(Graphics* graphics) override;
        void hotKeyPressed() override;
        void hotKeyReleased() override;

        // Inherited from FocusListener

        void focusLost(const Event& event) override;

        // Inherited from MouseListener

        void mousePressed(MouseEvent& mouseEvent) override;
        void mouseReleased(MouseEvent& mouseEvent) override;
        void mouseClicked(MouseEvent& mouseEvent) override;
        void mouseEntered(MouseEvent& mouseEvent) override;
        void mouseExited(MouseEvent& mouseEvent) override;
        void mouseDragged(MouseEvent& mouseEvent) override;

        // Inherited from KeyListener

        void keyPressed(KeyEvent& keyEvent) override;
        void keyReleased(KeyEvent& keyEvent) override;

    protected:
        /**
         * Checks if the button is pressed. Convenient method to use
         * when overloading the draw method of the button.
         *
         * @return True if the button is pressed, false otherwise.
         */
        bool isPressed() const;

        /**
         * Holds the caption of the button.
         */
        std::string mCaption;

        /**
         * True if the mouse is ontop of the button, false otherwise.
         */
        bool mHasMouse = false;

        /**
         * True if a key has been pressed, false otherwise.
         */
        bool mKeyPressed = false;

        /**
         * True if a mouse has been pressed, false otherwise.
         */
        bool mMousePressed = false;

        bool mHotKeyPressed = false;

        /**
         * Holds the alignment of the caption.
         */
        Graphics::Alignment mAlignment = Graphics::Alignment::Center;

        /**
         * Holds the spacing between the border and the caption.
         */
        unsigned int mSpacing = 4;
    };
}

#endif // end GCN_BUTTON_HPP
