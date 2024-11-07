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

/*
 * For comments regarding functions please see the header file.
 */

#include "guisan/widgets/inputbox.hpp"

#include "guisan/exception.hpp"
#include "guisan/font.hpp"
#include "guisan/graphics.hpp"
#include "guisan/mouseinput.hpp"

namespace gcn
{

    InputBox::InputBox(const std::string& caption,
                       const std::string& message,
                       const std::string& ok,
                       const std::string& cancel) :
        Window(caption)
    {
        setMovable(false);

        mLabel = std::make_unique<Label>(message);
        mLabel->setAlignment(Graphics::Left);
        mLabel->adjustSize();

        mText = std::make_unique<TextField>();

        mButtonOK = std::make_unique<Button>(ok);
        mButtonOK->setAlignment(Graphics::Center);
        mButtonOK->addActionListener(this);
        mButtonOK->adjustSize();

        mButtonCancel = std::make_unique<Button>(cancel);
        mButtonCancel->setAlignment(Graphics::Center);
        mButtonCancel->addActionListener(this);
        mButtonCancel->adjustSize();

        // Look-and-feel: make both buttons the same width
        if (mButtonCancel->getWidth() > mButtonOK->getWidth())
        {
            mButtonOK->setWidth(mButtonCancel->getWidth());
        }
        else
        {
            mButtonCancel->setWidth(mButtonOK->getWidth());
        }

        setHeight((int) getTitleBarHeight() + mLabel->getHeight() + mText->getHeight()
                  + 6 * mPadding + mButtonOK->getHeight() + 2 * getFrameSize());
        setWidth(mLabel->getWidth() + 2 * mPadding + 2 * getFrameSize());
        if (2 * mButtonOK->getWidth() + 4 * mPadding + 2 * getFrameSize() > getWidth())
        {
            setWidth(2 * mButtonOK->getWidth() + 4 * mPadding + 2 * getFrameSize());
        }
        mText->setWidth(getWidth() - 2 * getFrameSize() - 5 * mPadding);

        this->add(mLabel.get(), (getWidth() - mLabel->getWidth()) / 2 - mPadding, mPadding);
        this->add(mText.get(), 2 * mPadding, 2 * mPadding + mLabel->getHeight());
        int yButtons = getHeight() - (int) getTitleBarHeight() - getFrameSize() - 2 * mPadding
                     - mButtonOK->getHeight();
        this->add(mButtonOK.get(), (getWidth() - 2 * mButtonOK->getWidth()) / 4, yButtons);
        this->add(mButtonCancel.get(),
                  getWidth() - 2 * getFrameSize() - mButtonOK->getWidth() - mPadding,
                  yButtons);

        try
        {
            requestModalFocus();
        }
        catch (Exception e)
        {
            // Not having modal focus is not critical
        }
    }

    InputBox::~InputBox()
    {
        releaseModalFocus();
    }

    void InputBox::action(const ActionEvent& actionEvent)
    {
        if (actionEvent.getSource() == mButtonOK.get())
        {
            mClickedButton = 0;
            distributeActionEvent();
        }
        else if (actionEvent.getSource() == mButtonCancel.get())
        {
            mClickedButton = 1;
            setVisible(false);
            distributeActionEvent();
        }
    }

    int InputBox::getClickedButton() const
    {
        return mClickedButton;
    }

    std::string InputBox::getText() const
    {
        return mText->getText();
    }

    void InputBox::addToContainer(Container* container)
    {
        int x = container->getWidth() - getWidth();
        int y = container->getHeight() - getHeight();
        container->add(this, x / 2, y / 2);
    }
} // namespace gcn
