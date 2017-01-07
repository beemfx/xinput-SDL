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
#include "../../events/SDL_events_c.h"
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

enum class sdl_xi_mode
{
	GAMEPAD,
	GAMEPAD_ALWAYS,
	KEYMAP,
	KEYMAP_ALWAYS,
};

static struct SDL_XInputButtonToKey
{
	const sdl_xi_button Button;
	SDLKey              Key;
}
SDL_XInput_ButtonToKeyTable[] =
{
	{ sdl_xi_button::DUP    , SDLK_w },
	{ sdl_xi_button::DDOWN  , SDLK_s },
	{ sdl_xi_button::DLEFT  , SDLK_a },
	{ sdl_xi_button::DRIGHT , SDLK_d },
	{ sdl_xi_button::START  , SDLK_RETURN },
	{ sdl_xi_button::BACK   , SDLK_ESCAPE },
	{ sdl_xi_button::L3     , SDLK_PAGEUP },
	{ sdl_xi_button::R3     , SDLK_PAGEDOWN },
	{ sdl_xi_button::L1     , SDLK_LEFTBRACKET },
	{ sdl_xi_button::R1     , SDLK_RIGHTBRACKET },
	{ sdl_xi_button::A      , SDLK_LCTRL },
	{ sdl_xi_button::B      , SDLK_z },
	{ sdl_xi_button::X      , SDLK_x },
	{ sdl_xi_button::Y      , SDLK_c },
	{ sdl_xi_button::L2     , SDLK_COMMA },
	{ sdl_xi_button::R2     , SDLK_PERIOD },
	{ sdl_xi_button::LUP    , SDLK_w },
	{ sdl_xi_button::LDOWN  , SDLK_s },
	{ sdl_xi_button::LLEFT  , SDLK_a },
	{ sdl_xi_button::LRIGHT , SDLK_d },
	{ sdl_xi_button::RUP    , SDLK_KP8 },
	{ sdl_xi_button::RDOWN  , SDLK_KP2 },
	{ sdl_xi_button::RLEFT  , SDLK_KP4 },
	{ sdl_xi_button::RRIGHT , SDLK_KP6 },
};

static sdl_xi_mode SDL_XINPUT_Config_Mode = sdl_xi_mode::GAMEPAD;

static SDL_keysym* SDL_XINPUT_TranslateButton( SDLKey Key , SDL_keysym* KeySm )
{
	if( KeySm )
	{
		KeySm->scancode = 0;
		KeySm->mod = KMOD_NONE;
		KeySm->unicode = 0;
		KeySm->sym = Key;
	}

	return KeySm;
}

static sdl_xi_button SDL_XINPUT_StringToButton( const char* String )
{
	auto Equals =[]( const char* Left , const char* Right ) -> bool
	{
		return !_stricmp( Left , Right );
	};

	if( false )
	{
	}
#define HANDLE_ITEM( _name_ ) else if( Equals( String , #_name_ ) ){ return sdl_xi_button::_name_; }
		HANDLE_ITEM( A      )
		HANDLE_ITEM( B      )
		HANDLE_ITEM( X      )
		HANDLE_ITEM( Y      )
		HANDLE_ITEM( L1     )
		HANDLE_ITEM( R1     )
		HANDLE_ITEM( L2     )
		HANDLE_ITEM( R2     )
		HANDLE_ITEM( L3     )
		HANDLE_ITEM( R3     )
		HANDLE_ITEM( START  )
		HANDLE_ITEM( BACK   )
		HANDLE_ITEM( DUP    )
		HANDLE_ITEM( DDOWN  )
		HANDLE_ITEM( DLEFT  )
		HANDLE_ITEM( DRIGHT )
		HANDLE_ITEM( LUP    )
		HANDLE_ITEM( LDOWN  )
		HANDLE_ITEM( LLEFT  )
		HANDLE_ITEM( LRIGHT )
		HANDLE_ITEM( RUP    )
		HANDLE_ITEM( RDOWN  )
		HANDLE_ITEM( RLEFT  )
		HANDLE_ITEM( RRIGHT )
#undef HANDLE_ITEM

	return sdl_xi_button::COUNT;
}

static SDLKey SDL_XINPUT_StringToKey( const char* String )
{
	auto Equals =[]( const char* Left , const char* Right ) -> bool
	{
		return !_stricmp( Left , Right );
	};

	for( int i=0; i<SDLK_LAST; i++ )
	{
		if( Equals( String , SDL_GetKeyName(static_cast<SDLKey>(i)) ) )
		{
			return static_cast<SDLKey>(i);
		}
	}

	return SDLK_LAST;
}

static void SDL_XINPUT_SetConfigItem( const char* Item , const char* Value )
{
	if( Item[0] == '\0' )
	{
		return;
	}

	auto Equals =[]( const char* Left , const char* Right ) -> bool
	{
		return !_stricmp( Left , Right );
	};

	if( Equals( Item , "mode" ) )
	{
		if( Equals( Value , "GAMEPAD" ) )SDL_XINPUT_Config_Mode = sdl_xi_mode::GAMEPAD;
		else if( Equals( Value , "GAMEPAD_ALWAYS" ) )SDL_XINPUT_Config_Mode = sdl_xi_mode::GAMEPAD_ALWAYS;
		else if( Equals( Value , "KEYMAP" ) )SDL_XINPUT_Config_Mode = sdl_xi_mode::KEYMAP;
		else if( Equals( Value , "KEYMAP_ALWAYS" ) )SDL_XINPUT_Config_Mode = sdl_xi_mode::KEYMAP_ALWAYS;
	}
	else
	{
		sdl_xi_button Button = SDL_XINPUT_StringToButton( Item );
		SDLKey Key = SDL_XINPUT_StringToKey( Value );
		if( Button != sdl_xi_button::COUNT )
		{
			for( SDL_XInputButtonToKey& Pair : SDL_XInput_ButtonToKeyTable )
			{
				if( Pair.Button == Button )
				{
					Pair.Key = Key;
					break;
				}
			}
		}
	}
}

static void SDL_XINPUT_InitConfig()
{
	// Get the config filename (we want it in the directory of the applciation
	// not whatever the current working directory is).
	WCHAR Filename[1024];
	GetModuleFileNameW( GetModuleHandleW( NULL ) , Filename , countof(Filename) );
	Filename[countof(Filename)-1] = '\0';

	size_t FilenameLen = wcslen( Filename );

	for( size_t Search = FilenameLen-1; Search > 0; Search-- )
	{
		if( Filename[Search] == '\\' || Filename[Search] == '//' )
		{
			Filename[Search+1] = '\0'; // Just in case.
			break;
		}
	}

	wcscat( Filename , L"xinputsdl.conf" );

	FILE* InFile = _wfopen( Filename , L"r" );
	if( InFile )
	{
		char Line[256];
		while( fgets( Line , countof(Line) , InFile ) != nullptr )
		{
			Line[countof(Line)-1] = '\0'; // Just in case.

			char Button[256];
			size_t ButtonPos = 0;
			char Key[256];
			size_t KeyPos = 0;

			Button[0] = '\0';
			Key[0] = '\0';

			auto AppendButton = [&Button,&ButtonPos]( char c ) -> void
			{
				if( ButtonPos < (countof(Button)-1) )
				{
					Button[ButtonPos] = c;
					ButtonPos++;
					Button[ButtonPos] = '\0';
				}
			};

			auto AppendKey = [&Key,&KeyPos]( char c ) -> void
			{
				if( KeyPos < (countof(Key)-1) )
				{
					Key[KeyPos] = c;
					KeyPos++;
					Key[KeyPos] = '\0';
				}
			};

			auto IsWhitespace = []( char c ) -> bool
			{
				return c == ' ' || c == '\r' || c == '\t' || c == '\n';
			};

			size_t LineLen = strlen( Line );

			bool bReadingKey = false;
			bool bReadingName = false;
			bool bInQuote = false;
			bool bIsComment = false;

			bReadingKey = true;

			for( size_t i=0; i<LineLen; i++ )
			{
				char c = Line[i];

				if( bIsComment )
				{
					// Do nothing
				}
				else if( c == ';' && !bInQuote )
				{
					bReadingKey = false;
					bReadingName = false;
					bIsComment = true;
				}
				else if( c == '=' && !bInQuote )
				{
					if( bReadingKey )
					{
						bReadingKey = false;
						bReadingName = true;
					}
				}
				else if( bReadingName && c == '"' )
				{
					bInQuote = !bInQuote;
					if( !bInQuote )
					{
						bReadingName = false;
					}
				}
				else if( bReadingName && bInQuote )
				{
					AppendKey( c );
				}
				else if( bReadingKey && !IsWhitespace(c) )
				{
					AppendButton( c );
				}
			}

			SDL_XINPUT_SetConfigItem( Button , Key );

			// char TestMessage[1024];
			// sprintf( TestMessage , "%s\n\"%s\" = \"%s\"" , Line , Button , Key );
			// MessageBoxA( nullptr , TestMessage , "SDL Test" , MB_OK );

		}
		// fwprintf( OutFile , L"The file is %s.\n" , Filename );
		// GetCurrentDirectoryW( countof(Filename) , Filename );
		// fwprintf( OutFile , L"The directory is %s.\n" , Filename );
		fclose( InFile );
	}
}

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

	SDL_XINPUT_InitConfig();

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

	if( SDL_XINPUT_Config_Mode == sdl_xi_mode::GAMEPAD_ALWAYS || SDL_XINPUT_Config_Mode == sdl_xi_mode::KEYMAP_ALWAYS )
	{
		SDL_numjoysticks = XUSER_MAX_COUNT;
	}

	if( 0 == SDL_numjoysticks )
	{
#if(_WIN32_WINNT >= _WIN32_WINNT_WIN8)
		XInputEnable( FALSE );
#endif
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
	bool bMapToKeyboard = (SDL_XINPUT_Config_Mode == sdl_xi_mode::KEYMAP || SDL_XINPUT_Config_Mode == sdl_xi_mode::KEYMAP_ALWAYS);

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
				if( !bMapToKeyboard )
				{
					SDL_PrivateJoystickButton( joystick , static_cast<Uint8>(Mapping.Button) , SDL_PRESSED );
				}
				else
				{
					if( Mapping.Key != SDLK_LAST )
					{
						SDL_keysym KeySm;
						SDL_PrivateKeyboard( SDL_PRESSED , SDL_XINPUT_TranslateButton( Mapping.Key , &KeySm ) );
					}
				}
			}

			if( Handler.WasReleased(Mapping.Button) )
			{
				if( !bMapToKeyboard )
				{
					SDL_PrivateJoystickButton( joystick , static_cast<Uint8>(Mapping.Button) , SDL_RELEASED );
				}
				else
				{
					if( Mapping.Key != SDLK_LAST )
					{
						SDL_keysym KeySm;
						SDL_PrivateKeyboard( SDL_RELEASED , SDL_XINPUT_TranslateButton( Mapping.Key , &KeySm ) );
					}
				}
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
