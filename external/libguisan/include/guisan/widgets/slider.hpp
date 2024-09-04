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

#ifndef GCN_SLIDER_HPP
#define GCN_SLIDER_HPP

#include "guisan/keylistener.hpp"
#include "guisan/mouselistener.hpp"
#include "guisan/platform.hpp"
#include "guisan/widget.hpp"

namespace gcn
{
    /**
     * An implementation of a slider where a user can select different values by
     * sliding between a start value and an end value of a scale.
     *
     * If the selected value is changed an action event will be sent to all
     * action listeners of the slider.
     */
    class GCN_CORE_DECLSPEC Slider :
        public Widget,
        public MouseListener,
        public KeyListener
    {
    public:
        /**
         * Draw orientations for the slider. A slider can be drawn vertically or
         * horizontally.
         */
        enum Orientation
        {
            Horizontal = 0,
            Vertical
        };

        /**
         * Constructor. The default start value of the slider scale is zero.
         *
         * @param scaleEnd The end value of the slider scale.
         */
        Slider(double scaleEnd = 1.0);

        /**
         * Constructor.
         *
         * @param scaleStart The start value of the slider scale.
         * @param scaleEnd The end value of the slider scale.
         */
        Slider(double scaleStart, double scaleEnd);

        /**
         * Destructor.
         */
        virtual ~Slider() { }

        /**
         * Sets the scale of the slider.
         *
         * @param scaleStart The start value of the scale.
         * @param scaleEnd tThe end of value the scale.
         * @see getScaleStart, getScaleEnd
         */
        void setScale(double scaleStart, double scaleEnd);

        /**
         * Gets the start value of the scale.
         *
         * @return The start value of the scale.
         * @see setScaleStart, setScale
         */
        double getScaleStart() const;

        /**
         * Sets the start value of the scale.
         *
         * @param scaleStart The start value of the scale.
         * @see getScaleStart
         */
        void setScaleStart(double scaleStart);

        /**
         * Gets the end value of the scale.
         *
         * @return The end value of the scale.
         * @see setScaleEnd, setScale
         */
        double getScaleEnd() const;

        /**
         * Sets the end value of the scale.
         *
         * @param scaleEnd The end value of the scale.
         * @see getScaleEnd
         */
        void setScaleEnd(double scaleEnd);

        /**
         * Gets the current selected value.
         *
         * @return The current selected value.
         * @see setValue
         */
        double getValue() const;

        /**
         * Sets the current selected value.
         *
         * @param value The current selected value.
         * @see getValue
         */
        void setValue(double value);

        /**
         * Sets the length of the marker.
         *
         * @param length The length for the marker.
         * @see getMarkerLength
         */
        void setMarkerLength(int length);

        /**
         * Gets the length of the marker.
         *
         * @return The length of the marker.
         * @see setMarkerLength
         */
        int getMarkerLength() const;

        /**
         * Sets the orientation of the slider. A slider can be drawn vertically
         * or horizontally.
         *
         * @param orientation The orientation of the slider.
         * @see getOrientation
         */
        void setOrientation(Orientation orientation);

        /**
         * Gets the orientation of the slider. A slider can be drawn vertically
         * or horizontally.
         *
         * @return The orientation of the slider.
         * @see setOrientation
         */
        Orientation getOrientation() const;

        /**
         * Sets the step length. The step length is used when the keys LEFT 
         * and RIGHT are pressed to step in the scale.
         *
         * @param length The step length.
         * @see getStepLength
         */
        void setStepLength(double length);

        /**
         * Gets the step length. The step length is used when the keys LEFT 
         * and RIGHT are pressed to step in the scale.
         *
         * @return the step length.
         * @see setStepLength
         */
        double getStepLength() const;


        // Inherited from Widget

        virtual void draw(Graphics* graphics);


        // Inherited from MouseListener.

        virtual void mousePressed(MouseEvent& mouseEvent);

        virtual void mouseDragged(MouseEvent& mouseEvent);

        virtual void mouseWheelMovedUp(MouseEvent& mouseEvent);

        virtual void mouseWheelMovedDown(MouseEvent& mouseEvent);


        // Inherited from KeyListener

        virtual void keyPressed(KeyEvent& keyEvent);

    protected:
        /**
         * Draws the marker.
         *
         * @param graphics A graphics object to draw with.
         */
        virtual void drawMarker(Graphics* graphics);

        /**
         * Converts a marker position to a value in the scale.
         *
         * @param position The position to convert.
         * @return A scale value corresponding to the position.
         * @see valueToMarkerPosition
         */
        virtual double markerPositionToValue(int position) const;

        /**
         * Converts a value to a marker position.
         *
         * @param value The value to convert.
         * @return A marker position corresponding to the value.
         * @see markerPositionToValue
         */
        virtual int valueToMarkerPosition(double value) const;

        /**
         * Gets the marker position of the current selected value.
         *
         * @return The marker position of the current selected value.
         */
        virtual int getMarkerPosition() const;

        /**
         * True if the slider is dragged, false otherwise.
         */
        bool mDragged;

        /**
         * Holds the current selected value.
         */
        double mValue;

        /**
         * Holds the step length. The step length is used when the keys LEFT 
         * and RIGHT are pressed to step in the scale.
         */
        double mStepLength;

        /**
         * Holds the length of the marker.
         */
        int mMarkerLength;

        /**
         * Holds the start value of the scale.
         */
        double mScaleStart;

        /**
         * Holds the end value of the scale.
         */
        double mScaleEnd;

        /**
         * Holds the orientation of the slider. A slider can be drawn 
         * vertically or horizontally.
         */
        Orientation mOrientation;
    };
}

#endif // end GCN_SLIDER_HPP
