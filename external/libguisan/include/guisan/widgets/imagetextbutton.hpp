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

#ifndef GCN_IMAGETEXTBUTTON_HPP
#define GCN_IMAGETEXTBUTTON_HPP

#include <string>

#include "guisan/platform.hpp"
#include "guisan/widgets/imagebutton.hpp"

namespace gcn
{
    class Image;

    /**
     * A simple button that displays an image and a caption.
     */
    class GCN_CORE_DECLSPEC ImageTextButton : public gcn::ImageButton
    {
    public:
        /**
         * Constructor.
         *
         * @param filename The filename of the image to display.
         * @param caption The text to display. 
         */
        ImageTextButton(const std::string& filename, std::string& caption);

        /**
         * Constructor.
         *
         * @param image The image to display.
         * @param caption The text to display.
         */
        ImageTextButton(Image* image, std::string& caption);

        /**
         * Destructor.
         */
        virtual ~ImageTextButton();

        /**
         * Adjusts the size of the image button to fit the image.
         */
        void adjustSize();

        /**
         * Sets the image to display.
         *
         * @param image The image to display.
         */
        void setImage(Image* image);

        /**
         * Gets the image of the image button.
         *
         * @return The image of the image button.
         */
        Image* getImage();


        // Inherited from Widget

        void draw(gcn::Graphics* graphics);
        
        /**
         * Sets the alignment for the caption. The alignment is relative
         * to the center of the button.
         *
         * @param alignment ImageTextButton::TOP, ImageTextButton::RIGHT, ImageTextButton::BOTTOM or ImageTextButton::LEFT.
         */
        void setAlignment(unsigned int alignment);

        /**
         * Gets the alignment for the caption. The alignment is relative to
         * the center of the button.
         *
         * @return alignment of caption. ImageTextButton::TOP, ImageTextButton::RIGHT, ImageTextButton::BOTTOM or ImageTextButton::LEFT.
         */
        unsigned int getAlignment() const;
        
        enum
        {
            TOP,
            RIGHT,
            BOTTOM,
            LEFT
        };

    protected:
        gcn::Image* mImage;

        /**
         * True if the image has been loaded internally, false otherwise.
         * An image not loaded internally should not be deleted in the
         * destructor.
         */
        bool mInternalImage;
        unsigned int mAlignment;
    };
}
#endif
