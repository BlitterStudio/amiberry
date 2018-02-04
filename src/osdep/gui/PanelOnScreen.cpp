#include <guichan.hpp>
#include <SDL/SDL_ttf.h>
#include <guichan/sdl.hpp>
#include "sdltruetypefont.hpp"
#include "SelectorEntry.hpp"
#include "UaeRadioButton.hpp"
#include "UaeDropDown.hpp"
#include "UaeCheckBox.hpp"

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "gui_handling.h"


static gcn::UaeCheckBox* checkBox_onscreen_control;
static gcn::UaeCheckBox* checkBox_onscreen_textinput;
static gcn::UaeCheckBox* checkBox_onscreen_dpad;
static gcn::UaeCheckBox* checkBox_onscreen_button1;
static gcn::UaeCheckBox* checkBox_onscreen_button2;
static gcn::UaeCheckBox* checkBox_onscreen_button3;
static gcn::UaeCheckBox* checkBox_onscreen_button4;
static gcn::UaeCheckBox* checkBox_onscreen_button5;
static gcn::UaeCheckBox* checkBox_onscreen_button6;
static gcn::UaeCheckBox* checkBox_onscreen_custompos;
static gcn::UaeCheckBox* checkBox_floatingJoystick;
static gcn::UaeCheckBox* checkBox_disableMenuVKeyb;
static gcn::Button* button_onscreen_pos;
static gcn::Button* button_onscreen_ok;
static gcn::Button* button_onscreen_reset;
static gcn::Window *window_setup_position;
static gcn::Window *window_pos_textinput;
static gcn::Window *window_pos_dpad;
static gcn::Window *window_pos_button1;
static gcn::Window *window_pos_button2;
static gcn::Window *window_pos_button3;
static gcn::Window *window_pos_button4;
static gcn::Window *window_pos_button5;
static gcn::Window *window_pos_button6;
static gcn::Label* label_setup_onscreen;
extern gcn::TextField *txtName;

class OnScreenActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
        if (actionEvent.getSource() == checkBox_onscreen_control) {
            if (checkBox_onscreen_control->isSelected())
                changed_prefs.onScreen=1;
            else
                changed_prefs.onScreen=0;
        }
        if (actionEvent.getSource() == checkBox_onscreen_textinput) {
            if (checkBox_onscreen_textinput->isSelected())
                changed_prefs.onScreen_textinput=1;
            else
                changed_prefs.onScreen_textinput=0;
        }
        if (actionEvent.getSource() == checkBox_onscreen_dpad) {
            if (checkBox_onscreen_dpad->isSelected())
                changed_prefs.onScreen_dpad=1;
            else
                changed_prefs.onScreen_dpad=0;
        }
        if (actionEvent.getSource() == checkBox_onscreen_button1) {
            if (checkBox_onscreen_button1->isSelected())
                changed_prefs.onScreen_button1=1;
            else
                changed_prefs.onScreen_button1=0;
        }
        if (actionEvent.getSource() == checkBox_onscreen_button2) {
            if (checkBox_onscreen_button2->isSelected())
                changed_prefs.onScreen_button2=1;
            else
                changed_prefs.onScreen_button2=0;
        }
        if (actionEvent.getSource() == checkBox_onscreen_button3) {
            if (checkBox_onscreen_button3->isSelected())
                changed_prefs.onScreen_button3=1;
            else
                changed_prefs.onScreen_button3=0;
        }
        if (actionEvent.getSource() == checkBox_onscreen_button4) {
            if (checkBox_onscreen_button4->isSelected())
                changed_prefs.onScreen_button4=1;
            else
                changed_prefs.onScreen_button4=0;
        }
        if (actionEvent.getSource() == checkBox_onscreen_button5) {
            if (checkBox_onscreen_button5->isSelected())
                changed_prefs.onScreen_button5=1;
            else
                changed_prefs.onScreen_button5=0;
        }
        if (actionEvent.getSource() == checkBox_onscreen_button6) {
            if (checkBox_onscreen_button6->isSelected())
                changed_prefs.onScreen_button6=1;
            else
                changed_prefs.onScreen_button6=0;
        }
        if (actionEvent.getSource() == checkBox_onscreen_custompos) {
            if (checkBox_onscreen_custompos->isSelected())
                changed_prefs.custom_position=1;
            else
                changed_prefs.custom_position=0;
        }
        if (actionEvent.getSource() == checkBox_floatingJoystick)
            if (checkBox_floatingJoystick->isSelected())
                changed_prefs.floatingJoystick=1;
            else
                changed_prefs.floatingJoystick=0;
        if (actionEvent.getSource() == checkBox_disableMenuVKeyb)
            if (checkBox_disableMenuVKeyb->isSelected())
                changed_prefs.disableMenuVKeyb=1;
            else
                changed_prefs.disableMenuVKeyb=0;
        RefreshPanelOnScreen();
    }
};
OnScreenActionListener* onScreenActionListener;

class SetupPosButtonActionListener : public gcn::ActionListener
{
public:
    void action(const gcn::ActionEvent& actionEvent) {
        if (actionEvent.getSource() == button_onscreen_pos)
            window_setup_position->setVisible(true);
        RefreshPanelOnScreen();
    }
};
SetupPosButtonActionListener* setupPosButtonActionListener;

class WindowPosButtonActionListener : public gcn::ActionListener
{
public:
    void action(const gcn::ActionEvent& actionEvent) {
        if (actionEvent.getSource() == button_onscreen_ok) {
            changed_prefs.pos_x_textinput = window_pos_textinput->getX();
            changed_prefs.pos_y_textinput = window_pos_textinput->getY();
            changed_prefs.pos_x_dpad = window_pos_dpad->getX();
            changed_prefs.pos_y_dpad = window_pos_dpad->getY();
            changed_prefs.pos_x_button1 = window_pos_button1->getX();
            changed_prefs.pos_y_button1 = window_pos_button1->getY();
            changed_prefs.pos_x_button2 = window_pos_button2->getX();
            changed_prefs.pos_y_button2 = window_pos_button2->getY();
            changed_prefs.pos_x_button3 = window_pos_button3->getX();
            changed_prefs.pos_y_button3 = window_pos_button3->getY();
            changed_prefs.pos_x_button4 = window_pos_button4->getX();
            changed_prefs.pos_y_button4 = window_pos_button4->getY();
            changed_prefs.pos_x_button5 = window_pos_button5->getX();
            changed_prefs.pos_y_button5 = window_pos_button5->getY();
            changed_prefs.pos_x_button6 = window_pos_button6->getX();
            changed_prefs.pos_y_button6 = window_pos_button6->getY();
            window_setup_position->setVisible(false);
        }
        if (actionEvent.getSource() == button_onscreen_reset) {
            changed_prefs.pos_x_textinput = 0;
            changed_prefs.pos_y_textinput = 0;
            changed_prefs.pos_x_dpad = 4;
            changed_prefs.pos_y_dpad = 215;
            changed_prefs.pos_x_button1 = 430;
            changed_prefs.pos_y_button1 = 286;
            changed_prefs.pos_x_button2 = 378;
            changed_prefs.pos_y_button2 = 286;
            changed_prefs.pos_x_button3 = 430;
            changed_prefs.pos_y_button3 = 214;
            changed_prefs.pos_x_button4 = 378;
            changed_prefs.pos_y_button4 = 214;
            changed_prefs.pos_x_button5 = 430;
            changed_prefs.pos_y_button5 = 142;
            changed_prefs.pos_x_button6 = 378;
            changed_prefs.pos_y_button6 = 142;
            window_setup_position->setVisible(false);
        }
    }
};
WindowPosButtonActionListener* windowPosButtonActionListener;

void InitPanelOnScreen(const struct _ConfigCategory& category)
{
    onScreenActionListener = new OnScreenActionListener();
    checkBox_onscreen_control = new gcn::UaeCheckBox("On-screen control");
    checkBox_onscreen_control->setPosition(10,20);
    checkBox_onscreen_control->setId("OnScrCtrl");
    checkBox_onscreen_control->addActionListener(onScreenActionListener);
    checkBox_onscreen_textinput = new gcn::UaeCheckBox("TextInput button");
    checkBox_onscreen_textinput->setPosition(10,50);
    checkBox_onscreen_textinput->setId("OnScrTextInput");
    checkBox_onscreen_textinput->addActionListener(onScreenActionListener);
    checkBox_onscreen_dpad = new gcn::UaeCheckBox("D-pad");
    checkBox_onscreen_dpad->setPosition(10,80);
    checkBox_onscreen_dpad->setId("OnScrDpad");
    checkBox_onscreen_dpad->addActionListener(onScreenActionListener);
    checkBox_onscreen_button1 = new gcn::UaeCheckBox("Button 1 <A>");
    checkBox_onscreen_button1->setPosition(10,110);
    checkBox_onscreen_button1->setId("OnScrButton1");
    checkBox_onscreen_button1->addActionListener(onScreenActionListener);
    checkBox_onscreen_button2 = new gcn::UaeCheckBox("Button 2 <B>");
    checkBox_onscreen_button2->setPosition(10,140);
    checkBox_onscreen_button2->setId("OnScrButton2");
    checkBox_onscreen_button2->addActionListener(onScreenActionListener);
    checkBox_onscreen_button3 = new gcn::UaeCheckBox("Button 3 <X>");
    checkBox_onscreen_button3->setPosition(170,20);
    checkBox_onscreen_button3->setId("OnScrButton3");
    checkBox_onscreen_button3->addActionListener(onScreenActionListener);
    checkBox_onscreen_button4 = new gcn::UaeCheckBox("Button 4 <Y>");
    checkBox_onscreen_button4->setPosition(170,50);
    checkBox_onscreen_button4->setId("OnScrButton4");
    checkBox_onscreen_button4->addActionListener(onScreenActionListener);
    checkBox_onscreen_button5 = new gcn::UaeCheckBox("Button 5 <R>");
    checkBox_onscreen_button5->setPosition(170,80);
    checkBox_onscreen_button5->setId("OnScrButton5");
    checkBox_onscreen_button5->addActionListener(onScreenActionListener);
    checkBox_onscreen_button6 = new gcn::UaeCheckBox("Button 6 <L>");
    checkBox_onscreen_button6->setPosition(170,110);
    checkBox_onscreen_button6->setId("OnScrButton6");
    checkBox_onscreen_button6->addActionListener(onScreenActionListener);
    checkBox_onscreen_custompos = new gcn::UaeCheckBox("Custom position");
    checkBox_onscreen_custompos->setPosition(170,140);
    checkBox_onscreen_custompos->setId("CustomPos");
    checkBox_onscreen_custompos->addActionListener(onScreenActionListener);
    checkBox_floatingJoystick = new gcn::UaeCheckBox("Floating Joystick");
    checkBox_floatingJoystick->setPosition(10,180);
    checkBox_floatingJoystick->setId("FloatJoy");
    checkBox_floatingJoystick->addActionListener(onScreenActionListener);
    checkBox_disableMenuVKeyb = new gcn::UaeCheckBox("Disable virtual keyboard in menu");
    checkBox_disableMenuVKeyb->setPosition(10,210);
    checkBox_disableMenuVKeyb->setId("DisableMenuVKeyb");
    checkBox_disableMenuVKeyb->addActionListener(onScreenActionListener);

    button_onscreen_pos = new gcn::Button("Position Setup");
    button_onscreen_pos->setPosition(170,180);
    button_onscreen_pos->setBaseColor(gui_baseCol);
    setupPosButtonActionListener = new SetupPosButtonActionListener();
    button_onscreen_pos->addActionListener(setupPosButtonActionListener);

    button_onscreen_ok = new gcn::Button(" Ok ");
    button_onscreen_ok->setPosition(220,175);
    button_onscreen_ok->setBaseColor(gui_baseCol);
    button_onscreen_reset = new gcn::Button(" Reset Position to default ");
    button_onscreen_reset->setPosition(150,105);
    button_onscreen_reset->setBaseColor(gui_baseCol);
    windowPosButtonActionListener = new WindowPosButtonActionListener();
    button_onscreen_ok->addActionListener(windowPosButtonActionListener);
    button_onscreen_reset->addActionListener(windowPosButtonActionListener);
    label_setup_onscreen = new gcn::Label("Try drag and drop window then press ok");
    label_setup_onscreen->setPosition(100,140);
//    label_setup_onscreen->setFont(font14);

    window_pos_textinput = new gcn::Window("Ab");
    window_pos_textinput->setMovable(true);
    window_pos_textinput->setSize(25,30);
    window_pos_textinput->setBaseColor(gui_baseCol);
    window_pos_dpad = new gcn::Window("Dpad");
    window_pos_dpad->setMovable(true);
    window_pos_dpad->setSize(100,130);
    window_pos_dpad->setBaseColor(gui_baseCol);
    window_pos_button1 = new gcn::Window("1<A>");
    window_pos_button1->setMovable(true);
    window_pos_button1->setSize(50,65);
    window_pos_button1->setBaseColor(gui_baseCol);
    window_pos_button2 = new gcn::Window("2<B>");
    window_pos_button2->setMovable(true);
    window_pos_button2->setSize(50,65);
    window_pos_button2->setBaseColor(gui_baseCol);
    window_pos_button3 = new gcn::Window("3<X>");
    window_pos_button3->setMovable(true);
    window_pos_button3->setSize(50,65);
    window_pos_button3->setBaseColor(gui_baseCol);
    window_pos_button4 = new gcn::Window("4<Y>");
    window_pos_button4->setMovable(true);
    window_pos_button4->setSize(50,65);
    window_pos_button4->setBaseColor(gui_baseCol);
    window_pos_button5 = new gcn::Window("5<R>");
    window_pos_button5->setMovable(true);
    window_pos_button5->setSize(50,65);
    window_pos_button5->setBaseColor(gui_baseCol);
    window_pos_button6 = new gcn::Window("6<L>");
    window_pos_button6->setMovable(true);
    window_pos_button6->setSize(50,65);
    window_pos_button6->setBaseColor(gui_baseCol);

    window_setup_position = new gcn::Window("Setup position");
    window_setup_position->setPosition(60,20);
    window_setup_position->add(label_setup_onscreen);
    window_setup_position->add(button_onscreen_ok);
    window_setup_position->add(button_onscreen_reset);
    window_setup_position->add(window_pos_textinput);
    window_setup_position->add(window_pos_dpad);
    window_setup_position->add(window_pos_button1);
    window_setup_position->add(window_pos_button2);
    window_setup_position->add(window_pos_button3);
    window_setup_position->add(window_pos_button4);
    window_setup_position->add(window_pos_button5);
    window_setup_position->add(window_pos_button6);
    window_setup_position->setMovable(false);
    window_setup_position->setSize(485,370);   
    window_setup_position->setVisible(false);
    
    category.panel->add(checkBox_onscreen_control);
    category.panel->add(checkBox_onscreen_textinput);
    category.panel->add(checkBox_onscreen_dpad);
    category.panel->add(checkBox_onscreen_button1);
    category.panel->add(checkBox_onscreen_button2);
    category.panel->add(checkBox_onscreen_button3);
    category.panel->add(checkBox_onscreen_button4);
    category.panel->add(checkBox_onscreen_button5);
    category.panel->add(checkBox_onscreen_button6);
    category.panel->add(checkBox_onscreen_custompos);
    category.panel->add(checkBox_floatingJoystick);
    category.panel->add(checkBox_disableMenuVKeyb);
    category.panel->add(button_onscreen_pos);
    category.panel->add(window_setup_position);
    
  RefreshPanelOnScreen();
}


void ExitPanelOnScreen(void)
{
    delete checkBox_onscreen_control;
    delete checkBox_onscreen_textinput;
    delete checkBox_onscreen_dpad;
    delete checkBox_onscreen_button1;
    delete checkBox_onscreen_button2;
    delete checkBox_onscreen_button3;
    delete checkBox_onscreen_button4;
    delete checkBox_onscreen_button5;
    delete checkBox_onscreen_button6;
    delete checkBox_onscreen_custompos;
    delete checkBox_floatingJoystick;
    delete checkBox_disableMenuVKeyb;
    delete button_onscreen_pos;
    delete button_onscreen_ok;
    delete button_onscreen_reset;
    delete window_setup_position;
    delete window_pos_textinput;
    delete window_pos_dpad;
    delete window_pos_button1;
    delete window_pos_button2;
    delete window_pos_button3;
    delete window_pos_button4;
    delete window_pos_button5;
    delete window_pos_button6;
    delete label_setup_onscreen;
  delete setupPosButtonActionListener;
  delete windowPosButtonActionListener;
  delete onScreenActionListener;
}


void RefreshPanelOnScreen(void)
{
    if (changed_prefs.onScreen==0)
        checkBox_onscreen_control->setSelected(false);
    else if (changed_prefs.onScreen==1)
        checkBox_onscreen_control->setSelected(true);
    if (changed_prefs.onScreen_textinput==0)
        checkBox_onscreen_textinput->setSelected(false);
    else if (changed_prefs.onScreen_textinput==1)
        checkBox_onscreen_textinput->setSelected(true);
    if (changed_prefs.onScreen_dpad==0)
        checkBox_onscreen_dpad->setSelected(false);
    else if (changed_prefs.onScreen_dpad==1)
        checkBox_onscreen_dpad->setSelected(true);
    if (changed_prefs.onScreen_button1==0)
        checkBox_onscreen_button1->setSelected(false);
    else if (changed_prefs.onScreen_button1==1)
        checkBox_onscreen_button1->setSelected(true);
    if (changed_prefs.onScreen_button2==0)
        checkBox_onscreen_button2->setSelected(false);
    else if (changed_prefs.onScreen_button2==1)
        checkBox_onscreen_button2->setSelected(true);
    if (changed_prefs.onScreen_button3==0)
        checkBox_onscreen_button3->setSelected(false);
    else if (changed_prefs.onScreen_button3==1)
        checkBox_onscreen_button3->setSelected(true);
    if (changed_prefs.onScreen_button4==0)
        checkBox_onscreen_button4->setSelected(false);
    else if (changed_prefs.onScreen_button4==1)
        checkBox_onscreen_button4->setSelected(true);
    if (changed_prefs.onScreen_button5==0)
        checkBox_onscreen_button5->setSelected(false);
    else if (changed_prefs.onScreen_button5==1)
        checkBox_onscreen_button5->setSelected(true);
    if (changed_prefs.onScreen_button6==0)
        checkBox_onscreen_button6->setSelected(false);
    else if (changed_prefs.onScreen_button6==1)
        checkBox_onscreen_button6->setSelected(true);
    if (changed_prefs.custom_position==0)
        checkBox_onscreen_custompos->setSelected(false);
    else if (changed_prefs.custom_position==1)
        checkBox_onscreen_custompos->setSelected(true);
    if (changed_prefs.floatingJoystick)
        checkBox_floatingJoystick->setSelected(true);
    else
        checkBox_floatingJoystick->setSelected(false);
    if (changed_prefs.disableMenuVKeyb)
        checkBox_disableMenuVKeyb->setSelected(true);
    else
        checkBox_disableMenuVKeyb->setSelected(false);
    
    txtName->disableVirtualKeyboard(changed_prefs.disableMenuVKeyb);
    
    window_pos_textinput->setX(changed_prefs.pos_x_textinput);
    window_pos_textinput->setY(changed_prefs.pos_y_textinput);
    window_pos_textinput->setVisible(changed_prefs.onScreen_textinput);
    window_pos_dpad->setX(changed_prefs.pos_x_dpad);
    window_pos_dpad->setY(changed_prefs.pos_y_dpad);
    window_pos_dpad->setVisible(changed_prefs.onScreen_dpad);
    window_pos_button1->setX(changed_prefs.pos_x_button1);
    window_pos_button1->setY(changed_prefs.pos_y_button1);
    window_pos_button1->setVisible(changed_prefs.onScreen_button1);
    window_pos_button2->setX(changed_prefs.pos_x_button2);
    window_pos_button2->setY(changed_prefs.pos_y_button2);
    window_pos_button2->setVisible(changed_prefs.onScreen_button2);
    window_pos_button3->setX(changed_prefs.pos_x_button3);
    window_pos_button3->setY(changed_prefs.pos_y_button3);
    window_pos_button3->setVisible(changed_prefs.onScreen_button3);
    window_pos_button4->setX(changed_prefs.pos_x_button4);
    window_pos_button4->setY(changed_prefs.pos_y_button4);
    window_pos_button4->setVisible(changed_prefs.onScreen_button4);
    window_pos_button5->setX(changed_prefs.pos_x_button5);
    window_pos_button5->setY(changed_prefs.pos_y_button5);
    window_pos_button5->setVisible(changed_prefs.onScreen_button5);
    window_pos_button6->setX(changed_prefs.pos_x_button6);
    window_pos_button6->setY(changed_prefs.pos_y_button6);
    window_pos_button6->setVisible(changed_prefs.onScreen_button6);
    button_onscreen_pos->setVisible(changed_prefs.custom_position);
}

bool HelpPanelOnScreen(std::vector<std::string> &helptext)
{
  helptext.clear();
  helptext.push_back("onScreen Help");
  return true;
}
