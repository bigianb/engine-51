#pragma once

enum class InputGadget
{
    Undefined,

    INPUT_MSG_EXIT,

    INPUT_MOUSE__ANALOG,

    INPUT_MOUSE_X_REL,
    INPUT_MOUSE_Y_REL,
    INPUT_MOUSE_WHEEL_REL,

    INPUT_MOUSE_X_ABS,
    INPUT_MOUSE_Y_ABS,
    INPUT_MOUSE_WHEEL_ABS,

    INPUT_MOUSE_BTN_L,
    INPUT_MOUSE_BTN_C,
    INPUT_MOUSE_BTN_R,

    INPUT_PS2_BTN_L2,         //0 ANALOG            
    INPUT_PS2_BTN_R2,         //1 ANALOG            
    INPUT_PS2_BTN_L1,         //2 ANALOG            
    INPUT_PS2_BTN_R1,         //3 ANALOG            

    INPUT_PS2_BTN_TRIANGLE,   //4 ANALOG            
    INPUT_PS2_BTN_CIRCLE,     //5 ANALOG            
    INPUT_PS2_BTN_CROSS,      //6 ANALOG            
    INPUT_PS2_BTN_SQUARE,     //7 ANALOG            

    INPUT_PS2_BTN_SELECT,     //8          
    INPUT_PS2_BTN_L_STICK,    //9            
    INPUT_PS2_BTN_R_STICK,    //10            
    INPUT_PS2_BTN_START,      //11            

    INPUT_PS2_BTN_L_UP,       //12 ANALOG            
    INPUT_PS2_BTN_L_RIGHT,    //13 ANALOG            
    INPUT_PS2_BTN_L_DOWN,     //14 ANALOG            
    INPUT_PS2_BTN_L_LEFT,     //15 ANALOG            
    
    INPUT_PS2_STICK_LEFT_X,   // ANALOG            
    INPUT_PS2_STICK_LEFT_Y,   // ANALOG            
    INPUT_PS2_STICK_RIGHT_X,  // ANALOG            
    INPUT_PS2_STICK_RIGHT_Y,  // ANALOG            
};
