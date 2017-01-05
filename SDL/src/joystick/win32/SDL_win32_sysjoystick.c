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
#include "SDL_config.h"

#include "SDL_win32_sysjoystick.h"

typedef struct _SDL_JoystickInterface
{
	int         ( * JoystickInit)(void);
	const char* ( * JoystickName)(int index);
	int         ( * JoystickOpen)(SDL_Joystick *joystick);
	void        ( * JoystickUpdate)(SDL_Joystick *joystick);
	void        ( * JoystickClose)(SDL_Joystick *joystick);
	void        ( * JoystickQuit)(void);
} SDL_JoystickInterface;

int SDL_SYS_NULL_JoystickInit(void){ return 0; }
const char *SDL_SYS_NULL_JoystickName(int index){ return NULL; }
int SDL_SYS_NULL_JoystickOpen(SDL_Joystick *joystick){ return -1; }
void SDL_SYS_NULL_JoystickUpdate(SDL_Joystick *joystick){ }
void SDL_SYS_NULL_JoystickClose(SDL_Joystick *joystick){ }
void SDL_SYS_NULL_JoystickQuit(void){ }

#if SDL_JOYSTICK_WINMM
static const SDL_JoystickInterface SDL_MM_JoysticInterface =
{
	SDL_SYS_MM_JoystickInit,
	SDL_SYS_MM_JoystickName,
	SDL_SYS_MM_JoystickOpen,
	SDL_SYS_MM_JoystickUpdate,
	SDL_SYS_MM_JoystickClose,
	SDL_SYS_MM_JoystickQuit,
};
#endif

#if SDL_JOYSTICK_XINPUT
static const SDL_JoystickInterface SDL_XINPUT_JoysticInterface =
{
	SDL_SYS_XINPUT_JoystickInit,
	SDL_SYS_XINPUT_JoystickName,
	SDL_SYS_XINPUT_JoystickOpen,
	SDL_SYS_XINPUT_JoystickUpdate,
	SDL_SYS_XINPUT_JoystickClose,
	SDL_SYS_XINPUT_JoystickQuit,
};
#endif

static const SDL_JoystickInterface SDL_NULL_JoysticInterface =
{
	SDL_SYS_NULL_JoystickInit,
	SDL_SYS_NULL_JoystickName,
	SDL_SYS_NULL_JoystickOpen,
	SDL_SYS_NULL_JoystickUpdate,
	SDL_SYS_NULL_JoystickClose,
	SDL_SYS_NULL_JoystickQuit,
};

static const SDL_JoystickInterface* SDL_ActiveJoystickInterface = &SDL_NULL_JoysticInterface;

int SDL_SYS_JoystickInit(void)
{
	int NumJoysticks = 0;

	#if SDL_JOYSTICK_XINPUT
	if( 0 == NumJoysticks )
	{
		NumJoysticks = SDL_SYS_XINPUT_JoystickInit();
		if( NumJoysticks > 0 )
		{
			SDL_ActiveJoystickInterface = &SDL_XINPUT_JoysticInterface;
		}
	}
	#endif

	#if SDL_JOYSTICK_WINMM
	if( 0 == NumJoysticks )
	{
		NumJoysticks = SDL_SYS_MM_JoystickInit();
		if( NumJoysticks > 0 )
		{
			SDL_ActiveJoystickInterface = &SDL_XINPUT_JoysticInterface;
		}
	}
	#endif

	if( 0 == NumJoysticks )
	{
		SDL_ActiveJoystickInterface = &SDL_NULL_JoysticInterface;
	}
	
	return NumJoysticks;
}

const char *SDL_SYS_JoystickName(int index)
{
	return SDL_ActiveJoystickInterface->JoystickName( index );
}

int SDL_SYS_JoystickOpen(SDL_Joystick *joystick)
{
	return SDL_ActiveJoystickInterface->JoystickOpen( joystick );
}

void SDL_SYS_JoystickUpdate(SDL_Joystick *joystick)
{
	SDL_ActiveJoystickInterface->JoystickUpdate( joystick );
}

void SDL_SYS_JoystickClose(SDL_Joystick *joystick)
{
	SDL_ActiveJoystickInterface->JoystickClose( joystick );
}

void SDL_SYS_JoystickQuit(void)
{
	SDL_ActiveJoystickInterface->JoystickQuit();
}