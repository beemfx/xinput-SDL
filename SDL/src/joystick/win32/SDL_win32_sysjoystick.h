/*
SDL - Simple DirectMedia Layer
Copyright (C) 1997-2012 Sam Lantinga

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

Sam Lantinga
slouken@libsdl.org
*/

/** @file SDL_joystick.h
*  Include file for SDL joystick event handling
*/

#ifndef _SDL_win32_joystick_h
#define _SDL_win32_joystick_h

#include "../SDL_sysjoystick.h"

#if SDL_JOYSTICK_WINMM
extern int SDL_SYS_XINPUT_JoystickInit(void);
extern const char *SDL_SYS_XINPUT_JoystickName(int index);
extern int SDL_SYS_XINPUT_JoystickOpen(SDL_Joystick *joystick);
extern void SDL_SYS_XINPUT_JoystickUpdate(SDL_Joystick *joystick);
extern void SDL_SYS_XINPUT_JoystickClose(SDL_Joystick *joystick);
extern void SDL_SYS_XINPUT_JoystickQuit(void);
#endif

#if SDL_JOYSTICK_XINPUT
extern int SDL_SYS_MM_JoystickInit(void);
extern const char *SDL_SYS_MM_JoystickName(int index);
extern int SDL_SYS_MM_JoystickOpen(SDL_Joystick *joystick);
extern void SDL_SYS_MM_JoystickUpdate(SDL_Joystick *joystick);
extern void SDL_SYS_MM_JoystickClose(SDL_Joystick *joystick);
extern void SDL_SYS_MM_JoystickQuit(void);
#endif

extern int SDL_SYS_NULL_JoystickInit(void);
extern const char *SDL_SYS_NULL_JoystickName(int index);
extern int SDL_SYS_NULL_JoystickOpen(SDL_Joystick *joystick);
extern void SDL_SYS_NULL_JoystickUpdate(SDL_Joystick *joystick);
extern void SDL_SYS_NULL_JoystickClose(SDL_Joystick *joystick);
extern void SDL_SYS_NULL_JoystickQuit(void);

#endif /* _SDL_win32_joystick_h */