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
     * A slider able to slide between different values. You can set the scale
     * of the slider yourself so that it ranges between, for example, -1.0 and
     * 2.0.
     */
    class GCN_CORE_DECLSPEC Slider :
        public Widget,
        public MouseListener,
        public KeyListener
    {
    public:

        /**
         * Constructor. Scale start is 0.
         *
         * @param scaleEnd the end of the slider scale.
         */
        Slider(double scaleEnd = 1.0);

        /**
         * Constructor.
         *
         * @param scaleStart the start of the scale.
         * @param scaleEnd the end of the scale.
         */
        Slider(double scaleStart, double scaleEnd);

        /**
         * Destructor.
         */
        virtual ~Slider() { }

        /**
         * Sets the scale.
         *
         * @param scaleStart the start of the scale.
         * @param scaleEnd the end of the scale.
         */
        void setScale(double scaleStart, double scaleEnd);

        /**
         * Gets the scale start.
         *
         * @return the scale start.
         */
        double getScaleStart() const;

        /**
         * Sets the scale start.
         *
         * @param scaleStart the start of the scale.
         */
        void setScaleStart(double scaleStart);

        /**
         * Gets the scale end.
         *
         * @return the scale end.
         */
        double getScaleEnd() const;

        /**
         * Sets the scale end.
         *
         * @param scaleEnd the end of the scale.
         */
        void setScaleEnd(double scaleEnd);

        /**
         * Gets the current value.
         *
         * @return the current value.
         */
        double getValue() const;

        /**
         * Sets the current value.
         *
         * @param value a scale value.
         */
        void setValue(double value);

        /**
         * Draws the marker.
         *
         * @param graphics a graphics object to draw with.
         */
        virtual void drawMarker(gcn::Graphics* graphics);

        /**
         * Sets the length of the marker.
         *
         * @param length new length for the marker.
         */
        void setMarkerLength(int length);

        /**
         * Gets the length of the marker.
         *
         * @return the length of the marker.
         */
        int getMarkerLength() const;

        /**
         * Sets the orientation of the slider. A slider can be drawn verticaly
         * or horizontaly. For orientation, see the enum in this class.
         *
         * @param orientation the orientation.
         */
        void setOrientation(unsigned int orientation);

        /**
         * Gets the orientation of the slider. Se the enum in this class.
         *
         * @return the orientation of the slider.
         */
        unsigned int getOrientation() const;

        /**
         * Sets the step length. Step length is used when the keys left and
         * right are pressed.
         *
         * @param length the step length.
         */
        void setStepLength(double length);

        /**
         * Gets the step length.
         *
         * @return the step length.
         */
        double getStepLength() const;


        // Inherited from Widget

        virtual void draw(gcn::Graphics* graphics);

        virtual void drawBorder(gcn::Graphics* graphics);


        // Inherited from MouseListener.

        virtual void mousePressed(MouseEvent& mouseEvent);

        virtual void mouseDragged(MouseEvent& mouseEvent);

        virtual void mouseWheelMovedUp(MouseEvent& mouseEvent);

        virtual void mouseWheelMovedDown(MouseEvent& mouseEvent);


        // Inherited from KeyListener

        virtual void keyPressed(KeyEvent& keyEvent);

        /**
         * Draw orientations for the slider. It can be drawn verticaly or
         * horizontaly.
         */
        enum
        {
            HORIZONTAL = 0,
            VERTICAL
        };

    protected:
        /**
         * Converts a marker position to a value.
         *
         * @param v the position to convert.
         * @return the value corresponding to the position.
         */
        virtual double markerPositionToValue(int v) const;

        /**
         * Converts a value to a marker position.
         *
         * @param value the value to convert.
         * @return the position corresponding to the value.
         */
        virtual int valueToMarkerPosition(double value) const;

        /**
         * Gets the marker position for the current value.
         *
         * @return the marker position for the current value.
         */
        virtual int getMarkerPosition() const;

        bool mMouseDrag;
        double mValue;
        double mStepLength;
        int mMarkerLength;
        double mScaleStart;
        double mScaleEnd;
        unsigned int mOrientation;
    };
}

#endif // end GCN_SLIDER_HPP
