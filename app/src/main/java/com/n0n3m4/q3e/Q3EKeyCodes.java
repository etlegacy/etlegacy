/*
 	Copyright (C) 2012 n0n3m4
	
    This file is part of Q3E.

    Q3E is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Q3E is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Q3E.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.n0n3m4.q3e;

import android.util.Log;
import android.view.KeyEvent;

import com.n0n3m4.q3e.device.Q3EOuya;

import java.lang.reflect.Field;
import java.util.HashMap;
import java.util.Map;

public class Q3EKeyCodes
{

    public static final int K_VKBD = 9000;

    public static class KeyCodesRTCW
    {
        public static final int K_TAB = 9;
        public static final int K_ENTER = 13;
        public static final int K_ESCAPE = 27;
        public static final int K_SPACE = 32;

        public static final int K_BACKSPACE = 127;

        public static final int K_COMMAND = 128;
        public static final int K_CAPSLOCK = 129;
        public static final int K_POWER = 130;
        public static final int K_PAUSE = 131;

        public static final int K_UPARROW = 132;
        public static final int K_DOWNARROW = 133;
        public static final int K_LEFTARROW = 134;
        public static final int K_RIGHTARROW = 135;

        public static final int K_ALT = 136;
        public static final int K_CTRL = 137;
        public static final int K_SHIFT = 138;
        public static final int K_INS = 139;
        public static final int K_DEL = 140;
        public static final int K_PGDN = 141;
        public static final int K_PGUP = 142;
        public static final int K_HOME = 143;
        public static final int K_END = 144;

        public static final int K_F1 = 145;
        public static final int K_F2 = 146;
        public static final int K_F3 = 147;
        public static final int K_F4 = 148;
        public static final int K_F5 = 149;
        public static final int K_F6 = 150;
        public static final int K_F7 = 151;
        public static final int K_F8 = 152;
        public static final int K_F9 = 153;
        public static final int K_F10 = 154;
        public static final int K_F11 = 155;
        public static final int K_F12 = 156;
        public static final int K_F13 = 157;
        public static final int K_F14 = 158;
        public static final int K_F15 = 159;

        public static final int K_KP_HOME = 160;
        public static final int K_KP_UPARROW = 161;
        public static final int K_KP_PGUP = 162;
        public static final int K_KP_LEFTARROW = 163;
        public static final int K_KP_5 = 164;
        public static final int K_KP_RIGHTARROW = 165;
        public static final int K_KP_END = 166;
        public static final int K_KP_DOWNARROW = 167;
        public static final int K_KP_PGDN = 168;
        public static final int K_KP_ENTER = 169;
        public static final int K_KP_INS = 170;
        public static final int K_KP_DEL = 171;
        public static final int K_KP_SLASH = 172;
        public static final int K_KP_MINUS = 173;
        public static final int K_KP_PLUS = 174;
        public static final int K_KP_NUMLOCK = 175;
        public static final int K_KP_STAR = 176;
        public static final int K_KP_EQUALS = 177;

        public static final int K_MOUSE1 = 178;
        public static final int K_MOUSE2 = 179;
        public static final int K_MOUSE3 = 180;
        public static final int K_MOUSE4 = 181;
        public static final int K_MOUSE5 = 182;

        public static final int K_MWHEELDOWN = 183;
        public static final int K_MWHEELUP = 184;

        public static final int J_LEFT = 'a';
        public static final int J_RIGHT = 'd';
        public static final int J_UP = K_UPARROW;
        public static final int J_DOWN = K_DOWNARROW;
    }

    ;

    public static class KeyCodesQ3
    {
        public static final int K_TAB = 9;
        public static final int K_ENTER = 13;
        public static final int K_ESCAPE = 27;
        public static final int K_SPACE = 32;

        public static final int K_BACKSPACE = 127;

        public static final int K_COMMAND = 128;
        public static final int K_CAPSLOCK = 129;
        public static final int K_POWER = 130;
        public static final int K_PAUSE = 131;

        public static final int K_UPARROW = 132;
        public static final int K_DOWNARROW = 133;
        public static final int K_LEFTARROW = 134;
        public static final int K_RIGHTARROW = 135;

        public static final int K_ALT = 136;
        public static final int K_CTRL = 137;
        public static final int K_SHIFT = 138;
        public static final int K_INS = 139;
        public static final int K_DEL = 140;
        public static final int K_PGDN = 141;
        public static final int K_PGUP = 142;
        public static final int K_HOME = 143;
        public static final int K_END = 144;

        public static final int K_F1 = 145;
        public static final int K_F2 = 146;
        public static final int K_F3 = 147;
        public static final int K_F4 = 148;
        public static final int K_F5 = 149;
        public static final int K_F6 = 150;
        public static final int K_F7 = 151;
        public static final int K_F8 = 152;
        public static final int K_F9 = 153;
        public static final int K_F10 = 154;
        public static final int K_F11 = 155;
        public static final int K_F12 = 156;
        public static final int K_F13 = 157;
        public static final int K_F14 = 158;
        public static final int K_F15 = 159;

        public static final int K_KP_HOME = 160;
        public static final int K_KP_UPARROW = 161;
        public static final int K_KP_PGUP = 162;
        public static final int K_KP_LEFTARROW = 163;
        public static final int K_KP_5 = 164;
        public static final int K_KP_RIGHTARROW = 165;
        public static final int K_KP_END = 166;
        public static final int K_KP_DOWNARROW = 167;
        public static final int K_KP_PGDN = 168;
        public static final int K_KP_ENTER = 169;
        public static final int K_KP_INS = 170;
        public static final int K_KP_DEL = 171;
        public static final int K_KP_SLASH = 172;
        public static final int K_KP_MINUS = 173;
        public static final int K_KP_PLUS = 174;
        public static final int K_KP_NUMLOCK = 175;
        public static final int K_KP_STAR = 176;
        public static final int K_KP_EQUALS = 177;

        public static final int K_MOUSE1 = 178;
        public static final int K_MOUSE2 = 179;
        public static final int K_MOUSE3 = 180;
        public static final int K_MOUSE4 = 181;
        public static final int K_MOUSE5 = 182;

        public static final int K_MWHEELDOWN = 183;
        public static final int K_MWHEELUP = 184;

        public static final int J_LEFT = 'a';
        public static final int J_RIGHT = 'd';
        public static final int J_UP = K_UPARROW;
        public static final int J_DOWN = K_DOWNARROW;
    }

    ;

    public static class KeyCodesD3
    {
        public static final int K_TAB = 9;
        public static final int K_ENTER = 13;
        public static final int K_ESCAPE = 27;
        public static final int K_SPACE = 32;
        public static final int K_BACKSPACE = 127;
        public static final int K_COMMAND = 128;
        public static final int K_CAPSLOCK = 129;
        public static final int K_SCROLL = 130;
        public static final int K_POWER = 131;
        public static final int K_PAUSE = 132;
        public static final int K_UPARROW = 133;
        public static final int K_DOWNARROW = 134;
        public static final int K_LEFTARROW = 135;
        public static final int K_RIGHTARROW = 136;
        public static final int K_LWIN = 137;
        public static final int K_RWIN = 138;
        public static final int K_MENU = 139;
        public static final int K_ALT = 140;
        public static final int K_CTRL = 141;
        public static final int K_SHIFT = 142;
        public static final int K_INS = 143;
        public static final int K_DEL = 144;
        public static final int K_PGDN = 145;
        public static final int K_PGUP = 146;
        public static final int K_HOME = 147;
        public static final int K_END = 148;
        public static final int K_F1 = 149;
        public static final int K_F2 = 150;
        public static final int K_F3 = 151;
        public static final int K_F4 = 152;
        public static final int K_F5 = 153;
        public static final int K_F6 = 154;
        public static final int K_F7 = 155;
        public static final int K_F8 = 156;
        public static final int K_F9 = 157;
        public static final int K_F10 = 158;
        public static final int K_F11 = 159;
        public static final int K_F12 = 160;
        public static final int K_INVERTED_EXCLAMATION = 161;
        public static final int K_F13 = 162;
        public static final int K_F14 = 163;
        public static final int K_F15 = 164;
        public static final int K_MOUSE1 = 187;
        public static final int K_MOUSE2 = 188;
        public static final int K_MOUSE3 = 189;
        public static final int K_MOUSE4 = 190;
        public static final int K_MOUSE5 = 191;
        public static final int K_MWHEELDOWN = 195;
        public static final int K_MWHEELUP = 196;

        public static final int J_LEFT = 'a';
        public static final int J_RIGHT = 'd';
        public static final int J_UP = K_UPARROW;
        public static final int J_DOWN = K_DOWNARROW;
    }

    ;

    public static class KeyCodesD3BFG
    {
        public static final int K_NONE = 0;
        public static final int K_ESCAPE = 1;
        public static final int K_1 = 2;
        public static final int K_2 = 3;
        public static final int K_3 = 4;
        public static final int K_4 = 5;
        public static final int K_5 = 6;
        public static final int K_6 = 7;
        public static final int K_7 = 8;
        public static final int K_8 = 9;
        public static final int K_9 = 10;
        public static final int K_0 = 11;
        public static final int K_MINUS = 12;
        public static final int K_EQUALS = 13;
        public static final int K_BACKSPACE = 14;
        public static final int K_TAB = 15;
        public static final int K_Q = 16;
        public static final int K_W = 17;
        public static final int K_E = 18;
        public static final int K_R = 19;
        public static final int K_T = 20;
        public static final int K_Y = 21;
        public static final int K_U = 22;
        public static final int K_I = 23;
        public static final int K_O = 24;
        public static final int K_P = 25;
        public static final int K_LBRACKET = 26;
        public static final int K_RBRACKET = 27;
        public static final int K_ENTER = 28;
        public static final int K_CTRL = 29;
        public static final int K_A = 30;
        public static final int K_S = 31;
        public static final int K_D = 32;
        public static final int K_F = 33;
        public static final int K_G = 34;
        public static final int K_H = 35;
        public static final int K_J = 36;
        public static final int K_K = 37;
        public static final int K_L = 38;
        public static final int K_SEMICOLON = 39;
        public static final int K_APOSTROPHE = 40;
        public static final int K_GRAVE = 41;
        public static final int K_SHIFT = 42;
        public static final int K_BACKSLASH = 43;
        public static final int K_Z = 44;
        public static final int K_X = 45;
        public static final int K_C = 46;
        public static final int K_V = 47;
        public static final int K_B = 48;
        public static final int K_N = 49;
        public static final int K_M = 50;
        public static final int K_COMMA = 51;
        public static final int K_PERIOD = 52;
        public static final int K_SLASH = 53;
        public static final int K_RSHIFT = 54;
        public static final int K_KP_STAR = 55;
        public static final int K_ALT = 56;
        public static final int K_SPACE = 57;
        public static final int K_CAPSLOCK = 58;
        public static final int K_F1 = 59;
        public static final int K_F2 = 60;
        public static final int K_F3 = 61;
        public static final int K_F4 = 62;
        public static final int K_F5 = 63;
        public static final int K_F6 = 64;
        public static final int K_F7 = 65;
        public static final int K_F8 = 66;
        public static final int K_F9 = 67;
        public static final int K_F10 = 68;
        public static final int K_NUMLOCK = 69;
        public static final int K_SCROLL = 70;
        public static final int K_KP_7 = 71;
        public static final int K_KP_8 = 72;
        public static final int K_KP_9 = 73;
        public static final int K_KP_MINUS = 74;
        public static final int K_KP_4 = 75;
        public static final int K_KP_5 = 76;
        public static final int K_KP_6 = 77;
        public static final int K_KP_PLUS = 78;
        public static final int K_KP_1 = 79;
        public static final int K_KP_2 = 80;
        public static final int K_KP_3 = 81;
        public static final int K_KP_0 = 82;
        public static final int K_KP_DOT = 83;
        public static final int K_F11 = 0x57;
        public static final int K_F12 = 0x58;
        public static final int K_F13 = 0x64;
        public static final int K_F14 = 0x65;
        public static final int K_F15 = 0x66;
        public static final int K_KANA = 0x70;
        public static final int K_CONVERT = 0x79;
        public static final int K_NOCONVERT = 0x7B;
        public static final int K_YEN = 0x7D;
        public static final int K_KP_EQUALS = 0x8D;
        public static final int K_CIRCUMFLEX = 0x90;
        public static final int K_AT = 0x91;
        public static final int K_COLON = 0x92;
        public static final int K_UNDERLINE = 0x93;
        public static final int K_KANJI = 0x94;
        public static final int K_STOP = 0x95;
        public static final int K_AX = 0x96;
        public static final int K_UNLABELED = 0x97;
        public static final int K_KP_ENTER = 0x9C;
        public static final int K_RCTRL = 0x9D;
        public static final int K_KP_COMMA = 0xB3;
        public static final int K_KP_SLASH = 0xB5;
        public static final int K_PRINTSCREEN = 0xB7;
        public static final int K_RALT = 0xB8;
        public static final int K_PAUSE = 0xC5;
        public static final int K_HOME = 0xC7;
        public static final int K_UPARROW = 0xC8;
        public static final int K_PGUP = 0xC9;
        public static final int K_LEFTARROW = 0xCB;
        public static final int K_RIGHTARROW = 0xCD;
        public static final int K_END = 0xCF;
        public static final int K_DOWNARROW = 0xD0;
        public static final int K_PGDN = 0xD1;
        public static final int K_INS = 0xD2;
        public static final int K_DEL = 0xD3;
        public static final int K_LWIN = 0xDB;
        public static final int K_RWIN = 0xDC;
        public static final int K_APPS = 0xDD;
        public static final int K_POWER = 0xDE;
        public static final int K_SLEEP = 0xDF;
        public static final int K_MOUSE1 = 286;
        public static final int K_MOUSE2 = 287;
        public static final int K_MOUSE3 = 288;
        public static final int K_MOUSE4 = 289;
        public static final int K_MOUSE5 = 290;
        public static final int K_MOUSE6 = 291;
        public static final int K_MOUSE7 = 292;
        public static final int K_MOUSE8 = 293;
        public static final int K_MWHEELDOWN = 294;
        public static final int K_MWHEELUP = 295;

        //karin: change to a/d
//        public static final int J_LEFT = K_LEFTARROW;
//        public static final int J_RIGHT = K_RIGHTARROW;
        public static final int J_LEFT = K_A;
        public static final int J_RIGHT = K_D;
        public static final int J_UP = K_UPARROW;
        public static final int J_DOWN = K_DOWNARROW;
    }

    ;

    public static class KeyCodesQ1
    {
        public static final int K_TAB = 9;
        public static final int K_ENTER = 13;
        public static final int K_ESCAPE = 27;
        public static final int K_SPACE = 32;
        public static final int K_BACKSPACE = 127;
        public static final int K_UPARROW = 128;
        public static final int K_DOWNARROW = 129;
        public static final int K_LEFTARROW = 130;
        public static final int K_RIGHTARROW = 131;
        public static final int K_ALT = 132;
        public static final int K_CTRL = 133;
        public static final int K_SHIFT = 134;
        public static final int K_F1 = 135;
        public static final int K_F2 = 136;
        public static final int K_F3 = 137;
        public static final int K_F4 = 138;
        public static final int K_F5 = 139;
        public static final int K_F6 = 140;
        public static final int K_F7 = 141;
        public static final int K_F8 = 142;
        public static final int K_F9 = 143;
        public static final int K_F10 = 144;
        public static final int K_F11 = 145;
        public static final int K_F12 = 146;
        public static final int K_INS = 147;
        public static final int K_DEL = 148;
        public static final int K_PGDN = 149;
        public static final int K_PGUP = 150;
        public static final int K_HOME = 151;
        public static final int K_END = 152;
        public static final int K_PAUSE = 153;
        public static final int K_NUMLOCK = 154;
        public static final int K_CAPSLOCK = 155;
        public static final int K_SCROLLOCK = 156;
        public static final int K_MOUSE1 = 512;
        public static final int K_MOUSE2 = 513;
        public static final int K_MOUSE3 = 514;
        public static final int K_MWHEELUP = 515;
        public static final int K_MWHEELDOWN = 516;
        public static final int K_MOUSE4 = 517;
        public static final int K_MOUSE5 = 518;

        public static final int J_LEFT = 'a';
        public static final int J_RIGHT = 'd';
        public static final int J_UP = K_UPARROW;
        public static final int J_DOWN = K_DOWNARROW;
    }

    ;

    private static final int SDLK_SCANCODE_MASK = (1<<30);
    private static int SDL_SCANCODE_TO_KEYCODE(int X) { return (X | SDLK_SCANCODE_MASK); }

    private static final class SDL_Scancode
    {
        public static final int SDL_SCANCODE_UNKNOWN = 0;

        /**
         * \name Usage page 0x07
         * <p>
         * These values are from usage page 0x07 (USB keyboard page).
         */
        /* @{ */

        public static final int SDL_SCANCODE_A = 4;
        public static final int SDL_SCANCODE_B = 5;
        public static final int SDL_SCANCODE_C = 6;
        public static final int SDL_SCANCODE_D = 7;
        public static final int SDL_SCANCODE_E = 8;
        public static final int SDL_SCANCODE_F = 9;
        public static final int SDL_SCANCODE_G = 10;
        public static final int SDL_SCANCODE_H = 11;
        public static final int SDL_SCANCODE_I = 12;
        public static final int SDL_SCANCODE_J = 13;
        public static final int SDL_SCANCODE_K = 14;
        public static final int SDL_SCANCODE_L = 15;
        public static final int SDL_SCANCODE_M = 16;
        public static final int SDL_SCANCODE_N = 17;
        public static final int SDL_SCANCODE_O = 18;
        public static final int SDL_SCANCODE_P = 19;
        public static final int SDL_SCANCODE_Q = 20;
        public static final int SDL_SCANCODE_R = 21;
        public static final int SDL_SCANCODE_S = 22;
        public static final int SDL_SCANCODE_T = 23;
        public static final int SDL_SCANCODE_U = 24;
        public static final int SDL_SCANCODE_V = 25;
        public static final int SDL_SCANCODE_W = 26;
        public static final int SDL_SCANCODE_X = 27;
        public static final int SDL_SCANCODE_Y = 28;
        public static final int SDL_SCANCODE_Z = 29;

        public static final int SDL_SCANCODE_1 = 30;
        public static final int SDL_SCANCODE_2 = 31;
        public static final int SDL_SCANCODE_3 = 32;
        public static final int SDL_SCANCODE_4 = 33;
        public static final int SDL_SCANCODE_5 = 34;
        public static final int SDL_SCANCODE_6 = 35;
        public static final int SDL_SCANCODE_7 = 36;
        public static final int SDL_SCANCODE_8 = 37;
        public static final int SDL_SCANCODE_9 = 38;
        public static final int SDL_SCANCODE_0 = 39;

        public static final int SDL_SCANCODE_RETURN = 40;
        public static final int SDL_SCANCODE_ESCAPE = 41;
        public static final int SDL_SCANCODE_BACKSPACE = 42;
        public static final int SDL_SCANCODE_TAB = 43;
        public static final int SDL_SCANCODE_SPACE = 44;

        public static final int SDL_SCANCODE_MINUS = 45;
        public static final int SDL_SCANCODE_EQUALS = 46;
        public static final int SDL_SCANCODE_LEFTBRACKET = 47;
        public static final int SDL_SCANCODE_RIGHTBRACKET = 48;
        public static final int SDL_SCANCODE_BACKSLASH = 49;
        /**
         * < Located at the lower left of the return
         * key on ISO keyboards and at the right end
         * of the QWERTY row on ANSI keyboards.
         * Produces REVERSE SOLIDUS (backslash) and
         * VERTICAL LINE in a US layout, REVERSE
         * SOLIDUS and VERTICAL LINE in a UK Mac
         * layout, NUMBER SIGN and TILDE in a UK
         * Windows layout, DOLLAR SIGN and POUND SIGN
         * in a Swiss German layout, NUMBER SIGN and
         * APOSTROPHE in a German layout, GRAVE
         * ACCENT and POUND SIGN in a French Mac
         * layout, and ASTERISK and MICRO SIGN in a
         * French Windows layout.
         */
        public static final int SDL_SCANCODE_NONUSHASH = 50;
        /**
         * < ISO USB keyboards actually use this code
         * instead of 49 for the same key, but all
         * OSes I've seen treat the two codes
         * identically. So, as an implementor, unless
         * your keyboard generates both of those
         * codes and your OS treats them differently,
         * you should generate SDL_SCANCODE_BACKSLASH
         * instead of this code. As a user, you
         * should not rely on this code because SDL
         * will never generate it with most (all?)
         * keyboards.
         */
        public static final int SDL_SCANCODE_SEMICOLON = 51;
        public static final int SDL_SCANCODE_APOSTROPHE = 52;
        public static final int SDL_SCANCODE_GRAVE = 53;
        /**
         * < Located in the top left corner (on both ANSI
         * and ISO keyboards). Produces GRAVE ACCENT and
         * TILDE in a US Windows layout and in US and UK
         * Mac layouts on ANSI keyboards, GRAVE ACCENT
         * and NOT SIGN in a UK Windows layout, SECTION
         * SIGN and PLUS-MINUS SIGN in US and UK Mac
         * layouts on ISO keyboards, SECTION SIGN and
         * DEGREE SIGN in a Swiss German layout (Mac:
         * only on ISO keyboards), CIRCUMFLEX ACCENT and
         * DEGREE SIGN in a German layout (Mac: only on
         * ISO keyboards), SUPERSCRIPT TWO and TILDE in a
         * French Windows layout, COMMERCIAL AT and
         * NUMBER SIGN in a French Mac layout on ISO
         * keyboards, and LESS-THAN SIGN and GREATER-THAN
         * SIGN in a Swiss German, German, or French Mac
         * layout on ANSI keyboards.
         */
        public static final int SDL_SCANCODE_COMMA = 54;
        public static final int SDL_SCANCODE_PERIOD = 55;
        public static final int SDL_SCANCODE_SLASH = 56;

        public static final int SDL_SCANCODE_CAPSLOCK = 57;

        public static final int SDL_SCANCODE_F1 = 58;
        public static final int SDL_SCANCODE_F2 = 59;
        public static final int SDL_SCANCODE_F3 = 60;
        public static final int SDL_SCANCODE_F4 = 61;
        public static final int SDL_SCANCODE_F5 = 62;
        public static final int SDL_SCANCODE_F6 = 63;
        public static final int SDL_SCANCODE_F7 = 64;
        public static final int SDL_SCANCODE_F8 = 65;
        public static final int SDL_SCANCODE_F9 = 66;
        public static final int SDL_SCANCODE_F10 = 67;
        public static final int SDL_SCANCODE_F11 = 68;
        public static final int SDL_SCANCODE_F12 = 69;

        public static final int SDL_SCANCODE_PRINTSCREEN = 70;
        public static final int SDL_SCANCODE_SCROLLLOCK = 71;
        public static final int SDL_SCANCODE_PAUSE = 72;
        public static final int SDL_SCANCODE_INSERT = 73;
        /**
         * < insert on PC, help on some Mac keyboards (but
         * does send code 73, not 117)
         */
        public static final int SDL_SCANCODE_HOME = 74;
        public static final int SDL_SCANCODE_PAGEUP = 75;
        public static final int SDL_SCANCODE_DELETE = 76;
        public static final int SDL_SCANCODE_END = 77;
        public static final int SDL_SCANCODE_PAGEDOWN = 78;
        public static final int SDL_SCANCODE_RIGHT = 79;
        public static final int SDL_SCANCODE_LEFT = 80;
        public static final int SDL_SCANCODE_DOWN = 81;
        public static final int SDL_SCANCODE_UP = 82;

        public static final int SDL_SCANCODE_NUMLOCKCLEAR = 83;
        /**
         * < num lock on PC, clear on Mac keyboards
         */
        public static final int SDL_SCANCODE_KP_DIVIDE = 84;
        public static final int SDL_SCANCODE_KP_MULTIPLY = 85;
        public static final int SDL_SCANCODE_KP_MINUS = 86;
        public static final int SDL_SCANCODE_KP_PLUS = 87;
        public static final int SDL_SCANCODE_KP_ENTER = 88;
        public static final int SDL_SCANCODE_KP_1 = 89;
        public static final int SDL_SCANCODE_KP_2 = 90;
        public static final int SDL_SCANCODE_KP_3 = 91;
        public static final int SDL_SCANCODE_KP_4 = 92;
        public static final int SDL_SCANCODE_KP_5 = 93;
        public static final int SDL_SCANCODE_KP_6 = 94;
        public static final int SDL_SCANCODE_KP_7 = 95;
        public static final int SDL_SCANCODE_KP_8 = 96;
        public static final int SDL_SCANCODE_KP_9 = 97;
        public static final int SDL_SCANCODE_KP_0 = 98;
        public static final int SDL_SCANCODE_KP_PERIOD = 99;

        public static final int SDL_SCANCODE_NONUSBACKSLASH = 100;
        /**
         * < This is the additional key that ISO
         * keyboards have over ANSI ones,
         * located between left shift and Y.
         * Produces GRAVE ACCENT and TILDE in a
         * US or UK Mac layout, REVERSE SOLIDUS
         * (backslash) and VERTICAL LINE in a
         * US or UK Windows layout, and
         * LESS-THAN SIGN and GREATER-THAN SIGN
         * in a Swiss German, German, or French
         * layout.
         */
        public static final int SDL_SCANCODE_APPLICATION = 101;
        /**
         * < windows contextual menu, compose
         */
        public static final int SDL_SCANCODE_POWER = 102;
        /**
         * < The USB document says this is a status flag,
         * not a physical key - but some Mac keyboards
         * do have a power key.
         */
        public static final int SDL_SCANCODE_KP_EQUALS = 103;
        public static final int SDL_SCANCODE_F13 = 104;
        public static final int SDL_SCANCODE_F14 = 105;
        public static final int SDL_SCANCODE_F15 = 106;
        public static final int SDL_SCANCODE_F16 = 107;
        public static final int SDL_SCANCODE_F17 = 108;
        public static final int SDL_SCANCODE_F18 = 109;
        public static final int SDL_SCANCODE_F19 = 110;
        public static final int SDL_SCANCODE_F20 = 111;
        public static final int SDL_SCANCODE_F21 = 112;
        public static final int SDL_SCANCODE_F22 = 113;
        public static final int SDL_SCANCODE_F23 = 114;
        public static final int SDL_SCANCODE_F24 = 115;
        public static final int SDL_SCANCODE_EXECUTE = 116;
        public static final int SDL_SCANCODE_HELP = 117;
        /**
         * < AL Integrated Help Center
         */
        public static final int SDL_SCANCODE_MENU = 118;
        /**
         * < Menu (show menu)
         */
        public static final int SDL_SCANCODE_SELECT = 119;
        public static final int SDL_SCANCODE_STOP = 120;
        /**
         * < AC Stop
         */
        public static final int SDL_SCANCODE_AGAIN = 121;
        /**
         * < AC Redo/Repeat
         */
        public static final int SDL_SCANCODE_UNDO = 122;
        /**
         * < AC Undo
         */
        public static final int SDL_SCANCODE_CUT = 123;
        /**
         * < AC Cut
         */
        public static final int SDL_SCANCODE_COPY = 124;
        /**
         * < AC Copy
         */
        public static final int SDL_SCANCODE_PASTE = 125;
        /**
         * < AC Paste
         */
        public static final int SDL_SCANCODE_FIND = 126;
        /**
         * < AC Find
         */
        public static final int SDL_SCANCODE_MUTE = 127;
        public static final int SDL_SCANCODE_VOLUMEUP = 128;
        public static final int SDL_SCANCODE_VOLUMEDOWN = 129;
        /* not sure whether there's a reason to enable these */
        /*     SDL_SCANCODE_LOCKINGCAPSLOCK = 130,  */
        /*     SDL_SCANCODE_LOCKINGNUMLOCK = 131, */
        /*     SDL_SCANCODE_LOCKINGSCROLLLOCK = 132, */
        public static final int SDL_SCANCODE_KP_COMMA = 133;
        public static final int SDL_SCANCODE_KP_EQUALSAS400 = 134;

        public static final int SDL_SCANCODE_INTERNATIONAL1 = 135;
        /**
         * < used on Asian keyboards, see
         * footnotes in USB doc
         */
        public static final int SDL_SCANCODE_INTERNATIONAL2 = 136;
        public static final int SDL_SCANCODE_INTERNATIONAL3 = 137;
        /**
         * < Yen
         */
        public static final int SDL_SCANCODE_INTERNATIONAL4 = 138;
        public static final int SDL_SCANCODE_INTERNATIONAL5 = 139;
        public static final int SDL_SCANCODE_INTERNATIONAL6 = 140;
        public static final int SDL_SCANCODE_INTERNATIONAL7 = 141;
        public static final int SDL_SCANCODE_INTERNATIONAL8 = 142;
        public static final int SDL_SCANCODE_INTERNATIONAL9 = 143;
        public static final int SDL_SCANCODE_LANG1 = 144;
        /**
         * < Hangul/English toggle
         */
        public static final int SDL_SCANCODE_LANG2 = 145;
        /**
         * < Hanja conversion
         */
        public static final int SDL_SCANCODE_LANG3 = 146;
        /**
         * < Katakana
         */
        public static final int SDL_SCANCODE_LANG4 = 147;
        /**
         * < Hiragana
         */
        public static final int SDL_SCANCODE_LANG5 = 148;
        /**
         * < Zenkaku/Hankaku
         */
        public static final int SDL_SCANCODE_LANG6 = 149;
        /**
         * < reserved
         */
        public static final int SDL_SCANCODE_LANG7 = 150;
        /**
         * < reserved
         */
        public static final int SDL_SCANCODE_LANG8 = 151;
        /**
         * < reserved
         */
        public static final int SDL_SCANCODE_LANG9 = 152;
        /**
         * < reserved
         */

        public static final int SDL_SCANCODE_ALTERASE = 153;
        /**
         * < Erase-Eaze
         */
        public static final int SDL_SCANCODE_SYSREQ = 154;
        public static final int SDL_SCANCODE_CANCEL = 155;
        /**
         * < AC Cancel
         */
        public static final int SDL_SCANCODE_CLEAR = 156;
        public static final int SDL_SCANCODE_PRIOR = 157;
        public static final int SDL_SCANCODE_RETURN2 = 158;
        public static final int SDL_SCANCODE_SEPARATOR = 159;
        public static final int SDL_SCANCODE_OUT = 160;
        public static final int SDL_SCANCODE_OPER = 161;
        public static final int SDL_SCANCODE_CLEARAGAIN = 162;
        public static final int SDL_SCANCODE_CRSEL = 163;
        public static final int SDL_SCANCODE_EXSEL = 164;

        public static final int SDL_SCANCODE_KP_00 = 176;
        public static final int SDL_SCANCODE_KP_000 = 177;
        public static final int SDL_SCANCODE_THOUSANDSSEPARATOR = 178;
        public static final int SDL_SCANCODE_DECIMALSEPARATOR = 179;
        public static final int SDL_SCANCODE_CURRENCYUNIT = 180;
        public static final int SDL_SCANCODE_CURRENCYSUBUNIT = 181;
        public static final int SDL_SCANCODE_KP_LEFTPAREN = 182;
        public static final int SDL_SCANCODE_KP_RIGHTPAREN = 183;
        public static final int SDL_SCANCODE_KP_LEFTBRACE = 184;
        public static final int SDL_SCANCODE_KP_RIGHTBRACE = 185;
        public static final int SDL_SCANCODE_KP_TAB = 186;
        public static final int SDL_SCANCODE_KP_BACKSPACE = 187;
        public static final int SDL_SCANCODE_KP_A = 188;
        public static final int SDL_SCANCODE_KP_B = 189;
        public static final int SDL_SCANCODE_KP_C = 190;
        public static final int SDL_SCANCODE_KP_D = 191;
        public static final int SDL_SCANCODE_KP_E = 192;
        public static final int SDL_SCANCODE_KP_F = 193;
        public static final int SDL_SCANCODE_KP_XOR = 194;
        public static final int SDL_SCANCODE_KP_POWER = 195;
        public static final int SDL_SCANCODE_KP_PERCENT = 196;
        public static final int SDL_SCANCODE_KP_LESS = 197;
        public static final int SDL_SCANCODE_KP_GREATER = 198;
        public static final int SDL_SCANCODE_KP_AMPERSAND = 199;
        public static final int SDL_SCANCODE_KP_DBLAMPERSAND = 200;
        public static final int SDL_SCANCODE_KP_VERTICALBAR = 201;
        public static final int SDL_SCANCODE_KP_DBLVERTICALBAR = 202;
        public static final int SDL_SCANCODE_KP_COLON = 203;
        public static final int SDL_SCANCODE_KP_HASH = 204;
        public static final int SDL_SCANCODE_KP_SPACE = 205;
        public static final int SDL_SCANCODE_KP_AT = 206;
        public static final int SDL_SCANCODE_KP_EXCLAM = 207;
        public static final int SDL_SCANCODE_KP_MEMSTORE = 208;
        public static final int SDL_SCANCODE_KP_MEMRECALL = 209;
        public static final int SDL_SCANCODE_KP_MEMCLEAR = 210;
        public static final int SDL_SCANCODE_KP_MEMADD = 211;
        public static final int SDL_SCANCODE_KP_MEMSUBTRACT = 212;
        public static final int SDL_SCANCODE_KP_MEMMULTIPLY = 213;
        public static final int SDL_SCANCODE_KP_MEMDIVIDE = 214;
        public static final int SDL_SCANCODE_KP_PLUSMINUS = 215;
        public static final int SDL_SCANCODE_KP_CLEAR = 216;
        public static final int SDL_SCANCODE_KP_CLEARENTRY = 217;
        public static final int SDL_SCANCODE_KP_BINARY = 218;
        public static final int SDL_SCANCODE_KP_OCTAL = 219;
        public static final int SDL_SCANCODE_KP_DECIMAL = 220;
        public static final int SDL_SCANCODE_KP_HEXADECIMAL = 221;

        public static final int SDL_SCANCODE_LCTRL = 224;
        public static final int SDL_SCANCODE_LSHIFT = 225;
        public static final int SDL_SCANCODE_LALT = 226;
        /**
         * < alt, option
         */
        public static final int SDL_SCANCODE_LGUI = 227;
        /**
         * < windows, command (apple), meta
         */
        public static final int SDL_SCANCODE_RCTRL = 228;
        public static final int SDL_SCANCODE_RSHIFT = 229;
        public static final int SDL_SCANCODE_RALT = 230;
        /**
         * < alt gr, option
         */
        public static final int SDL_SCANCODE_RGUI = 231;
        /**
         * < windows, command (apple), meta
         */

        public static final int SDL_SCANCODE_MODE = 257; /**< I'm not sure if this is really not covered
     *   by any of the above, but since there's a
     *   special KMOD_MODE for it I'm adding it here
     */

        /* @} *//* Usage page 0x07 */

        /**
         * \name Usage page 0x0C
         * <p>
         * These values are mapped from usage page 0x0C (USB consumer page).
         * See https://usb.org/sites/default/files/hut1_2.pdf
         * <p>
         * There are way more keys in the spec than we can represent in the
         * current scancode range, so pick the ones that commonly come up in
         * real world usage.
         */
        /* @{ */

        public static final int SDL_SCANCODE_AUDIONEXT = 258;
        public static final int SDL_SCANCODE_AUDIOPREV = 259;
        public static final int SDL_SCANCODE_AUDIOSTOP = 260;
        public static final int SDL_SCANCODE_AUDIOPLAY = 261;
        public static final int SDL_SCANCODE_AUDIOMUTE = 262;
        public static final int SDL_SCANCODE_MEDIASELECT = 263;
        public static final int SDL_SCANCODE_WWW = 264;
        /**
         * < AL Internet Browser
         */
        public static final int SDL_SCANCODE_MAIL = 265;
        public static final int SDL_SCANCODE_CALCULATOR = 266;
        /**
         * < AL Calculator
         */
        public static final int SDL_SCANCODE_COMPUTER = 267;
        public static final int SDL_SCANCODE_AC_SEARCH = 268;
        /**
         * < AC Search
         */
        public static final int SDL_SCANCODE_AC_HOME = 269;
        /**
         * < AC Home
         */
        public static final int SDL_SCANCODE_AC_BACK = 270;
        /**
         * < AC Back
         */
        public static final int SDL_SCANCODE_AC_FORWARD = 271;
        /**
         * < AC Forward
         */
        public static final int SDL_SCANCODE_AC_STOP = 272;
        /**
         * < AC Stop
         */
        public static final int SDL_SCANCODE_AC_REFRESH = 273;
        /**
         * < AC Refresh
         */
        public static final int SDL_SCANCODE_AC_BOOKMARKS = 274; /**< AC Bookmarks */

        /* @} *//* Usage page 0x0C */

        /**
         * \name Walther keys
         * <p>
         * These are values that Christian Walther added (for mac keyboard?).
         */
        /* @{ */

        public static final int SDL_SCANCODE_BRIGHTNESSDOWN = 275;
        public static final int SDL_SCANCODE_BRIGHTNESSUP = 276;
        public static final int SDL_SCANCODE_DISPLAYSWITCH = 277;
        /**
         * < display mirroring/dual display
         * switch, video mode switch
         */
        public static final int SDL_SCANCODE_KBDILLUMTOGGLE = 278;
        public static final int SDL_SCANCODE_KBDILLUMDOWN = 279;
        public static final int SDL_SCANCODE_KBDILLUMUP = 280;
        public static final int SDL_SCANCODE_EJECT = 281;
        public static final int SDL_SCANCODE_SLEEP = 282;
        /**
         * < SC System Sleep
         */

        public static final int SDL_SCANCODE_APP1 = 283;
        public static final int SDL_SCANCODE_APP2 = 284;

        /* @} *//* Walther keys */

        /**
         * \name Usage page 0x0C (additional media keys)
         * <p>
         * These values are mapped from usage page 0x0C (USB consumer page).
         */
        /* @{ */

        public static final int SDL_SCANCODE_AUDIOREWIND = 285;
        public static final int SDL_SCANCODE_AUDIOFASTFORWARD = 286;

        /* @} *//* Usage page 0x0C (additional media keys) */

        /**
         * \name Mobile keys
         * <p>
         * These are values that are often used on mobile phones.
         */
        /* @{ */

        public static final int SDL_SCANCODE_SOFTLEFT = 287;
        /**
         * < Usually situated below the display on phones and
         * used as a multi-function feature key for selecting
         * a software defined function shown on the bottom left
         * of the display.
         */
        public static final int SDL_SCANCODE_SOFTRIGHT = 288;
        /**
         * < Usually situated below the display on phones and
         * used as a multi-function feature key for selecting
         * a software defined function shown on the bottom right
         * of the display.
         */
        public static final int SDL_SCANCODE_CALL = 289;
        /**
         * < Used for accepting phone calls.
         */
        public static final int SDL_SCANCODE_ENDCALL = 290;
        /**
         * < Used for rejecting phone calls.
         */

        /* @} *//* Mobile keys */

        /* Add any other keys here. */

        public static final int SDL_NUM_SCANCODES = 512; /**< not a key, just marks the number of scancodes
     for array bounds */
    }

    private static final class SDL_KeyCode
    {
        public static final int SDLK_UNKNOWN = 0;

        public static final int SDLK_RETURN = '\r';
        public static final int SDLK_ESCAPE = 0x1B;
        public static final int SDLK_BACKSPACE = '\b';
        public static final int SDLK_TAB = '\t';
        public static final int SDLK_SPACE = ' ';
        public static final int SDLK_EXCLAIM = '!';
        public static final int SDLK_QUOTEDBL = '"';
        public static final int SDLK_HASH = '#';
        public static final int SDLK_PERCENT = '%';
        public static final int SDLK_DOLLAR = '$';
        public static final int SDLK_AMPERSAND = '&';
        public static final int SDLK_QUOTE = '\'';
        public static final int SDLK_LEFTPAREN = '(';
        public static final int SDLK_RIGHTPAREN = ')';
        public static final int SDLK_ASTERISK = '*';
        public static final int SDLK_PLUS = '+';
        public static final int SDLK_COMMA = ',';
        public static final int SDLK_MINUS = '-';
        public static final int SDLK_PERIOD = '.';
        public static final int SDLK_SLASH = '/';
        public static final int SDLK_0 = '0';
        public static final int SDLK_1 = '1';
        public static final int SDLK_2 = '2';
        public static final int SDLK_3 = '3';
        public static final int SDLK_4 = '4';
        public static final int SDLK_5 = '5';
        public static final int SDLK_6 = '6';
        public static final int SDLK_7 = '7';
        public static final int SDLK_8 = '8';
        public static final int SDLK_9 = '9';
        public static final int SDLK_COLON = ':';
        public static final int SDLK_SEMICOLON = ';';
        public static final int SDLK_LESS = '<';
        public static final int SDLK_EQUALS = '=';
        public static final int SDLK_GREATER = '>';
        public static final int SDLK_QUESTION = '?';
        public static final int SDLK_AT = '@';

    /*
       Skip uppercase letters
     */

        public static final int SDLK_LEFTBRACKET = '[';
        public static final int SDLK_BACKSLASH = '\\';
        public static final int SDLK_RIGHTBRACKET = ']';
        public static final int SDLK_CARET = '^';
        public static final int SDLK_UNDERSCORE = '_';
        public static final int SDLK_BACKQUOTE = '`';
        public static final int SDLK_a = 'a';
        public static final int SDLK_b = 'b';
        public static final int SDLK_c = 'c';
        public static final int SDLK_d = 'd';
        public static final int SDLK_e = 'e';
        public static final int SDLK_f = 'f';
        public static final int SDLK_g = 'g';
        public static final int SDLK_h = 'h';
        public static final int SDLK_i = 'i';
        public static final int SDLK_j = 'j';
        public static final int SDLK_k = 'k';
        public static final int SDLK_l = 'l';
        public static final int SDLK_m = 'm';
        public static final int SDLK_n = 'n';
        public static final int SDLK_o = 'o';
        public static final int SDLK_p = 'p';
        public static final int SDLK_q = 'q';
        public static final int SDLK_r = 'r';
        public static final int SDLK_s = 's';
        public static final int SDLK_t = 't';
        public static final int SDLK_u = 'u';
        public static final int SDLK_v = 'v';
        public static final int SDLK_w = 'w';
        public static final int SDLK_x = 'x';
        public static final int SDLK_y = 'y';
        public static final int SDLK_z = 'z';

        public static final int SDLK_CAPSLOCK = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_CAPSLOCK);

        public static final int SDLK_F1 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_F1);
        public static final int SDLK_F2 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_F2);
        public static final int SDLK_F3 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_F3);
        public static final int SDLK_F4 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_F4);
        public static final int SDLK_F5 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_F5);
        public static final int SDLK_F6 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_F6);
        public static final int SDLK_F7 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_F7);
        public static final int SDLK_F8 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_F8);
        public static final int SDLK_F9 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_F9);
        public static final int SDLK_F10 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_F10);
        public static final int SDLK_F11 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_F11);
        public static final int SDLK_F12 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_F12);

        public static final int SDLK_PRINTSCREEN = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_PRINTSCREEN);
        public static final int SDLK_SCROLLLOCK = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_SCROLLLOCK);
        public static final int SDLK_PAUSE = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_PAUSE);
        public static final int SDLK_INSERT = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_INSERT);
        public static final int SDLK_HOME = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_HOME);
        public static final int SDLK_PAGEUP = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_PAGEUP);
        public static final int SDLK_DELETE = 0x7F;
        public static final int SDLK_END = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_END);
        public static final int SDLK_PAGEDOWN = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_PAGEDOWN);
        public static final int SDLK_RIGHT = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_RIGHT);
        public static final int SDLK_LEFT = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_LEFT);
        public static final int SDLK_DOWN = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_DOWN);
        public static final int SDLK_UP = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_UP);

        public static final int SDLK_NUMLOCKCLEAR = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_NUMLOCKCLEAR);
        public static final int SDLK_KP_DIVIDE = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_DIVIDE);
        public static final int SDLK_KP_MULTIPLY = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_MULTIPLY);
        public static final int SDLK_KP_MINUS = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_MINUS);
        public static final int SDLK_KP_PLUS = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_PLUS);
        public static final int SDLK_KP_ENTER = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_ENTER);
        public static final int SDLK_KP_1 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_1);
        public static final int SDLK_KP_2 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_2);
        public static final int SDLK_KP_3 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_3);
        public static final int SDLK_KP_4 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_4);
        public static final int SDLK_KP_5 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_5);
        public static final int SDLK_KP_6 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_6);
        public static final int SDLK_KP_7 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_7);
        public static final int SDLK_KP_8 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_8);
        public static final int SDLK_KP_9 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_9);
        public static final int SDLK_KP_0 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_0);
        public static final int SDLK_KP_PERIOD = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_PERIOD);

        public static final int SDLK_APPLICATION = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_APPLICATION);
        public static final int SDLK_POWER = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_POWER);
        public static final int SDLK_KP_EQUALS = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_EQUALS);
        public static final int SDLK_F13 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_F13);
        public static final int SDLK_F14 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_F14);
        public static final int SDLK_F15 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_F15);
        public static final int SDLK_F16 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_F16);
        public static final int SDLK_F17 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_F17);
        public static final int SDLK_F18 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_F18);
        public static final int SDLK_F19 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_F19);
        public static final int SDLK_F20 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_F20);
        public static final int SDLK_F21 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_F21);
        public static final int SDLK_F22 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_F22);
        public static final int SDLK_F23 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_F23);
        public static final int SDLK_F24 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_F24);
        public static final int SDLK_EXECUTE = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_EXECUTE);
        public static final int SDLK_HELP = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_HELP);
        public static final int SDLK_MENU = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_MENU);
        public static final int SDLK_SELECT = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_SELECT);
        public static final int SDLK_STOP = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_STOP);
        public static final int SDLK_AGAIN = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_AGAIN);
        public static final int SDLK_UNDO = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_UNDO);
        public static final int SDLK_CUT = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_CUT);
        public static final int SDLK_COPY = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_COPY);
        public static final int SDLK_PASTE = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_PASTE);
        public static final int SDLK_FIND = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_FIND);
        public static final int SDLK_MUTE = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_MUTE);
        public static final int SDLK_VOLUMEUP = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_VOLUMEUP);
        public static final int SDLK_VOLUMEDOWN = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_VOLUMEDOWN);
        public static final int SDLK_KP_COMMA = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_COMMA);
        public static final int SDLK_KP_EQUALSAS400 =
                SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_EQUALSAS400);

        public static final int SDLK_ALTERASE = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_ALTERASE);
        public static final int SDLK_SYSREQ = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_SYSREQ);
        public static final int SDLK_CANCEL = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_CANCEL);
        public static final int SDLK_CLEAR = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_CLEAR);
        public static final int SDLK_PRIOR = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_PRIOR);
        public static final int SDLK_RETURN2 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_RETURN2);
        public static final int SDLK_SEPARATOR = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_SEPARATOR);
        public static final int SDLK_OUT = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_OUT);
        public static final int SDLK_OPER = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_OPER);
        public static final int SDLK_CLEARAGAIN = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_CLEARAGAIN);
        public static final int SDLK_CRSEL = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_CRSEL);
        public static final int SDLK_EXSEL = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_EXSEL);

        public static final int SDLK_KP_00 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_00);
        public static final int SDLK_KP_000 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_000);
        public static final int SDLK_THOUSANDSSEPARATOR =
                SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_THOUSANDSSEPARATOR);
        public static final int SDLK_DECIMALSEPARATOR =
                SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_DECIMALSEPARATOR);
        public static final int SDLK_CURRENCYUNIT = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_CURRENCYUNIT);
        public static final int SDLK_CURRENCYSUBUNIT =
                SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_CURRENCYSUBUNIT);
        public static final int SDLK_KP_LEFTPAREN = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_LEFTPAREN);
        public static final int SDLK_KP_RIGHTPAREN = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_RIGHTPAREN);
        public static final int SDLK_KP_LEFTBRACE = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_LEFTBRACE);
        public static final int SDLK_KP_RIGHTBRACE = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_RIGHTBRACE);
        public static final int SDLK_KP_TAB = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_TAB);
        public static final int SDLK_KP_BACKSPACE = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_BACKSPACE);
        public static final int SDLK_KP_A = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_A);
        public static final int SDLK_KP_B = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_B);
        public static final int SDLK_KP_C = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_C);
        public static final int SDLK_KP_D = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_D);
        public static final int SDLK_KP_E = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_E);
        public static final int SDLK_KP_F = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_F);
        public static final int SDLK_KP_XOR = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_XOR);
        public static final int SDLK_KP_POWER = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_POWER);
        public static final int SDLK_KP_PERCENT = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_PERCENT);
        public static final int SDLK_KP_LESS = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_LESS);
        public static final int SDLK_KP_GREATER = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_GREATER);
        public static final int SDLK_KP_AMPERSAND = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_AMPERSAND);
        public static final int SDLK_KP_DBLAMPERSAND =
                SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_DBLAMPERSAND);
        public static final int SDLK_KP_VERTICALBAR =
                SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_VERTICALBAR);
        public static final int SDLK_KP_DBLVERTICALBAR =
                SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_DBLVERTICALBAR);
        public static final int SDLK_KP_COLON = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_COLON);
        public static final int SDLK_KP_HASH = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_HASH);
        public static final int SDLK_KP_SPACE = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_SPACE);
        public static final int SDLK_KP_AT = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_AT);
        public static final int SDLK_KP_EXCLAM = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_EXCLAM);
        public static final int SDLK_KP_MEMSTORE = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_MEMSTORE);
        public static final int SDLK_KP_MEMRECALL = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_MEMRECALL);
        public static final int SDLK_KP_MEMCLEAR = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_MEMCLEAR);
        public static final int SDLK_KP_MEMADD = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_MEMADD);
        public static final int SDLK_KP_MEMSUBTRACT =
                SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_MEMSUBTRACT);
        public static final int SDLK_KP_MEMMULTIPLY =
                SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_MEMMULTIPLY);
        public static final int SDLK_KP_MEMDIVIDE = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_MEMDIVIDE);
        public static final int SDLK_KP_PLUSMINUS = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_PLUSMINUS);
        public static final int SDLK_KP_CLEAR = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_CLEAR);
        public static final int SDLK_KP_CLEARENTRY = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_CLEARENTRY);
        public static final int SDLK_KP_BINARY = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_BINARY);
        public static final int SDLK_KP_OCTAL = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_OCTAL);
        public static final int SDLK_KP_DECIMAL = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_DECIMAL);
        public static final int SDLK_KP_HEXADECIMAL =
                SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KP_HEXADECIMAL);

        public static final int SDLK_LCTRL = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_LCTRL);
        public static final int SDLK_LSHIFT = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_LSHIFT);
        public static final int SDLK_LALT = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_LALT);
        public static final int SDLK_LGUI = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_LGUI);
        public static final int SDLK_RCTRL = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_RCTRL);
        public static final int SDLK_RSHIFT = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_RSHIFT);
        public static final int SDLK_RALT = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_RALT);
        public static final int SDLK_RGUI = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_RGUI);

        public static final int SDLK_MODE = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_MODE);

        public static final int SDLK_AUDIONEXT = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_AUDIONEXT);
        public static final int SDLK_AUDIOPREV = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_AUDIOPREV);
        public static final int SDLK_AUDIOSTOP = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_AUDIOSTOP);
        public static final int SDLK_AUDIOPLAY = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_AUDIOPLAY);
        public static final int SDLK_AUDIOMUTE = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_AUDIOMUTE);
        public static final int SDLK_MEDIASELECT = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_MEDIASELECT);
        public static final int SDLK_WWW = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_WWW);
        public static final int SDLK_MAIL = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_MAIL);
        public static final int SDLK_CALCULATOR = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_CALCULATOR);
        public static final int SDLK_COMPUTER = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_COMPUTER);
        public static final int SDLK_AC_SEARCH = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_AC_SEARCH);
        public static final int SDLK_AC_HOME = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_AC_HOME);
        public static final int SDLK_AC_BACK = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_AC_BACK);
        public static final int SDLK_AC_FORWARD = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_AC_FORWARD);
        public static final int SDLK_AC_STOP = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_AC_STOP);
        public static final int SDLK_AC_REFRESH = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_AC_REFRESH);
        public static final int SDLK_AC_BOOKMARKS = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_AC_BOOKMARKS);

        public static final int SDLK_BRIGHTNESSDOWN =
                SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_BRIGHTNESSDOWN);
        public static final int SDLK_BRIGHTNESSUP = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_BRIGHTNESSUP);
        public static final int SDLK_DISPLAYSWITCH = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_DISPLAYSWITCH);
        public static final int SDLK_KBDILLUMTOGGLE =
                SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KBDILLUMTOGGLE);
        public static final int SDLK_KBDILLUMDOWN = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KBDILLUMDOWN);
        public static final int SDLK_KBDILLUMUP = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_KBDILLUMUP);
        public static final int SDLK_EJECT = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_EJECT);
        public static final int SDLK_SLEEP = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_SLEEP);
        public static final int SDLK_APP1 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_APP1);
        public static final int SDLK_APP2 = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_APP2);

        public static final int SDLK_AUDIOREWIND = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_AUDIOREWIND);
        public static final int SDLK_AUDIOFASTFORWARD = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_AUDIOFASTFORWARD);

        public static final int SDLK_SOFTLEFT = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_SOFTLEFT);
        public static final int SDLK_SOFTRIGHT = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_SOFTRIGHT);
        public static final int SDLK_CALL = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_CALL);
        public static final int SDLK_ENDCALL = SDL_SCANCODE_TO_KEYCODE(SDL_Scancode.SDL_SCANCODE_ENDCALL);
    }


    private static final class SDL_Mouse
    {
        public static final int SDL_BUTTON_LEFT = 1;
        public static final int SDL_BUTTON_MIDDLE = 2;
        public static final int SDL_BUTTON_RIGHT = 3;
        public static final int SDL_BUTTON_X1 = 4;
        public static final int SDL_BUTTON_X2 = 5;
    }

    public static class KeyCodesSDL
    {
        public static final int K_NONE = SDL_KeyCode.SDLK_UNKNOWN;
        public static final int K_ESCAPE = SDL_KeyCode.SDLK_ESCAPE;
        public static final int K_1 = SDL_KeyCode.SDLK_1;
        public static final int K_2 = SDL_KeyCode.SDLK_2;
        public static final int K_3 = SDL_KeyCode.SDLK_3;
        public static final int K_4 = SDL_KeyCode.SDLK_4;
        public static final int K_5 = SDL_KeyCode.SDLK_5;
        public static final int K_6 = SDL_KeyCode.SDLK_6;
        public static final int K_7 = SDL_KeyCode.SDLK_7;
        public static final int K_8 = SDL_KeyCode.SDLK_8;
        public static final int K_9 = SDL_KeyCode.SDLK_9;
        public static final int K_0 = SDL_KeyCode.SDLK_0;
        public static final int K_MINUS = SDL_KeyCode.SDLK_MINUS;
        public static final int K_EQUALS = SDL_KeyCode.SDLK_EQUALS;
        public static final int K_BACKSPACE = SDL_KeyCode.SDLK_BACKSPACE;
        public static final int K_TAB = SDL_KeyCode.SDLK_TAB;
        public static final int K_Q = SDL_KeyCode.SDLK_q;
        public static final int K_W = SDL_KeyCode.SDLK_w;
        public static final int K_E = SDL_KeyCode.SDLK_e;
        public static final int K_R = SDL_KeyCode.SDLK_r;
        public static final int K_T = SDL_KeyCode.SDLK_t;
        public static final int K_Y = SDL_KeyCode.SDLK_y;
        public static final int K_U = SDL_KeyCode.SDLK_u;
        public static final int K_I = SDL_KeyCode.SDLK_i;
        public static final int K_O = SDL_KeyCode.SDLK_o;
        public static final int K_P = SDL_KeyCode.SDLK_p;
        public static final int K_LBRACKET = SDL_KeyCode.SDLK_LEFTBRACKET;
        public static final int K_RBRACKET = SDL_KeyCode.SDLK_RIGHTBRACKET;
        public static final int K_ENTER = SDL_KeyCode.SDLK_RETURN;
        public static final int K_CTRL = SDL_KeyCode.SDLK_LCTRL;
        public static final int K_A = SDL_KeyCode.SDLK_a;
        public static final int K_S = SDL_KeyCode.SDLK_s;
        public static final int K_D = SDL_KeyCode.SDLK_d;
        public static final int K_F = SDL_KeyCode.SDLK_f;
        public static final int K_G = SDL_KeyCode.SDLK_g;
        public static final int K_H = SDL_KeyCode.SDLK_h;
        public static final int K_J = SDL_KeyCode.SDLK_j;
        public static final int K_K = SDL_KeyCode.SDLK_k;
        public static final int K_L = SDL_KeyCode.SDLK_l;
        public static final int K_SEMICOLON = SDL_KeyCode.SDLK_SEMICOLON;
        public static final int K_APOSTROPHE = SDL_KeyCode.SDLK_QUOTE;
        public static final int K_GRAVE = SDL_KeyCode.SDLK_BACKQUOTE;
        public static final int K_SHIFT = SDL_KeyCode.SDLK_LSHIFT;
        public static final int K_BACKSLASH = SDL_KeyCode.SDLK_BACKSLASH;
        public static final int K_Z = SDL_KeyCode.SDLK_z;
        public static final int K_X = SDL_KeyCode.SDLK_x;
        public static final int K_C = SDL_KeyCode.SDLK_c;
        public static final int K_V = SDL_KeyCode.SDLK_v;
        public static final int K_B = SDL_KeyCode.SDLK_b;
        public static final int K_N = SDL_KeyCode.SDLK_n;
        public static final int K_M = SDL_KeyCode.SDLK_m;
        public static final int K_COMMA = SDL_KeyCode.SDLK_COMMA;
        public static final int K_PERIOD = SDL_KeyCode.SDLK_PERIOD;
        public static final int K_SLASH = SDL_KeyCode.SDLK_SLASH;
        public static final int K_RSHIFT = SDL_KeyCode.SDLK_RSHIFT;
        public static final int K_KP_STAR = SDL_KeyCode.SDLK_KP_MULTIPLY;
        public static final int K_ALT = SDL_KeyCode.SDLK_LALT;
        public static final int K_SPACE = SDL_KeyCode.SDLK_SPACE;
        public static final int K_CAPSLOCK = SDL_KeyCode.SDLK_CAPSLOCK;
        public static final int K_F1 = SDL_KeyCode.SDLK_F1;
        public static final int K_F2 = SDL_KeyCode.SDLK_F2;
        public static final int K_F3 = SDL_KeyCode.SDLK_F3;
        public static final int K_F4 = SDL_KeyCode.SDLK_F4;
        public static final int K_F5 = SDL_KeyCode.SDLK_F5;
        public static final int K_F6 = SDL_KeyCode.SDLK_F6;
        public static final int K_F7 = SDL_KeyCode.SDLK_F7;
        public static final int K_F8 = SDL_KeyCode.SDLK_F8;
        public static final int K_F9 = SDL_KeyCode.SDLK_F9;
        public static final int K_F10 = SDL_KeyCode.SDLK_F10;
        public static final int K_NUMLOCK = SDL_KeyCode.SDLK_NUMLOCKCLEAR;
        public static final int K_SCROLL = SDL_KeyCode.SDLK_SCROLLLOCK;
        public static final int K_KP_7 = SDL_KeyCode.SDLK_KP_7;
        public static final int K_KP_8 = SDL_KeyCode.SDLK_KP_8;
        public static final int K_KP_9 = SDL_KeyCode.SDLK_KP_9;
        public static final int K_KP_MINUS = SDL_KeyCode.SDLK_KP_MINUS;
        public static final int K_KP_4 = SDL_KeyCode.SDLK_KP_4;
        public static final int K_KP_5 = SDL_KeyCode.SDLK_KP_5;
        public static final int K_KP_6 = SDL_KeyCode.SDLK_KP_6;
        public static final int K_KP_PLUS = SDL_KeyCode.SDLK_KP_PLUS;
        public static final int K_KP_1 = SDL_KeyCode.SDLK_KP_1;
        public static final int K_KP_2 = SDL_KeyCode.SDLK_KP_2;
        public static final int K_KP_3 = SDL_KeyCode.SDLK_KP_3;
        public static final int K_KP_0 = SDL_KeyCode.SDLK_KP_0;
        public static final int K_KP_DOT = SDL_KeyCode.SDLK_KP_PERIOD;
        public static final int K_F11 = SDL_KeyCode.SDLK_F11;
        public static final int K_F12 = SDL_KeyCode.SDLK_F12;
        public static final int K_F13 = SDL_KeyCode.SDLK_F13;
        public static final int K_F14 = SDL_KeyCode.SDLK_F14;
        public static final int K_F15 = SDL_KeyCode.SDLK_F15;
//        public static final int K_KANA = SDL_KeyCode.;
//        public static final int K_CONVERT = SDL_KeyCode.;
//        public static final int K_NOCONVERT = SDL_KeyCode.;
//        public static final int K_YEN = SDL_KeyCode.;
        public static final int K_KP_EQUALS = SDL_KeyCode.SDLK_KP_EQUALS;
//        public static final int K_CIRCUMFLEX = SDL_KeyCode.;
        public static final int K_AT = SDL_KeyCode.SDLK_AT;
        public static final int K_COLON = SDL_KeyCode.SDLK_COLON;
//        public static final int K_UNDERLINE = SDL_KeyCode.;
//        public static final int K_KANJI = SDL_KeyCode.;
//        public static final int K_STOP = SDL_KeyCode.;
//        public static final int K_AX = SDL_KeyCode.;
//        public static final int K_UNLABELED = SDL_KeyCode.;
        public static final int K_KP_ENTER = SDL_KeyCode.SDLK_KP_ENTER;
        public static final int K_RCTRL = SDL_KeyCode.SDLK_RCTRL;
        public static final int K_KP_COMMA = SDL_KeyCode.SDLK_KP_COMMA;
        public static final int K_KP_SLASH = SDL_KeyCode.SDLK_KP_DIVIDE;
        public static final int K_PRINTSCREEN = SDL_KeyCode.SDLK_PRINTSCREEN;
        public static final int K_RALT = SDL_KeyCode.SDLK_RALT;
        public static final int K_PAUSE = SDL_KeyCode.SDLK_PAUSE;
        public static final int K_HOME = SDL_KeyCode.SDLK_HOME;
        public static final int K_UPARROW = SDL_KeyCode.SDLK_UP;
        public static final int K_PGUP = SDL_KeyCode.SDLK_PAGEUP;
        public static final int K_LEFTARROW = SDL_KeyCode.SDLK_LEFT;
        public static final int K_RIGHTARROW = SDL_KeyCode.SDLK_RIGHT;
        public static final int K_END = SDL_KeyCode.SDLK_END;
        public static final int K_DOWNARROW = SDL_KeyCode.SDLK_DOWN;
        public static final int K_PGDN = SDL_KeyCode.SDLK_PAGEDOWN;
        public static final int K_INS = SDL_KeyCode.SDLK_INSERT;
        public static final int K_DEL = SDL_KeyCode.SDLK_DELETE;
        public static final int K_LWIN = SDL_KeyCode.SDLK_LGUI;
        public static final int K_RWIN = SDL_KeyCode.SDLK_RGUI;
        public static final int K_APPS = SDL_KeyCode.SDLK_APPLICATION;
        public static final int K_POWER = SDL_KeyCode.SDLK_POWER;
        public static final int K_SLEEP = SDL_KeyCode.SDLK_SLEEP;
        public static final int K_MOUSE1 = SDL_Mouse.SDL_BUTTON_LEFT;
        public static final int K_MOUSE2 = SDL_Mouse.SDL_BUTTON_RIGHT;
        public static final int K_MOUSE3 = SDL_Mouse.SDL_BUTTON_MIDDLE;
//        public static final int K_MOUSE4 = SDL_KeyCode.;
//        public static final int K_MOUSE5 = SDL_KeyCode.;
//        public static final int K_MOUSE6 = SDL_KeyCode.;
//        public static final int K_MOUSE7 = SDL_KeyCode.;
//        public static final int K_MOUSE8 = SDL_KeyCode.;
        public static final int K_MWHEELDOWN = SDL_Mouse.SDL_BUTTON_X1;
        public static final int K_MWHEELUP = SDL_Mouse.SDL_BUTTON_X2;

        //karin: change to a/d
//        public static final int J_LEFT = K_LEFTARROW;
//        public static final int J_RIGHT = K_RIGHTARROW;
        public static final int J_LEFT = K_A;
        public static final int J_RIGHT = K_D;
        public static final int J_UP = K_W; // K_UPARROW;
        public static final int J_DOWN = K_S; // K_DOWNARROW;
    }

    ;


    public static class KeyCodes
    {
        public static int K_TAB;
        public static int K_ENTER;
        public static int K_ESCAPE;
        public static int K_SPACE;
        public static int K_BACKSPACE;
        public static int K_CAPSLOCK;
        public static int K_PAUSE;
        public static int K_UPARROW;
        public static int K_DOWNARROW;
        public static int K_LEFTARROW;
        public static int K_RIGHTARROW;
        public static int K_ALT;
        public static int K_CTRL;
        public static int K_SHIFT;
        public static int K_INS;
        public static int K_DEL;
        public static int K_PGDN;
        public static int K_PGUP;
        public static int K_HOME;
        public static int K_END;
        public static int K_F1;
        public static int K_F2;
        public static int K_F3;
        public static int K_F4;
        public static int K_F5;
        public static int K_F6;
        public static int K_F7;
        public static int K_F8;
        public static int K_F9;
        public static int K_F10;
        public static int K_F11;
        public static int K_F12;
        public static int K_MOUSE1;
        public static int K_MOUSE2;
        public static int K_MOUSE3;
        public static int K_MOUSE4;
        public static int K_MOUSE5;
        public static int K_MWHEELDOWN;
        public static int K_MWHEELUP;

        public static int J_LEFT;
        public static int J_RIGHT;
        public static int J_UP;
        public static int J_DOWN;

        public static int K_A;
        public static int K_B;
        public static int K_C;
        public static int K_D;
        public static int K_E;
        public static int K_F;
        public static int K_G;
        public static int K_H;
        public static int K_I;
        public static int K_J;
        public static int K_K;
        public static int K_L;
        public static int K_M;
        public static int K_N;
        public static int K_O;
        public static int K_P;
        public static int K_Q;
        public static int K_R;
        public static int K_S;
        public static int K_T;
        public static int K_U;
        public static int K_V;
        public static int K_W;
        public static int K_X;
        public static int K_Y;
        public static int K_Z;

        public static int K_0;
        public static int K_1;
        public static int K_2;
        public static int K_3;
        public static int K_4;
        public static int K_5;
        public static int K_6;
        public static int K_7;
        public static int K_8;
        public static int K_9;
    }

    ;

    public static void InitRTCWKeycodes()
    {
        InitKeycodes(KeyCodesRTCW.class);
    }

    public static void InitQ3Keycodes()
    {
        InitKeycodes(KeyCodesQ3.class);
    }

    public static void InitD3Keycodes()
    {
        InitKeycodes(KeyCodesD3.class);
    }

    public static void InitD3BFGKeycodes()
    {
        InitKeycodes(KeyCodesD3BFG.class);
    }

    public static void InitQ1Keycodes()
    {
        InitKeycodes(KeyCodesQ1.class);
    }

    public static void InitSDLKeycodes()
    {
        InitKeycodes(KeyCodesSDL.class);
    }

    public static void InitKeycodes(Class<?> clazz)
    {
        Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Using key map: " + clazz.getName());
        for (Field f : KeyCodes.class.getFields())
        {
            try
            {
                f.set(null, clazz.getField(f.getName()).get(null));
            } catch (Exception ignored) {
                try // else setup generic key codes
                {
                    f.set(null, KeyCodesGeneric.class.getField(f.getName()).get(null));
                } catch (Exception ignored2) { }
            }
        }
    }

    public static int GetRealKeyCode(int keycodeGeneric)
    {
        Field[] fields = KeyCodesGeneric.class.getFields();
        for (Field field : fields)
        {
            try
            {
                int key = (Integer) field.get(null);
                if(key == keycodeGeneric)
                {
                    String name = field.getName();
                    Field f = KeyCodes.class.getField(name);
                    Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Map virtual key: " + name + " = " + keycodeGeneric + " -> " + f.get(null));
                    return (Integer) f.get(null);
                }
            } catch (Exception ignored) {}
        }
        return keycodeGeneric;
    }

    public static int[] GetRealKeyCodes(int[] keycodeGeneric)
    {
        int[] codes = new int[keycodeGeneric.length];
        for (int i = 0; i < keycodeGeneric.length; i++)
            codes[i] = Q3EKeyCodes.GetRealKeyCode(keycodeGeneric[i]);
        return codes;
    }

    public static void ConvertRealKeyCodes(int[] codes)
    {
        for (int i = 0; i < codes.length; i++)
            codes[i] = Q3EKeyCodes.GetRealKeyCode(codes[i]);
    }


    public static int convertKeyCode(int keyCode, KeyEvent event)
    {
        switch (keyCode)
        {
            case KeyEvent.KEYCODE_FOCUS:
                return KeyCodes.K_F1;
            case KeyEvent.KEYCODE_VOLUME_DOWN:
                return KeyCodes.K_F2;
            case KeyEvent.KEYCODE_VOLUME_UP:
                return KeyCodes.K_F3;
            case KeyEvent.KEYCODE_DPAD_UP:
                return KeyCodes.K_UPARROW;
            case KeyEvent.KEYCODE_DPAD_DOWN:
                return KeyCodes.K_DOWNARROW;
            case KeyEvent.KEYCODE_DPAD_LEFT:
                return KeyCodes.K_LEFTARROW;
            case KeyEvent.KEYCODE_DPAD_RIGHT:
                return KeyCodes.K_RIGHTARROW;
            case KeyEvent.KEYCODE_DPAD_CENTER:
                return KeyCodes.K_CTRL;
            case KeyEvent.KEYCODE_ENTER:
                return KeyCodes.K_ENTER;
            case KeyEvent.KEYCODE_BACK:
                return KeyCodes.K_ESCAPE;
            case KeyEvent.KEYCODE_DEL:
                return KeyCodes.K_BACKSPACE;
            case KeyEvent.KEYCODE_ALT_LEFT:
            case KeyEvent.KEYCODE_ALT_RIGHT:
                return KeyCodes.K_ALT;
            case KeyEvent.KEYCODE_SHIFT_LEFT:
            case KeyEvent.KEYCODE_SHIFT_RIGHT:
                return KeyCodes.K_SHIFT;
            case KeyEvent.KEYCODE_CTRL_LEFT:
            case KeyEvent.KEYCODE_CTRL_RIGHT:
                return KeyCodes.K_CTRL;
            case KeyEvent.KEYCODE_INSERT:
                return KeyCodes.K_INS;
            case 122:
                return KeyCodes.K_HOME;
            case KeyEvent.KEYCODE_FORWARD_DEL:
                return KeyCodes.K_DEL;
            case 123:
                return KeyCodes.K_END;
            case KeyEvent.KEYCODE_ESCAPE:
                return KeyCodes.K_ESCAPE;
            case KeyEvent.KEYCODE_TAB:
                return KeyCodes.K_TAB;
            case KeyEvent.KEYCODE_F1:
                return KeyCodes.K_F1;
            case KeyEvent.KEYCODE_F2:
                return KeyCodes.K_F2;
            case KeyEvent.KEYCODE_F3:
                return KeyCodes.K_F3;
            case KeyEvent.KEYCODE_F4:
                return KeyCodes.K_F4;
            case KeyEvent.KEYCODE_F5:
                return KeyCodes.K_F5;
            case KeyEvent.KEYCODE_F6:
                return KeyCodes.K_F6;
            case KeyEvent.KEYCODE_F7:
                return KeyCodes.K_F7;
            case KeyEvent.KEYCODE_F8:
                return KeyCodes.K_F8;
            case KeyEvent.KEYCODE_F9:
                return KeyCodes.K_F9;
            case KeyEvent.KEYCODE_F10:
                return KeyCodes.K_F10;
            case KeyEvent.KEYCODE_F11:
                return KeyCodes.K_F11;
            case KeyEvent.KEYCODE_F12:
                return KeyCodes.K_F12;
            case KeyEvent.KEYCODE_CAPS_LOCK:
                return KeyCodes.K_CAPSLOCK;
            case KeyEvent.KEYCODE_PAGE_DOWN:
                return KeyCodes.K_PGDN;
            case KeyEvent.KEYCODE_PAGE_UP:
                return KeyCodes.K_PGUP;
            case KeyEvent.KEYCODE_BUTTON_A:
                return 'c';
            case KeyEvent.KEYCODE_BUTTON_B:
                return 'r';
            case KeyEvent.KEYCODE_BUTTON_X:
                if (Q3EUtils.isOuya) return KeyCodes.K_ENTER;//No enter button on ouya
                return KeyCodes.K_SPACE;//Why not?
            case KeyEvent.KEYCODE_BUTTON_Y:
                return 'f';//RTCW use
            //These buttons are not so popular
            case KeyEvent.KEYCODE_BUTTON_C:
                return 'a';//That's why here is a, nobody cares.
            case KeyEvent.KEYCODE_BUTTON_Z:
                return 'z';
            //--------------------------------
            case KeyEvent.KEYCODE_BUTTON_START:
                return KeyCodes.K_ESCAPE;
            case KeyEvent.KEYCODE_BUTTON_SELECT:
                return KeyCodes.K_ENTER;
            case KeyEvent.KEYCODE_MENU:
                if (Q3EUtils.isOuya) return KeyCodes.K_ESCAPE;
                break;
            case KeyEvent.KEYCODE_BUTTON_L2:
                return KeyCodes.K_MWHEELDOWN;
            case KeyEvent.KEYCODE_BUTTON_R2:
                return KeyCodes.K_MWHEELUP;
            case KeyEvent.KEYCODE_BUTTON_R1:
                return KeyCodes.K_MOUSE1;//Sometimes it is necessary
            case KeyEvent.KEYCODE_BUTTON_L1:
                if (Q3EUtils.isOuya) return KeyCodes.K_SPACE;
                return 'l';//dunno why
            case Q3EOuya.BUTTON_L3:
                return '[';
            case Q3EOuya.BUTTON_R3:
                return ']';

        }
        int uchar = event.getUnicodeChar(0);
        if ((uchar < 127) && (uchar != 0))
            return uchar;
        return keyCode % 95 + 32;//Magic
    }

    /*
    * generic key code integer -> KeyCodesGeneric's field name -> KeyCodes's field key code value
    if(KeyCodesGeneric.K_SOME == KeyCodesGeneric.getFields()[some].get())
    {
        return KeyCodes.getField(KeyCodesGeneric.getFields()[some].getName()).get()
    }
     */
    public static class KeyCodesGeneric
    {
        public static final int K_MOUSE1 = 187;
        public static final int K_MOUSE2 = 188;
        public static final int K_MOUSE3 = 189;
        public static final int K_MOUSE4 = 190;
        public static final int K_MOUSE5 = 191;
        public static final int K_MWHEELUP = 195;
        public static final int K_MWHEELDOWN = 196;

        public static final int K_A = 97;
        public static final int K_B = 98;
        public static final int K_C = 99;
        public static final int K_D = 100;
        public static final int K_E = 101;
        public static final int K_F = 102;
        public static final int K_G = 103;
        public static final int K_H = 104;
        public static final int K_I = 105;
        public static final int K_J = 106;
        public static final int K_K = 107;
        public static final int K_L = 108;
        public static final int K_M = 109;
        public static final int K_N = 110;
        public static final int K_O = 111;
        public static final int K_P = 112;
        public static final int K_Q = 113;
        public static final int K_R = 114;
        public static final int K_S = 115;
        public static final int K_T = 116;
        public static final int K_U = 117;
        public static final int K_V = 118;
        public static final int K_W = 119;
        public static final int K_X = 120;
        public static final int K_Y = 121;
        public static final int K_Z = 122;

        public static final int K_0 = 48;
        public static final int K_1 = 49;
        public static final int K_2 = 50;
        public static final int K_3 = 51;
        public static final int K_4 = 52;
        public static final int K_5 = 53;
        public static final int K_6 = 54;
        public static final int K_7 = 55;
        public static final int K_8 = 56;
        public static final int K_9 = 57;

        public static final int K_F1 = 149;
        public static final int K_F2 = 150;
        public static final int K_F3 = 151;
        public static final int K_F4 = 152;
        public static final int K_F5 = 153;
        public static final int K_F6 = 154;
        public static final int K_F7 = 155;
        public static final int K_F8 = 156;
        public static final int K_F9 = 157;
        public static final int K_F10 = 158;
        public static final int K_F11 = 159;
        public static final int K_F12 = 160;

        public static final int K_BACKSPACE = 127;
        public static final int K_TAB = 9;
        public static final int K_ENTER = 13;
        public static final int K_SHIFT = 142;
        public static final int K_CTRL = 141;
        public static final int K_ALT = 140;
        public static final int K_CAPSLOCK = 129;
        public static final int K_ESCAPE = 27;
        public static final int K_SPACE = 32;
        public static final int K_PGUP = 146;
        public static final int K_PGDN = 145;
        public static final int K_END = 148;
        public static final int K_HOME = 147;
        public static final int K_LEFTARROW = 135;
        public static final int K_UPARROW = 133;
        public static final int K_RIGHTARROW = 136;
        public static final int K_DOWNARROW = 134;
        public static final int K_INS = 143;
        public static final int K_DEL = 144;

        public static final int K_SEMICOLON = 59;
        public static final int K_EQUALS = 61;
        public static final int K_COMMA = 44;
        public static final int K_MINUS = 45;
        public static final int K_PERIOD = 46;
        public static final int K_SLASH = 47;
        public static final int K_GRAVE = 96;
        public static final int K_LBRACKET = 91;
        public static final int K_BACKSLASH = 92;
        public static final int K_RBRACKET = 93;
        public static final int K_APOSTROPHE = 39;

        public static final int J_LEFT = -'a';
        public static final int J_RIGHT = -'d';
        public static final int J_UP = -K_UPARROW;
        public static final int J_DOWN = -K_DOWNARROW;
    }

    public static final String K_WEAPONS_STR = "1,2,3,4,5,6,7,8,9,q,0";
}
