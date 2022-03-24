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

#include "guisan/key.hpp"

namespace gcn
{
	Key::Key(int value)
		: mValue(value)
	{
	}

	bool Key::isCharacter() const
	{
		return (mValue >= 32 && mValue <= 126)
			|| (mValue >= 162 && mValue <= 255)
			|| (mValue == 9);
	}

	bool Key::isNumber() const
	{
		return mValue >= 48 && mValue <= 57;
	}

	bool Key::isLetter() const
	{
		return (((mValue >= 65 && mValue <= 90)
				|| (mValue >= 97 && mValue <= 122)
				|| (mValue >= 192 && mValue <= 255))
			&& (mValue != 215) && (mValue != 247));
	}

	bool Key::isSymbol() const
	{
		// ,-./
		// ;'
		// [\]
		// = `
		return (mValue >= 44 && mValue <= 47)
		|| mValue == 59 || mValue == 39
		|| (mValue >= 91 && mValue <= 93)
		|| mValue == 61 || mValue == 96;
	}

	int Key::getValue() const
	{
		return mValue;
	}

	char Key::getChar() const
	{
		if (mValue == 9 || mValue == 13 || (mValue <= 122 && mValue >= 32))
			return static_cast<char>(mValue);

		return '\0';
	}

	char Key::getShiftedNumeric() const
	{
		// 1 -> !
		if (mValue == 49) return static_cast<char>(33);
		// 2 -> @
		if (mValue == 50) return static_cast<char>(64);
		// 3 -> #
		if (mValue == 51) return static_cast<char>(35);
		// 4 -> $
		if (mValue == 52) return static_cast<char>(36);
		// 5 -> %
		if (mValue == 53) return static_cast<char>(37);
		// 6 -> ^
		if (mValue == 54) return static_cast<char>(94);
		// 7 -> &
		if (mValue == 55) return static_cast<char>(38);
		// 8 -> *
		if (mValue == 56) return static_cast<char>(42);
		// 9 -> (
		if (mValue == 57) return static_cast<char>(40);
		// 0 -> )
		if (mValue == 48) return static_cast<char>(41);

		return '\0';
	}

	char Key::getShiftedSymbol() const
	{
		// , -> <
		if (mValue == 44) return static_cast<char>(60);
		// - -> _
		if (mValue == 45) return static_cast<char>(95);
		// . -> >
		if (mValue == 46) return static_cast<char>(62);
		// / -> ?
		if (mValue == 47) return static_cast<char>(63);
		// ; -> :
		if (mValue == 59) return static_cast<char>(58);
		// ' -> "
		if (mValue == 39) return static_cast<char>(34);
		// [ -> {
		if (mValue == 91) return static_cast<char>(123);
		// \ -> |
		if (mValue == 92) return static_cast<char>(124);
		// ] -> }
		if (mValue == 93) return static_cast<char>(125);
		// = -> +
		if (mValue == 61) return static_cast<char>(43);
		// ` -> ~
		if (mValue == 96) return static_cast<char>(126);

		return '\0';
	}

}
