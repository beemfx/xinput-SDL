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

#if  SDL_JOYSTICK_XINPUT

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <xinput.h>

extern "C" {
#include "SDL_joystick.h"
#include "SDL_events.h"
#include "SDL_win32_sysjoystick.h"
#include "../SDL_joystick_c.h"
}

#define countof(b) (sizeof(b)/sizeof(0[(b)]))

enum class sdl_xi_button 
{
	A      , // A or X
	B      , // B or Circle
	X      , // X or Square
	Y      , // Y or Triangle
	L1     , // Left Bumper (LB)
	R1     , // Right Bumper (RB)	
	L2     , // Left Trigger (LT)
	R2     , // Right Trigger (RT)
	L3     , // Left Thumb (Stick)
	R3     , // Right Thumb (Stick)
	START  ,
	BACK   ,
	ACTUAL_BUTTON_COUNT,
	DUP    , // D-Pad
	DDOWN  , // D-Pad
	DLEFT  , // D-Pad
	DRIGHT , // D-Pad
	LUP    , // Left Stick Direction
	LDOWN  , // Left Stick Direction
	LLEFT  , // Left Stick Direction
	LRIGHT , // Left Stick Direction
	RUP    , // Right Stick Direction
	RDOWN  , // Right Stick Direction
	RLEFT  , // Right Stick Direction
	RRIGHT , // Right Stick Direction
	COUNT  ,
};

static const struct SDL_XInputButtonToKey
{
	sdl_xi_button Button;
	DWORD     Key;
}
SDL_XInput_ButtonToKeyTable[] =
{
	{ sdl_xi_button::DUP    , 'W' },
	{ sdl_xi_button::DDOWN  , 'S' },
	{ sdl_xi_button::DLEFT  , 'A' },
	{ sdl_xi_button::DRIGHT , 'D' },
	{ sdl_xi_button::START  , VK_RETURN },
	{ sdl_xi_button::BACK   , VK_ESCAPE },
	{ sdl_xi_button::L3     , VK_NEXT },
	{ sdl_xi_button::R3     , VK_PRIOR },
	{ sdl_xi_button::L1     , VK_OEM_4 },
	{ sdl_xi_button::R1     , VK_OEM_6 },
	{ sdl_xi_button::A      , VK_CONTROL },
	{ sdl_xi_button::B      , 'Z' },
	{ sdl_xi_button::X      , 'X' },
	{ sdl_xi_button::Y      , 'C' },
	{ sdl_xi_button::L2     , VK_OEM_COMMA },
	{ sdl_xi_button::R2     , VK_OEM_PERIOD },
	{ sdl_xi_button::LUP    , 'W' },
	{ sdl_xi_button::LDOWN  , 'S' },
	{ sdl_xi_button::LLEFT  , 'A' },
	{ sdl_xi_button::LRIGHT , 'D' },
	{ sdl_xi_button::RUP    , VK_NUMPAD8 },
	{ sdl_xi_button::RDOWN  , VK_NUMPAD2 },
	{ sdl_xi_button::RLEFT  , VK_NUMPAD4 },
	{ sdl_xi_button::RRIGHT , VK_NUMPAD6 },
};

class SDL_XInputHandler
{
private:

	DWORD m_ControllerIndex;
	bool  m_bIsInited:1;
	float m_RightStickAxis[2];
	float m_LeftStickAxis[2];
	bool  m_ButtonsDownThisFrame[static_cast<unsigned>(sdl_xi_button::COUNT)];
	bool  m_ButtonsDown[static_cast<unsigned>(sdl_xi_button::COUNT)];
	bool  m_ButtonsPressed[static_cast<unsigned>(sdl_xi_button::COUNT)];
	bool  m_ButtonsReleased[static_cast<unsigned>(sdl_xi_button::COUNT)];

public:

	SDL_XInputHandler()
	: m_bIsInited( false )
	{
		Deinit(); // Zeroes everything
	}

	void Init( DWORD ControllerIndex )
	{
		m_ControllerIndex = ControllerIndex;
		zero( &m_RightStickAxis );
		zero( &m_LeftStickAxis );
		zero( &m_ButtonsDownThisFrame );
		zero( &m_ButtonsDown );
		zero( &m_ButtonsPressed );
		zero( &m_ButtonsReleased );
		m_bIsInited = true;
	}

	void Deinit()
	{
		m_ControllerIndex = 0;
		zero( &m_RightStickAxis );
		zero( &m_LeftStickAxis );
		zero( &m_ButtonsDownThisFrame );
		zero( &m_ButtonsDown );
		zero( &m_ButtonsPressed );
		zero( &m_ButtonsReleased );
		m_bIsInited = false;
	}

	bool IsInited() const { return m_bIsInited; }

	template<typename T>static void zero( T* Item ){ memset( Item , 0 , sizeof(*Item) ); };

	const float* GetRightStickAxis() const { return m_RightStickAxis; }
	const float* GetLeftStickAxis() const { return m_LeftStickAxis; }

	bool IsDown( sdl_xi_button Button )
	{
		unsigned Index = static_cast<unsigned>(Button);
		if( 0 <= Index && Index < countof(m_ButtonsDown) )
		{
			return m_ButtonsDown[Index];
		}
		return false;
	}

	bool WasPressed( sdl_xi_button Button )
	{
		unsigned Index = static_cast<unsigned>(Button);
		if( 0 <= Index && Index < countof(m_ButtonsPressed) )
		{
			return m_ButtonsPressed[Index];
		}
		return false;
	}

	bool WasReleased( sdl_xi_button Button )
	{
		unsigned Index = static_cast<unsigned>(Button);
		if( 0 <= Index && Index < countof(m_ButtonsReleased) )
		{
			return m_ButtonsReleased[Index];
		}
		return false;
	}

	void Update()
	{
		if( IsInited() )
		{
			UpdateHeldButtons();
			UpdatePressesAndReleases();
		}
	}

private:

	bool IsDownThisFrame( sdl_xi_button Button )
	{
		unsigned Index = static_cast<unsigned>(Button);
		if( 0 <= Index && Index < countof(m_ButtonsDownThisFrame) )
		{
			return m_ButtonsDownThisFrame[Index];
		}
		return false;
	}

	void SetDownThisFrame( sdl_xi_button Button , bool bSet )
	{
		unsigned Index = static_cast<unsigned>(Button);
		if( 0 <= Index && Index < countof(m_ButtonsDownThisFrame) )
		{
			m_ButtonsDownThisFrame[Index] = bSet;
		}
	}

	void SetDown( sdl_xi_button Button , bool bSet )
	{
		unsigned Index = static_cast<unsigned>(Button);
		if( 0 <= Index && Index < countof(m_ButtonsDown) )
		{
			m_ButtonsDown[Index] = bSet;
		}
	}

	void SetPressed( sdl_xi_button Button , bool bSet )
	{
		unsigned Index = static_cast<unsigned>(Button);
		if( 0 <= Index && Index < countof(m_ButtonsPressed) )
		{
			m_ButtonsPressed[Index] = bSet;
		}
	}

	void SetReleased( sdl_xi_button Button , bool bSet )
	{
		unsigned Index = static_cast<unsigned>(Button);
		if( 0 <= Index && Index < countof(m_ButtonsReleased) )
		{
			m_ButtonsReleased[Index] = bSet;
		}
	}

	void UpdateHeldButtons()
	{
		if( !(0<= m_ControllerIndex && m_ControllerIndex < XUSER_MAX_COUNT ) )
		{
			return;
		}

		// Helper functions.
		auto IsBetween = []( const auto& v1 , const auto& min , const auto& max) -> bool{ return (v1>=min) && (v1<=max); };
		auto Clamp =[](const auto& v1, const auto& min, const auto& max)->const auto { return ( (v1)>(max)?(max):(v1)<(min)?(min):(v1) ); };

		XINPUT_STATE State;
		zero( &State );
		DWORD Res = XInputGetState( m_ControllerIndex , &State );

		bool bConnected = ERROR_SUCCESS == Res;

		zero( &m_RightStickAxis );
		zero( &m_LeftStickAxis );
		zero( &m_ButtonsDownThisFrame );



		if( bConnected )
		{
			//Get button states:
			//Regular buttons:
			if( 0 != (State.Gamepad.wButtons&XINPUT_GAMEPAD_DPAD_UP)        ){ SetDownThisFrame( sdl_xi_button::DUP , true ); }
			if( 0 != (State.Gamepad.wButtons&XINPUT_GAMEPAD_DPAD_DOWN)      ){ SetDownThisFrame( sdl_xi_button::DDOWN , true ); }
			if( 0 != (State.Gamepad.wButtons&XINPUT_GAMEPAD_DPAD_LEFT)      ){ SetDownThisFrame( sdl_xi_button::DLEFT , true ); }
			if( 0 != (State.Gamepad.wButtons&XINPUT_GAMEPAD_DPAD_RIGHT)     ){ SetDownThisFrame( sdl_xi_button::DRIGHT , true ); }
			if( 0 != (State.Gamepad.wButtons&XINPUT_GAMEPAD_START)          ){ SetDownThisFrame( sdl_xi_button::START , true ); }
			if( 0 != (State.Gamepad.wButtons&XINPUT_GAMEPAD_BACK)           ){ SetDownThisFrame( sdl_xi_button::BACK , true ); }
			if( 0 != (State.Gamepad.wButtons&XINPUT_GAMEPAD_LEFT_THUMB)     ){ SetDownThisFrame( sdl_xi_button::L3 , true ); }
			if( 0 != (State.Gamepad.wButtons&XINPUT_GAMEPAD_RIGHT_THUMB)    ){ SetDownThisFrame( sdl_xi_button::R3 , true ); }
			if( 0 != (State.Gamepad.wButtons&XINPUT_GAMEPAD_LEFT_SHOULDER)  ){ SetDownThisFrame( sdl_xi_button::L1 , true ); }
			if( 0 != (State.Gamepad.wButtons&XINPUT_GAMEPAD_RIGHT_SHOULDER) ){ SetDownThisFrame( sdl_xi_button::R1 , true ); }
			if( 0 != (State.Gamepad.wButtons&XINPUT_GAMEPAD_A)              ){ SetDownThisFrame( sdl_xi_button::A , true ); }
			if( 0 != (State.Gamepad.wButtons&XINPUT_GAMEPAD_B)              ){ SetDownThisFrame( sdl_xi_button::B , true ); }
			if( 0 != (State.Gamepad.wButtons&XINPUT_GAMEPAD_X)              ){ SetDownThisFrame( sdl_xi_button::X , true ); }
			if( 0 != (State.Gamepad.wButtons&XINPUT_GAMEPAD_Y)              ){ SetDownThisFrame( sdl_xi_button::Y , true ); }
			//Trigger buttons
			if( State.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD  ){ SetDownThisFrame( sdl_xi_button::L2 , true ); }
			if( State.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD ){ SetDownThisFrame( sdl_xi_button::R2 , true ); }
			//Left stick buttons:
			if( State.Gamepad.sThumbLX >  XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ){ SetDownThisFrame( sdl_xi_button::LRIGHT , true ); }
			if( State.Gamepad.sThumbLX < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ){ SetDownThisFrame( sdl_xi_button::LLEFT , true ); }
			if( State.Gamepad.sThumbLY >  XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ){ SetDownThisFrame( sdl_xi_button::LUP , true ); }
			if( State.Gamepad.sThumbLY < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ){ SetDownThisFrame( sdl_xi_button::LDOWN , true ); }
			//Right stick buttons:
			if( State.Gamepad.sThumbRX >  XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE ){ SetDownThisFrame( sdl_xi_button::RRIGHT , true ); }
			if( State.Gamepad.sThumbRX < -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE ){ SetDownThisFrame( sdl_xi_button::RLEFT , true ); }
			if( State.Gamepad.sThumbRY >  XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE ){ SetDownThisFrame( sdl_xi_button::RUP , true ); }
			if( State.Gamepad.sThumbRY < -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE ){ SetDownThisFrame( sdl_xi_button::RDOWN , true ); }

			m_RightStickAxis[0] = Clamp(State.Gamepad.sThumbRX/32767.f,-1.f,1.f);
			m_RightStickAxis[1] = Clamp(State.Gamepad.sThumbRY/32767.f,-1.f,1.f);

			m_LeftStickAxis[0] = Clamp(State.Gamepad.sThumbLX/32767.f,-1.f,1.f);
			m_LeftStickAxis[1] = Clamp(State.Gamepad.sThumbLY/32767.f,-1.f,1.f);

			if( IsBetween( State.Gamepad.sThumbRX, -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE , XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE ) )
			{
				m_RightStickAxis[0] = 0;
			}
			if( IsBetween( State.Gamepad.sThumbRY, -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE , XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE ) )
			{
				m_RightStickAxis[1] = 0;
			}
			if( IsBetween( State.Gamepad.sThumbLX, -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE , XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ) )
			{
				m_LeftStickAxis[0] = 0;
			}
			if( IsBetween( State.Gamepad.sThumbLY, -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE , XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ) )
			{
				m_LeftStickAxis[1] = 0;
			}
		}
	}

	void UpdatePressesAndReleases()
	{
		for( unsigned ButtonIndex = 0; ButtonIndex < static_cast<unsigned>(sdl_xi_button::COUNT); ButtonIndex++ )
		{
			sdl_xi_button Button = static_cast<sdl_xi_button>(ButtonIndex);
			bool bHeld = IsDownThisFrame( Button );

			if( bHeld )
			{
				//If the button wasn't down, set as pressed.
				SetPressed( Button , !IsDown(Button) );
				SetReleased( Button , false );
				SetDown( Button , true );
			}
			else
			{
				//If the button was already down, set the deactivated flags...
				SetReleased( Button , IsDown(Button) );
				SetDown( Button , false );
				SetPressed( Button , false );
			}
		}
	}
};

static SDL_XInputHandler SDL_XInputHandlers[XUSER_MAX_COUNT];


int SDL_SYS_XINPUT_JoystickInit(void)
{
#if(_WIN32_WINNT >= _WIN32_WINNT_WIN8)
	XInputEnable( TRUE );
#endif

	XINPUT_CAPABILITIES Caps;
	memset( &Caps , 0 , sizeof(Caps) );
	for( DWORD i=0; i<XUSER_MAX_COUNT; i++ )
	{
		DWORD Res = XInputGetCapabilities( 0 , XINPUT_FLAG_GAMEPAD , &Caps );
		if( Res == ERROR_DEVICE_NOT_CONNECTED )
		{
			break;
		}
		SDL_numjoysticks++;
	}

	return SDL_numjoysticks;
}

const char *SDL_SYS_XINPUT_JoystickName( int index )
{
	switch( index )
	{
		case 0: return "XInput Device 0";
		case 1: return "XInput Device 1";
		case 2: return "XInput Device 2";
		case 3: return "XInput Device 3";
	}

	return nullptr;
}

/* Function to open a joystick for use.
The joystick to open is specified by the index field of the joystick.
This should fill the nbuttons and naxes fields of the joystick structure.
It returns 0, or -1 if there is an error.
*/
int SDL_SYS_XINPUT_JoystickOpen(SDL_Joystick *joystick)
{
	if( joystick && 0 <= joystick->index && joystick->index < countof(SDL_XInputHandlers) )
	{
		joystick->naxes = 4;
		joystick->nbuttons = static_cast<int>(sdl_xi_button::ACTUAL_BUTTON_COUNT);
		SDL_XInputHandlers[joystick->index].Init(joystick->index);
	}
	else
	{
		return -1;
	}

	return 0;
}

/* Function to update the state of a joystick - called as a device poll.
* This function shouldn't update the joystick structure directly,
* but instead should call SDL_PrivateJoystick*() to deliver events
* and update joystick device state.
*/
void SDL_SYS_XINPUT_JoystickUpdate(SDL_Joystick *joystick)
{	
	if( joystick && 0 <= joystick->index && joystick->index < countof(SDL_XInputHandlers) )
	{
		SDL_XInputHandler& Handler = SDL_XInputHandlers[joystick->index];
		Handler.Update();

		//SDL_Private
		Sint16 MainAxisX = static_cast<Sint16>(Handler.GetLeftStickAxis()[0]*32767.f);
		Sint16 MainAxisY = -static_cast<Sint16>(Handler.GetLeftStickAxis()[1]*32767.f);

		if( Handler.IsDown( sdl_xi_button::DUP ) )
		{
			MainAxisY = -32768;
		}

		if( Handler.IsDown( sdl_xi_button::DDOWN ) )
		{
			MainAxisY = 32767;
		}

		if( Handler.IsDown( sdl_xi_button::DLEFT ) )
		{
			MainAxisX = -32768;
		}

		if( Handler.IsDown( sdl_xi_button::DRIGHT ) )
		{
			MainAxisX = 32767;
		}

		SDL_PrivateJoystickAxis( joystick , 0 , MainAxisX );
		SDL_PrivateJoystickAxis( joystick , 1 , MainAxisY );
		SDL_PrivateJoystickAxis( joystick , 2 , static_cast<Sint16>(Handler.GetRightStickAxis()[0]*32767.f) );
		SDL_PrivateJoystickAxis( joystick , 3 , -static_cast<Sint16>(Handler.GetRightStickAxis()[1]*32767.f) );

		for( const SDL_XInputButtonToKey& Mapping : SDL_XInput_ButtonToKeyTable )
		{
			if( Handler.WasPressed(Mapping.Button) )
			{
				SDL_PrivateJoystickButton( joystick , static_cast<Uint8>(Mapping.Button) , SDL_PRESSED );
			}

			if( Handler.WasReleased(Mapping.Button) )
			{
				SDL_PrivateJoystickButton( joystick , static_cast<Uint8>(Mapping.Button) , SDL_RELEASED );
			}
		}
	}
}

/* Function to close a joystick after use */
void SDL_SYS_XINPUT_JoystickClose(SDL_Joystick *joystick)
{
	if( joystick && 0 <= joystick->index && joystick->index < countof(SDL_XInputHandlers) )
	{
		SDL_XInputHandlers[joystick->index].Deinit();
	}
}

/* Function to perform any system-specific joystick related cleanup */
void SDL_SYS_XINPUT_JoystickQuit(void)
{
#if(_WIN32_WINNT >= _WIN32_WINNT_WIN8)
	XInputEnable( FALSE );
#endif
}

#undef countof

#endif
