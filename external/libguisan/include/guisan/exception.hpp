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

#ifndef GCN_EXCEPTION_HPP
#define GCN_EXCEPTION_HPP

#include <string>

#include "guisan/platform.hpp"

# ifndef __FUNCTION__
#  define __FUNCTION__ "?"
# endif

/*
 * A macro used to create a standard exception object.
 * What it basicly does is that it creates a new exception
 * and automatically sets the filename and line number where
 * the exception occured by using other compiler macros.
 */
#define GCN_EXCEPTION(mess) gcn::Exception(mess,   \
                            __FUNCTION__,          \
                            __FILE__,              \
                            __LINE__)

namespace gcn
{
    /**
     * An exception containing a message, a file and a line number
     * where the exception occured. Guisan will only throw exceptions
     * of this class.
     *
     * You can use this class for your own exceptions that has
     * something to do with a GUI exception. A nifty feature of the
     * excpetion class is that it can tell you from which line and
     * file it was thrown. To make things easier when throwing
     * exceptions there exists a macro for creating exceptions
     * which automatically sets the filename and line number.
     *
     * EXAMPLE: @code
     *          throw GCN_EXCEPTION("my error message");
     *          @endcode
     */
    class GCN_CORE_DECLSPEC Exception
    {
    public:

        /**
         * Constructor.
         */
        Exception();

        /**
         * Constructor.
         *
         * @param message The error message of the exception.
         */
        Exception(std::string message);

        /**
         * Constructor.
         *
         * NOTE: Don't use this constructor. Use the GCN_EXCEPTION macro instead.
         *       This constructor merely exists for the GCN_EXCEPTION macro to
         *       use.
         *
         * @param message The error message of the exception.
         * @param function The function name where the exception occured.
         * @param filename The name of the file where the exception occured.
         * @param line The line number in the source code where the exception
         *             occured.
         */
        Exception(std::string message,
                  std::string function,
                  std::string filename,
                  unsigned int line);

        /**
         * Gets the function name where the exception occured.
         *
         * @return The function name where the exception occured.
         */
        const std::string& getFunction() const;

        /**
         * Gets the error message of the exception.
         *
         * @return The error message of the exception.
         */
        const std::string& getMessage() const;

        /**
         * Gets the filename where the exception occured.
         *
         * @return The filename where the exception occured.
         */
        const std::string& getFilename() const;

        /**
         * Gets the line number where the exception occured.
         *
         * @return The line number where the exception occured.
         */
        unsigned int getLine() const;

    protected:
        /**
         * Holds the name of the function name where the
         * exception occured.
         */
        std::string mFunction;

        /**
         * Holds the error message of the exception.
         */
        std::string mMessage;

        /**
         * Holds the filename where the exception occured.
         */
        std::string mFilename;

        /**
         * Holds the line number where the exception occured.
         */
        unsigned int mLine;
    };
}

#endif // end GCN_EXCEPTION_HPP

/*
 * "Final Fantasy XI is the BEST!... It's even better then water!"
 *  - Astrolite
 * I believe it's WoW now days.
 */
