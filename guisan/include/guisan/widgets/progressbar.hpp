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

#ifndef GCN_PROGRESSBAR_HPP
#define GCN_PROGRESSBAR_HPP

#include <string>

#include "guisan/platform.hpp"
#include "guisan/widget.hpp"
#include "guisan/widgets/label.hpp"

namespace gcn
{
    /**
     * Implementation of a label capable of displaying a caption and a progress bar.
     * 
     * Setting both start and end to 0 creates an "infinite" progressbar with a small rectangle
     * moving forward and disappearing. In this mode, it is up to the caller to set the progressbar
     * value in the range 0-100 (which indicates the start of the rectangle) 
     * regularly to achieve animation. 
     */
    class GCN_CORE_DECLSPEC ProgressBar: public Label
    {
    public:
        /**
         * Constructor.
         */
        ProgressBar();
        
        /**
         * Constructor.
         *
         * @param start minimum value
         * @param end maximum value
         * @param value current value
         */
        ProgressBar(const unsigned int start, const unsigned int end, const unsigned int value);

        /**
         * Constructor.
         *
         * @param caption The caption of the label.
         */
        ProgressBar(const std::string& caption);

        /**
         * Gets the caption of the label.
         *
         * @return The caption of the label.
         * @see setCaption
         */
        const std::string &getCaption() const;

        /**
         * Sets the caption of the label. 
         *
         * @param caption The caption of the label.
         * @see getCaption, adjustSize
         */
        void setCaption(const std::string& caption);

        /**
         * Sets the alignment for the caption. The alignment is relative
         * to the center of the label.
         *
         * @param alignemnt Graphics::LEFT, Graphics::CENTER or Graphics::RIGHT.
         * @see getAlignment, Graphics
         */
        void setAlignment(unsigned int alignment);

        /**
         * Gets the alignment for the caption. The alignment is relative to
         * the center of the label.
         *
         * @return alignment of caption. Graphics::LEFT, Graphics::CENTER or Graphics::RIGHT.
         * @see setAlignment, Graphics
         */
        unsigned int getAlignment() const;
        
        /**
         * Sets the minimum value.
         *
         * @param start the minimum value
         * @see getStart
         */
        void setStart(const unsigned int start);
        
        /**
         * Gets the minimum value.
         *
         * @return the minimum value
         * @see setStart
         */
        unsigned int getStart() const;
        
        /**
         * Sets the maximum value.
         *
         * @param end the maximum value
         * @see getEnd
         */
        void setEnd(const unsigned int end);
        
        /**
         * Gets the maximum value.
         *
         * @return the maximum value
         * @see setEnd
         */
        unsigned int getEnd() const;
        
        /**
         * Sets the current progress value.
         *
         * @param value the current value
         * @see getStart
         */
        void setValue(const unsigned int value);
        
        /**
         * Gets the current progress value.
         * 
         * @return progress value
         * @see setValue
         */
        unsigned int getValue() const;
        
        /**
         * Adjusts the size of the widget. 
         */
        void adjustSize();


        // Inherited from Widget

        virtual void draw(Graphics* graphics);

        virtual void drawBorder(Graphics* graphics);

    protected:
        /**
         * Holds the caption of the label.
         */
        std::string mCaption;

        /**
         * Holds the alignment of the caption.
         */
        unsigned int mAlignment;
        
        unsigned int mStart; //! minimum value of the progressbar
        unsigned int mEnd;   //! maximum value of the progressbar
        unsigned int mValue; //! current value of the progressbar
    };
}

#endif // end GCN_PROGRESSBAR_HPP
