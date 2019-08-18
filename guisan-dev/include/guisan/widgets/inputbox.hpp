/*      _______   __   __   __   ______   __   __   _______   __   __
 *     / _____/\ / /\ / /\ / /\ / ____/\ / /\ / /\ / ___  /\ /  |\/ /\
 *    / /\____\// / // / // / // /\___\// /_// / // /\_/ / // , |/ / /
 *   / / /__   / / // / // / // / /    / ___  / // ___  / // /| ' / /
 *  / /_// /\ / /_// / // / // /_/_   / / // / // /\_/ / // / |  / /
 * /______/ //______/ //_/ //_____/\ /_/ //_/ //_/ //_/ //_/ /|_/ /
 * \______\/ \______\/ \_\/ \_____\/ \_\/ \_\/ \_\/ \_\/ \_\/ \_\/
 *
 * Copyright (c) 2004, 2005, 2006, 2007 Olof Naessén and Per Larsson
 * Copyright (c) 2017, 2018, 2019 Gwilherm Baudic
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

#ifndef GCN_INPUTBOX_HPP
#define GCN_INPUTBOX_HPP

#include <string>

#include "guisan/mouselistener.hpp"
#include "guisan/actionlistener.hpp"
#include "guisan/platform.hpp"
#include "guisan/widgets/window.hpp"
#include "guisan/widgets/button.hpp"
#include "guisan/widgets/label.hpp"
#include "guisan/widgets/textfield.hpp"
#include "guisan/widgets/icon.hpp"

namespace gcn
{
    /**
     * A non-movable window to get a short string from the user.
     */
    class GCN_CORE_DECLSPEC InputBox : public Window
    {
    public:

        /**
         * Constructor. 
         *
         * @param caption the InputBox caption.
         * @param message the message to display in the InputBox
         * @param ok the string corresponding to the "OK" button
         * @param cancel the string corresponding to the "Cancel" button
         */
        InputBox(const std::string& caption, const std::string& message, const std::string &ok = "OK", const std::string &cancel = "Cancel");

        /**
         * Destructor.
         */
        virtual ~InputBox();
        
        /**
         * Add this InputBox to a parent container, centered both horizontally and vertically
         * If instead, you want to place it somewhere else, use Container::add(). 
         *
         * @param container parent container
         */
        void addToContainer(Container* container);
        
        /**
         * Get the text that was input by the user
         * Use in conjunction with getClickedButton() to tell an empty string from a cancel operation.
         * 
         * @return the text which was typed by the user
         */
        std::string getText() const;
        
        /**
         * Get the number of the button that was clicked
         * @return 0 for OK, 1 for Cancel
         */
        int getClickedButton() const;


        // Inherited from Widget

        virtual void draw(Graphics* graphics);

        virtual void drawBorder(Graphics* graphics);


        // Inherited from MouseListener

        virtual void mousePressed(MouseEvent& mouseEvent);

        virtual void mouseDragged(MouseEvent& mouseEvent);

        virtual void mouseReleased(MouseEvent& mouseEvent);

    protected:
        std::string mMessage;
        int mClickedButton;
        
        Button *mButtonOK;
        Button *mButtonCancel;
        Label *mLabel;
        TextField *mText;
    };
}

#endif // end GCN_INPUTBOX_HPP
