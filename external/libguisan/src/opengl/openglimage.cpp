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

/*
 * For comments regarding functions please see the header file.
 */

#include "guisan/opengl/openglimage.hpp"

#include "guisan/exception.hpp"

namespace gcn
{
    OpenGLImage::OpenGLImage(unsigned int* pixels, int width, int height,
                             bool convertToDisplayFormat)
    {
        mAutoFree = true;

        mWidth = width;
        mHeight = height;
		mTextureWidth = 1, mTextureHeight = 1;

        while(mTextureWidth < mWidth)
        {
            mTextureWidth *= 2;
        }

        while(mTextureHeight < mHeight)
        {
            mTextureHeight *= 2;
        }

		// Create a new pixel array and copy the pixels into it
		mPixels = new unsigned int[mTextureWidth * mTextureHeight];

#ifdef __BIG_ENDIAN__
        const unsigned int magicPink = 0xff00ffff;
#else
        const unsigned int magicPink = 0xffff00ff;
#endif
		int x, y;
		for (y = 0; y < mTextureHeight; y++)
		{
			for (x = 0; x < mTextureWidth; x++)
			{
				if (x < mWidth && y < mHeight)
				{
					unsigned int c = pixels[x + y * mWidth];

					// Magic pink to transparent
					if (c == magicPink)
					{
						c = 0x00000000;
					}

					mPixels[x + y * mTextureWidth] = c;
				}
				else
				{
					mPixels[x + y * mTextureWidth] = 0x00000000;
				}
			}
		}

        if (convertToDisplayFormat)
        {
            OpenGLImage::convertToDisplayFormat();
        }
    }

    OpenGLImage::OpenGLImage(GLuint textureHandle, int width, int height, bool autoFree)
    {
        mTextureHandle = textureHandle;
        mAutoFree = autoFree;
		mPixels = NULL;

		mWidth = width;
        mHeight = height;
		mTextureWidth = 1, mTextureHeight = 1;

        while(mTextureWidth < mWidth)
        {
            mTextureWidth *= 2;
        }

        while(mTextureHeight < mHeight)
        {
            mTextureHeight *= 2;
        }
    }

    OpenGLImage::~OpenGLImage()
    {
        if (mAutoFree)
        {
            free();
        }
    }

    GLuint OpenGLImage::getTextureHandle() const
    {
        return mTextureHandle;
    }

    int OpenGLImage::getTextureWidth() const
    {
		return mTextureWidth;
    }

    int OpenGLImage::getTextureHeight() const
    {
		return mTextureHeight;
    }

    void OpenGLImage::free()
    {
		if (mPixels == NULL)
		{
			glDeleteTextures(1, &mTextureHandle);
		}
		else
		{
			delete[] mPixels;
			mPixels = NULL;
		}
    }

    int OpenGLImage::getWidth() const
    {
        return mWidth;
    }

    int OpenGLImage::getHeight() const
    {
        return mHeight;
    }

    Color OpenGLImage::getPixel(int x, int y)
    {
		if (mPixels == NULL)
		{
			throw GCN_EXCEPTION("Image has been converted to display format");
		}

		if (x < 0 || x >= mWidth || y < 0 || y >= mHeight)
		{
			throw GCN_EXCEPTION("Coordinates outside of the image");
		}

		unsigned int c = mPixels[x + y * mTextureWidth];

#ifdef __BIG_ENDIAN__
		unsigned char r = (c >> 24) & 0xff;
		unsigned char g = (c >> 16) & 0xff;
		unsigned char b = (c >> 8) & 0xff;
		unsigned char a = c & 0xff;
#else
		unsigned char a = (c >> 24) & 0xff;
		unsigned char b = (c >> 16) & 0xff;
		unsigned char g = (c >> 8) & 0xff;
		unsigned char r = c & 0xff;
#endif

        return Color(r, g, b, a);
    }

    void OpenGLImage::putPixel(int x, int y, const Color& color)
    {
        if (mPixels == NULL)
		{
			throw GCN_EXCEPTION("Image has been converted to display format");
		}

		if (x < 0 || x >= mWidth || y < 0 || y >= mHeight)
		{
			throw GCN_EXCEPTION("Coordinates outside of the image");
		}

#ifdef __BIG_ENDIAN__
		unsigned int c = color.a | color.b << 8 | color.g << 16 | color.r << 24;
#else
		unsigned int c = color.r | color.g << 8 | color.b << 16 | color.a << 24;
#endif

		mPixels[x + y * mTextureWidth] = c;
    }

    void OpenGLImage::convertToDisplayFormat()
    {
		if (mPixels == NULL)
		{
			throw GCN_EXCEPTION("Image has already been converted to display format");
		}

        glGenTextures(1, &mTextureHandle);
        glBindTexture(GL_TEXTURE_2D, mTextureHandle);

        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     4,
                     mTextureWidth,
                     mTextureHeight,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     mPixels);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        delete[] mPixels;
		mPixels = NULL;

        GLenum error = glGetError();
        if (error)
        {
            std::string errmsg;
            switch (error)
            {
              case GL_INVALID_ENUM:
                  errmsg = "GL_INVALID_ENUM";
                  break;

              case GL_INVALID_VALUE:
                  errmsg = "GL_INVALID_VALUE";
                  break;

              case GL_INVALID_OPERATION:
                  errmsg = "GL_INVALID_OPERATION";
                  break;

              case GL_STACK_OVERFLOW:
                  errmsg = "GL_STACK_OVERFLOW";
                  break;

              case GL_STACK_UNDERFLOW:
                  errmsg = "GL_STACK_UNDERFLOW";
                  break;

              case GL_OUT_OF_MEMORY:
                  errmsg = "GL_OUT_OF_MEMORY";
                  break;
            }

            throw GCN_EXCEPTION(std::string("Unable to convert to OpenGL display format, glGetError said: ") + errmsg);
        }
    }
}
