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

#ifndef GCN_IMAGE_HPP
#define GCN_IMAGE_HPP

#include <string>

#include "guisan/platform.hpp"

namespace gcn
{
    class Color;
    class ImageLoader;

    /**
     * Holds an image. To be able to use this class you must first set an
     * ImageLoader in Image by calling
     * @code Image::setImageLoader(myImageLoader) @endcode
     * The function is static. If this is not done, the constructor taking a
     * filename will throw an exception. The ImageLoader you use must be
     * compatible with the Graphics object you use.
     *
     * EXAMPLE: If you use SDLGraphics you should use SDLImageLoader.
     *          Otherwise your program might crash in a most bizarre way.
     * @see OpenGLSDLImageLoader, SDLImageLoader
     * @since 0.1.0
     */
    class GCN_CORE_DECLSPEC Image
    {
    public:

        /**
         * Constructor.
         */
        Image();

        /**
         * Destructor.
         */
        virtual ~Image();

        /**
         * Loads an image by using the class' image laoder. All image loaders implemented
         * in Guisan return a newly instantiated image which must be deleted in
         * order to avoid a memory leak.
         *
         * NOTE: The functions getPixel and putPixel are only guaranteed to work
         *       before an image has been converted to display format.
         *
         * @param filename The file to load.
         * @param convertToDisplayFormat True if the image should be converted
         *                               to display, false otherwise.
         * @since 0.5.0
         */
        static Image* load(const std::string& filename, bool convertToDisplayFormat = true);

        /**
         * Gets the image loader used for loading images.
         *
         * @return The image loader used for loading images.
         * @see setImageLoader, OpenGLSDLImageLoader, SDLImageLoader
         * @since 0.1.0
         */
        static ImageLoader* getImageLoader();

        /**
         * Sets the ImageLoader to be used for loading images.
         *
         * IMPORTANT: The image loader is static and MUST be set before 
         *            loading images!
         *
         * @param imageLoader The image loader to be used for loading images.
         * @see getImageLoader, OpenGLSDLImageLoader, SDLImageLoader
         * @since 0.1.0
         */
        static void setImageLoader(ImageLoader* imageLoader);

        /**
         * Frees an image.
         *
         * @since 0.5.0
         */
        virtual void free() = 0;

        /**
         * Gets the width of the image.
         *
         * @return The width of the image.
         *
         * @since 0.1.0
         */
        virtual int getWidth() const = 0;

        /**
         * Gets the height of the image.
         *
         * @return The height of the image.
         *
         * @since 0.1.0
         */
        virtual int getHeight() const = 0;

        /**
         * Gets the color of a pixel at coordinate (x, y) in the image.
         *
         * IMPORTANT: Only guaranteed to work before the image has been
         *            converted to display format.
         *
         * @param x The x coordinate.
         * @param y The y coordinate.
         * @return The color of the pixel.
         *
         * @since 0.5.0
         */
        virtual Color getPixel(int x, int y) = 0;

        /**
         * Puts a pixel with a certain color at coordinate (x, y).
         *
         * @param x The x coordinate.
         * @param y The y coordinate.
         * @param color The color of the pixel to put.
         *
         * @since 0.5.0
         */
        virtual void putPixel(int x, int y, const Color& color) = 0;

        /**
         * Converts the image, if possible, to display format.
         *
         * IMPORTANT: Only guaranteed to work before the image has been
         *            converted to display format.
         * @since 0.5.0
         */
        virtual void convertToDisplayFormat() = 0;

    protected:
        /**
         * Holds the image loader to be used when loading images.
         */
        static ImageLoader* mImageLoader;
    };
}

#endif // end GCN_IMAGE_HPP
