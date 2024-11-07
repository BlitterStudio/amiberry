/*      _______   __   __   __   ______   __   __   _______   __   __
 *     / _____/\ / /\ / /\ / /\ / ____/\ / /\ / /\ / ___  /\ /  |\/ /\
 *    / /\____\// / // / // / // /\___\// /_// / // /\_/ / // , |/ / /
 *   / / /__   / / // / // / // / /    / ___  / // ___  / // /| ' / /
 *  / /_// /\ / /_// / // / // /_/_   / / // / // /\_/ / // / |  / /
 * /______/ //______/ //_/ //_____/\ /_/ //_/ //_/ //_/ //_/ /|_/ /
 * \______\/ \______\/ \_\/ \_____\/ \_\/ \_\/ \_\/ \_\/ \_\/ \_\/
 *
 * Copyright (c) 2004, 2005, 2006, 2007 Olof Naessén and Per Larsson
 * Copyright (c) 2017 Gwilherm Baudic
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

#ifndef GCN_MESSAGEBOX_HPP
#define GCN_MESSAGEBOX_HPP

#include "guisan/actionlistener.hpp"
#include "guisan/platform.hpp"
#include "guisan/widgets/button.hpp"
#include "guisan/widgets/label.hpp"
#include "guisan/widgets/window.hpp"

#include <memory>
#include <string>
#include <vector>

namespace gcn
{
    /**
     * A non-movable window to display a message with some buttons.
     */
    class GCN_CORE_DECLSPEC MessageBox : public Window, public ActionListener
    {
    public:
        /**
         * Constructor.
         * This version only has a single button labeled "OK".
         *
         * @param caption the MessageBox caption.
         * @param message the message to display in the MessageBox
         */
        MessageBox(const std::string& caption, const std::string& message);

        /**
         * Constructor.
         *
         * @param caption the MessageBox caption.
         * @param message the message to display in the MessageBox
         * @param button_captions strings to display as button captions
         * @param size length of the buttons array
         */
        MessageBox(const std::string& caption,
                   const std::string& message,
                   const std::string* button_captions,
                   int size);

        /**
         * Constructor.
         *
         * @param caption the MessageBox caption.
         * @param message the message to display in the MessageBox
         * @param button_captions strings to display as button captions
         */
        MessageBox(const std::string& caption,
                   const std::string& message,
                   const std::vector<std::string>& button_captions);

        /**
         * Destructor.
         */
        ~MessageBox() override;

        /**
         * Gets the index of the clicked button
         * 
         * @return index of clicked button, starting at 0. -1 if not set (i.e., no button clicked yet)
         */
        int getClickedButton() const;

        /**
         * Sets the position for the button(s) in the MessageBox.
         *
         * @param alignment The alignment of the button(s).
         */
        void setButtonAlignment(Graphics::Alignment alignment);

        /**
         * Gets the position for the button(s) in the MessageBox.
         *
         * @return alignment of buttons.
         */
        Graphics::Alignment getButtonAlignment() const;

        /**
         * Add this MessageBox to a parent container,
         * centered both horizontally and vertically.
         * If instead, you want to place it somewhere else, use Container::add().
         *
         * @param container parent container
         */
        void addToContainer(Container* container);

        // Inherited from ActionListener

        void action(const ActionEvent& keyEvent) override;

    protected:
        Graphics::Alignment mButtonAlignment = Graphics::Alignment::Left;
        int mClickedButton = -1;

        std::vector<std::unique_ptr<Button>> mButtons;
        std::unique_ptr<Label> mLabel;
    };
} // namespace gcn

#endif // end GCN_MESSAGEBOX_HPP
