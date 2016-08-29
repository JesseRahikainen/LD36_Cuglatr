/*
A game programmed for Ludum Dare 36.
Done completely from scratch in 48 hours.
Theme: Ancient Technology
*/

#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <conio.h>
#include <Windows.h>
#include <stdbool.h>
#include <stdio.h>
#include <varargs.h>

#include "stretchy_buffer.h"

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 50

#define BASE_SKILL_POINTS 48
#define MIN_SKILL_POINTS 36

const WORD BG_WHITE = BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY;
const WORD BG_BLACK = 0;
const WORD BG_GREY = BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED;
const WORD BG_BROWN = BACKGROUND_RED | BACKGROUND_GREEN;
const WORD BG_YELLOW = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY;
const WORD BG_MAGENTA = BACKGROUND_RED | BACKGROUND_BLUE | BACKGROUND_INTENSITY;
const WORD BG_PURPLE = BACKGROUND_RED | BACKGROUND_BLUE;
const WORD BG_DARK_CYAN = BACKGROUND_BLUE | BACKGROUND_GREEN;
const WORD BG_CYAN = BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_INTENSITY;

const WORD FG_WHITE = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY;
const WORD FG_BLACK = 0;
const WORD FG_GREY = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
const WORD FG_BROWN = FOREGROUND_RED | FOREGROUND_GREEN;
const WORD FG_YELLOW = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
const WORD FG_MAGENTA = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
const WORD FG_PURPLE = FOREGROUND_RED | FOREGROUND_BLUE;
const WORD FG_DARK_CYAN = FOREGROUND_BLUE | FOREGROUND_GREEN;
const WORD FG_CYAN = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
const WORD FG_RED = FOREGROUND_RED | FOREGROUND_INTENSITY;
const WORD FG_MAROON = FOREGROUND_RED;
const WORD FG_GREEN = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
const WORD FG_DARK_GREEN = FOREGROUND_GREEN;

// ═  205
// ║  186
// ╗  187
// ╚  200
// ╝  188
// ╔  201
// ╦  203
// ╩  202
// ╠  204
// ╣  185
// ╬  206
// /n 10
// _  32

typedef enum {
	G_KNIFE,
	G_PISTOL,
	G_LANTERN,
	G_RATION,
	G_ROPE,
	G_SLING,
	G_TOWEL,
	G_LIGHT_ARMOR,
	G_RIFLE,
	G_FLASHLIGHT,
	G_HACKING_DEVICE,
	G_SWORD,
	G_MONEY,
	G_MAP,
	NUM_GEAR
} Gear;

typedef enum {
	CMP_WARRIOR,
	CMP_SCHOLAR,
	CMP_BANDITS,
	CMP_ROBOT,
	NUM_COMPANIONS
} Companions;

typedef enum {
	SKL_FIGHT,
	SKL_SHOOT,
	SKL_ACROBATICS,
	SKL_STEALTH,
	SKL_INVESTIGATE,
	SKL_KNOWLEDGE,
	SKL_CRAFT,
	SKL_SURVIVAL,
	SKL_EMPATHY,
	SKL_INTIMIDATE,
	SKL_BARTER,
	SKL_LIE,
	NUM_SKILLS
} Skills;

typedef struct {
	// stats
	uint8_t stat_physicalDie;
	uint8_t stat_mentalDie;
	uint8_t stat_socialDie;

	uint8_t wounds_physical;
	uint8_t wounds_mental;
	uint8_t wounds_social;

	// skills
	//  physical
	uint8_t ps_fight;
	uint8_t ps_shoot;
	uint8_t ps_acrobatics;
	uint8_t ps_stealth;
	//  mental
	uint8_t ms_investigate;
	uint8_t ms_knowledge;
	uint8_t ms_craft;
	uint8_t ms_survival;
	//  social
	uint8_t ss_empathy;
	uint8_t ss_intimidate;
	uint8_t ss_barter;
	uint8_t ss_lie;

	uint32_t flags;

	uint8_t skillPointsLeft;

	uint8_t gear[NUM_GEAR];

	boolean hasCompanion[NUM_COMPANIONS];
} Character;

Character character;

typedef struct {
	HANDLE write;
	HANDLE read;
	CHAR_INFO buffer[SCREEN_WIDTH*SCREEN_HEIGHT];
} Screen;
Screen screen;

typedef enum {
	IN_NONE,
	IN_UP,
	IN_DOWN,
	IN_LEFT,
	IN_RIGHT,
	IN_ENTER,
	IN_SPACE,
	IN_PLUS,
	IN_MINUS,
	IN_OPT_1,
	IN_OPT_2,
	IN_OPT_3,
	IN_OPT_4,
	IN_OPT_5,
	IN_OPT_6,
	IN_OPT_7,
	IN_OPT_8,
	IN_OPT_9,
	IN_C,
	IN_H,
	IN_OTHER
} Input;

SMALL_RECT windowSize = { 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1 };
SMALL_RECT renderArea = { 1, 1, SCREEN_WIDTH - 2, SCREEN_HEIGHT - 2 };
SMALL_RECT safeWriteArea = { 2, 2, SCREEN_WIDTH - 3, SCREEN_HEIGHT - 3 };
COORD topLeft = { 0, 0 };
COORD bufferSize = { SCREEN_WIDTH, SCREEN_HEIGHT };

char* topTitle = NULL;

#define SCREEN_POS( x, y ) ( ( x ) + ( SCREEN_WIDTH * ( y ) ) )

char tempStrBuffer[2048];
char otherTempStrBuffer[2048];

typedef void (*Scene)( void );
Scene currentScene;
Scene storedScene;
Scene nextScene;

typedef enum {
	R_FAILURE,
	R_COSTLY_SUCCESS,
	R_SUCCESS
} Result;

boolean contains( COORD c, SMALL_RECT area )
{
	return ( ( c.X <= area.Right ) && ( c.X >= area.Left ) && ( c.Y <= area.Bottom ) && ( c.Y >= area.Top ) );
}

void simplePutChar( CHAR c, WORD attr, SHORT x, SHORT y )
{
	int idx = SCREEN_POS( x, y );
	screen.buffer[idx].Attributes = attr;
	screen.buffer[idx].Char.AsciiChar = c;
}

void putChar( CHAR c, WORD attr, COORD pos, SMALL_RECT border )
{
	if( contains( pos, border ) ) {
		int idx = SCREEN_POS( pos.X, pos.Y );
		screen.buffer[idx].Attributes = attr;
		screen.buffer[idx].Char.AsciiChar = c;
	}
}

COORD vdrawStringIgnoreSize( SHORT x, SHORT y, SMALL_RECT border, const char* str, WORD attributes, va_list args )
{
	vsnprintf( otherTempStrBuffer, ARRAYSIZE( otherTempStrBuffer ), str, args );
	SHORT startX = x;
	size_t len = strlen( otherTempStrBuffer );
	COORD endPoint;
	endPoint.X = x;
	endPoint.Y = y;

	for( size_t i = 0; i < len; ++i ) {
		if( otherTempStrBuffer[i] != '\n' ) {
			putChar( otherTempStrBuffer[i], attributes, endPoint, border );
			++endPoint.X;
		}

		if( ( otherTempStrBuffer[i] == '\n' ) || ( endPoint.X > border.Right ) ) {
			endPoint.X = startX;
			++endPoint.Y;
		}

		if( endPoint.Y > border.Bottom ) {
			return endPoint;
		}
	}

	return endPoint;
}

COORD drawStringIgnoreSize( SHORT x, SHORT y, SMALL_RECT border, const char* str, WORD attributes, ... )
{
	COORD c;
	va_list args;
	va_start( args, attributes );
	c = vdrawStringIgnoreSize( x, y, border, str, attributes, args );
	va_end( args );
	return c;
}

COORD vdrawString( SHORT x, SHORT y, SMALL_RECT border, const char* str, WORD attributes, va_list args )
{
	vsnprintf( otherTempStrBuffer, ARRAYSIZE( otherTempStrBuffer ), str, args );
#define NEWLINE { endPoint.X = startX; ++endPoint.Y; }
	COORD endPoint;
	SHORT startX = x;
	size_t len = strlen( otherTempStrBuffer );
	size_t strPos = 0;
	endPoint.X = x;
	endPoint.Y = y;

	char* copy = strncpy( &( tempStrBuffer[0] ), otherTempStrBuffer, ARRAYSIZE( tempStrBuffer ) );

	if( otherTempStrBuffer[0] == ' ' ) {
		putChar( ' ', attributes, endPoint, border );
		++( endPoint.X );
		++strPos;
		if( endPoint.X > border.Right ) {
			NEWLINE
		}
	}

	char* tok = strtok( tempStrBuffer, " " );
	
	while( tok != NULL ) {
		size_t tokLen = strlen( tok );

		// first see if there's enough room to write the word
		if( (size_t)( border.Right - endPoint.X ) < tokLen ) {
			endPoint.X = startX;
			++endPoint.Y;
		} else {
			for( size_t i = 0; i < tokLen; ++i ) {
				if( tok[i] == '\n' ) {
					NEWLINE
				} else {
					putChar( tok[i], attributes, endPoint, border );
					++endPoint.X;
					++strPos;
				}
			}

			// if we're not at the end of the string, that means we ran into a space, write it out
			if( otherTempStrBuffer[strPos] != 0 ) {
				putChar( ' ', attributes, endPoint, border );
				++endPoint.X;
				++strPos;
			}
			tok = strtok( NULL, " " );
		}

		if( endPoint.X > border.Right ) {
			NEWLINE
		}

		if( endPoint.Y > border.Bottom ) {
			return endPoint;
		}
	}

	return endPoint;

#undef NEWLINE
}

COORD drawString( SHORT x, SHORT y, SMALL_RECT border, const char* str, WORD attributes, ... )
{
	COORD c;
	va_list args;
	va_start( args, attributes );
	c = vdrawString( x, y, border, str, attributes, args );
	va_end( args );
	return c;
}

void centerStringHoriz( SMALL_RECT area, SHORT y, const char* str, WORD attributes, ... )
{
	va_list args;
	va_start( args, attributes );
	// want to measure the string by it's longest line
	char* copy = strncpy( &( tempStrBuffer[0] ), str, ARRAYSIZE( tempStrBuffer ) );
	SHORT x = 0;
	size_t maxLen = 0;
	char* tok = strtok( tempStrBuffer, "\n" );
	while( tok != NULL ) {
		size_t len = strlen( tok );
		if( len > maxLen ) {
			maxLen = len;
			size_t strMid = len / 2;
			SHORT renderMid = ( ( area.Right + area.Left ) / 2 );
			x = renderMid - (SHORT)strMid;
		}

		tok = strtok( NULL, "\n" );
	}

	vdrawStringIgnoreSize( x, y, area, str, attributes, args );
	va_end( args );
}

void drawBorder( )
{
	WORD attr = FG_GREY | BG_BLACK;
	// draw corners
	simplePutChar( 201, attr, 0, 0 );
	simplePutChar( 200, attr, 0, SCREEN_HEIGHT - 1 );
	simplePutChar( 187, attr, SCREEN_WIDTH - 1, 0 );
	simplePutChar( 188, attr, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1 );

	// draw top and bottom
	for( int x = 1; x < SCREEN_WIDTH-1; ++x ) {
		simplePutChar( 205, attr, x, SCREEN_HEIGHT - 1 );
		simplePutChar( 205, attr, x, 0 );
	}

	// draw left and right
	for( int y = 1; y < SCREEN_HEIGHT-1; ++y ) {
		simplePutChar( 186, attr, 0, y );
		simplePutChar( 186, attr, SCREEN_WIDTH - 1, y );
	}

	// draw top label
	if( topTitle != NULL ) {
		centerStringHoriz( windowSize, 0, topTitle, FG_WHITE | BG_BLACK );
	}
}

void startDraw( void )
{
	memset( screen.buffer, 0, sizeof( screen.buffer[0] ) * ARRAYSIZE( screen.buffer ) );
	drawBorder( );
}

void endDraw( void )
{
	WriteConsoleOutput( screen.write, screen.buffer, bufferSize, topLeft, &windowSize );
}

Input getNextInput( void )
{
	DWORD evtCnt = 0;
	DWORD evtRead = 0;

	GetNumberOfConsoleInputEvents( screen.read, &evtCnt );
	if( evtCnt <= 0 ) {
		return IN_NONE;
	}

	INPUT_RECORD inputRec;
	ReadConsoleInput( screen.read, &inputRec, 1, &evtRead );
	if( evtRead <= 0 ) {
		return IN_NONE;
	}

	if( ( inputRec.EventType == KEY_EVENT ) && inputRec.Event.KeyEvent.bKeyDown ) {
		if( inputRec.Event.KeyEvent.wVirtualKeyCode == VK_UP ) {
			return IN_UP;
		} else if( inputRec.Event.KeyEvent.wVirtualKeyCode == VK_DOWN ) {
			return IN_DOWN;
		} else if( inputRec.Event.KeyEvent.wVirtualKeyCode == VK_LEFT ) {
			return IN_LEFT;
		} else if( inputRec.Event.KeyEvent.wVirtualKeyCode == VK_RIGHT ) {
			return IN_RIGHT;
		} else if( inputRec.Event.KeyEvent.wVirtualKeyCode == VK_RETURN ) {
			return IN_ENTER;
		} else if( inputRec.Event.KeyEvent.wVirtualKeyCode == VK_SPACE ) {
			return IN_SPACE;
		} else if( ( inputRec.Event.KeyEvent.wVirtualKeyCode == VK_ADD ) ||
				   ( inputRec.Event.KeyEvent.uChar.AsciiChar == '=' ) ||
				   ( inputRec.Event.KeyEvent.uChar.AsciiChar == '+' ) ) {
			return IN_PLUS;
		} else if( ( inputRec.Event.KeyEvent.wVirtualKeyCode == VK_SUBTRACT ) ||
				   ( inputRec.Event.KeyEvent.uChar.AsciiChar == '-' ) ||
				   ( inputRec.Event.KeyEvent.uChar.AsciiChar == '_' ) ) {
			return IN_MINUS;
		} else if( ( inputRec.Event.KeyEvent.uChar.AsciiChar >= '1' ) &&
				   ( inputRec.Event.KeyEvent.uChar.AsciiChar <= '9' ) ) {
			return ( IN_OPT_1 + ( inputRec.Event.KeyEvent.uChar.AsciiChar - '1' ) );
		} else if( ( inputRec.Event.KeyEvent.uChar.AsciiChar == 'c' ) ||
				   ( inputRec.Event.KeyEvent.uChar.AsciiChar == 'C' ) ) {
			return IN_C;
		} else if( ( inputRec.Event.KeyEvent.uChar.AsciiChar == 'h' ) ||
				   ( inputRec.Event.KeyEvent.uChar.AsciiChar == 'H' ) ) {
			return IN_H;
		}
		return IN_OTHER;
	}

	return IN_NONE;
}

void waitForAnyInput( void )
{
	Input input;
	do {
		input = getNextInput( );
	} while( input == IN_NONE );
}

void eatAllInputs( void )
{
	Input input;
	do {
		input = getNextInput( );
	} while( input != IN_NONE );
}

void testDevCharacter( )
{
	character.stat_physicalDie = 8;
	character.stat_mentalDie = 8;
	character.stat_socialDie = 8;

	character.ps_fight = 4;
	character.ps_shoot = 4;
	character.ps_acrobatics = 4;
	character.ps_stealth = 4;
	character.ms_investigate = 4;
	character.ms_knowledge = 4;
	character.ms_craft = 4;
	character.ms_survival = 4;
	character.ss_empathy = 4;
	character.ss_intimidate = 4;
	character.ss_barter = 4;
	character.ss_lie = 4;

	character.flags = 0;

	character.skillPointsLeft = 0;

	character.wounds_physical = 0;
	character.wounds_mental = 0;
	character.wounds_social = 0;

	for( int i = 0; i < NUM_GEAR; ++i ) {
		character.gear[i] = 2;
	}

	for( int i = 0; i < NUM_COMPANIONS; ++i ) {
		character.hasCompanion[i] = false;
	}
}

void initCharacter( )
{
	character.stat_physicalDie = 8;
	character.stat_mentalDie = 8;
	character.stat_socialDie = 8;

	character.ps_fight = 0;
	character.ps_shoot = 0;
	character.ps_acrobatics = 0;
	character.ps_stealth = 0;
	character.ms_investigate = 0;
	character.ms_knowledge = 0;
	character.ms_craft = 0;
	character.ms_survival = 0;
	character.ss_empathy = 0;
	character.ss_intimidate = 0;
	character.ss_barter = 0;
	character.ss_lie = 0;

	character.flags = 0;

	character.skillPointsLeft = BASE_SKILL_POINTS;

	character.wounds_physical = 0;
	character.wounds_mental = 0;
	character.wounds_social = 0;

	for( int i = 0; i < NUM_GEAR; ++i ) {
		character.gear[i] = 0;
	}

	for( int i = 0; i < NUM_COMPANIONS; ++i ) {
		character.hasCompanion[i] = false;
	}
}

void increaseStat( uint8_t* stat )
{
	if( (*stat) >= 12 ) {
		character.skillPointsLeft += 4;
	} else {
		(*stat) += 2;
	}
}

void decreaseStat( uint8_t* stat )
{
	if( (*stat) <= 4 )  {
		if( ( character.skillPointsLeft - 4 ) >= MIN_SKILL_POINTS ) {
			character.skillPointsLeft -= 4;
		}
	} else {
		(*stat) -= 2;
	}
}

#define SHUFFLE( a, type ) { \
		for( int i = 0; i < ( ARRAYSIZE( a ) - 1 ); ++i ) { \
			int swap = i + ( rand( ) % ( ARRAYSIZE( a ) - ( i + 1 ) ) ); \
			type temp = a[i]; a[i] = a[swap]; a[swap] = temp; \
		} \
	}

#define NUM_SKILLS 12

typedef struct {
	char* name;
	uint8_t physical;
	uint8_t mental;
	uint8_t social;
} CompanionData;

CompanionData companionsData[NUM_COMPANIONS];

void setupCompanionData( void )
{
	companionsData[CMP_WARRIOR].name = "Bryon";
	companionsData[CMP_WARRIOR].physical = 3;
	companionsData[CMP_WARRIOR].mental = 0;
	companionsData[CMP_WARRIOR].social = 1;

	companionsData[CMP_SCHOLAR].name = "Freda";
	companionsData[CMP_SCHOLAR].physical = 1;
	companionsData[CMP_SCHOLAR].mental = 3;
	companionsData[CMP_SCHOLAR].social = 0;

	companionsData[CMP_BANDITS].name = "Koy, Ginger, and Hay";
	companionsData[CMP_BANDITS].physical = 2;
	companionsData[CMP_BANDITS].mental = 1;
	companionsData[CMP_BANDITS].social = 2;

	companionsData[CMP_ROBOT].name = "Red"; // Reactive Exploration Drone Model Gamma Production Number 012
	companionsData[CMP_ROBOT].physical = 4;
	companionsData[CMP_ROBOT].mental = 3;
	companionsData[CMP_ROBOT].social = 0;
}

typedef struct {
	char* name;
	char* description;
	boolean availableAtStore;
	uint8_t bonus;
} GearData;

GearData gearData[NUM_GEAR];

void setupGearData( void )
{
	gearData[G_KNIFE].name = "Knife";
	gearData[G_KNIFE].description = "A small blade, useful for cutting things or people.";
	gearData[G_KNIFE].availableAtStore = true;
	gearData[G_KNIFE].bonus = 1;

	gearData[G_PISTOL].name = "Pistol";
	gearData[G_PISTOL].description = "A small firearm, noisy but able to drop most things in a single shot.";
	gearData[G_PISTOL].availableAtStore = true;
	gearData[G_KNIFE].bonus = 2;

	gearData[G_LANTERN].name = "Lantern";
	gearData[G_LANTERN].description = "A glass lantern, useful for when you have to go somewhere with no light.";
	gearData[G_LANTERN].availableAtStore = true;

	gearData[G_RATION].name = "Ration";
	gearData[G_RATION].description = "Perserved food. A necessity if you're out on the road.";
	gearData[G_RATION].availableAtStore = true;

	gearData[G_ROPE].name = "Rope";
	gearData[G_ROPE].description = "A long length of rope with multiple uses.";
	gearData[G_ROPE].availableAtStore = true;

	gearData[G_SLING].name = "Sling";
	gearData[G_SLING].description = "A primitive weapon, but with the bonus of being quiet.";
	gearData[G_SLING].availableAtStore = true;
	gearData[G_KNIFE].bonus = 1;

	gearData[G_TOWEL].name = "Towel";
	gearData[G_TOWEL].description = "An ancient holy book has a commandment saying you should never go anywhere without one.";
	gearData[G_TOWEL].availableAtStore = true;

	gearData[G_LIGHT_ARMOR].name = "Light Armor";
	gearData[G_LIGHT_ARMOR].description = "Scraps of leather fitted together into crude armor. Better than nothing.";
	gearData[G_LIGHT_ARMOR].availableAtStore = true;

	gearData[G_RIFLE].name = "Rifle";
	gearData[G_RIFLE].description = "A sturdy long range fire arm.";
	gearData[G_RIFLE].availableAtStore = false;
	gearData[G_KNIFE].bonus = 3;

	gearData[G_FLASHLIGHT].name = "Flashlight";
	gearData[G_FLASHLIGHT].description = "A torch that runs on electricity.";
	gearData[G_FLASHLIGHT].availableAtStore = false;

	gearData[G_HACKING_DEVICE].name = "Strange Device";
	gearData[G_HACKING_DEVICE].description = "Some sort of electronic device with multiple cords coming out of it.";
	gearData[G_HACKING_DEVICE].availableAtStore = false;

	gearData[G_SWORD].name = "Sword";
	gearData[G_SWORD].description = "A long bladed weapon.";
	gearData[G_SWORD].availableAtStore = false;
	gearData[G_KNIFE].bonus = 3;

	gearData[G_MONEY].name = "Money";
	gearData[G_MONEY].description = "A handful of currency.";
	gearData[G_MONEY].availableAtStore = true;

	gearData[G_MAP].name = "Map";
	gearData[G_MAP].description = "A map of the region surrounding where you grew up.";
	gearData[G_MAP].availableAtStore = false;
}

typedef struct {
	uint8_t* value;
	uint8_t* attrValue;
	char* name;
	char* description;
	char* abbreviation;
} SkillData;

SkillData skillsData[NUM_SKILLS];

void setupSkillsData( void )
{
	skillsData[SKL_FIGHT].value = &( character.ps_fight );
	skillsData[SKL_FIGHT].name = "Fight";
	skillsData[SKL_FIGHT].description = "Your ability in melee combat, whether with hands or weapons.";
	skillsData[SKL_FIGHT].attrValue = &( character.stat_physicalDie );
	skillsData[SKL_FIGHT].abbreviation = "FGH";

	skillsData[SKL_SHOOT].value = &( character.ps_shoot );
	skillsData[SKL_SHOOT].name = "Shoot";
	skillsData[SKL_SHOOT].description = "How good you are at using any sort of ranged weapons.";
	skillsData[SKL_SHOOT].attrValue = &( character.stat_physicalDie );
	skillsData[SKL_SHOOT].abbreviation = "SHT";

	skillsData[SKL_ACROBATICS].value = &( character.ps_acrobatics );
	skillsData[SKL_ACROBATICS].name = "Acrobatics";
	skillsData[SKL_ACROBATICS].description = "The measure of your general physical training and how good you are at various physical tasks.";
	skillsData[SKL_ACROBATICS].attrValue = &( character.stat_physicalDie );
	skillsData[SKL_ACROBATICS].abbreviation = "ACR";

	skillsData[SKL_STEALTH].value = &( character.ps_stealth );
	skillsData[SKL_STEALTH].name = "Stealth";
	skillsData[SKL_STEALTH].description = "How good you are at escaping notice when you don't want to be seen.";
	skillsData[SKL_STEALTH].attrValue = &( character.stat_physicalDie );
	skillsData[SKL_STEALTH].abbreviation = "STL";

	skillsData[SKL_INVESTIGATE].value = &( character.ms_investigate );
	skillsData[SKL_INVESTIGATE].name = "Investigate";
	skillsData[SKL_INVESTIGATE].description = "How good you are at noticing things around you.";
	skillsData[SKL_INVESTIGATE].attrValue = &( character.stat_mentalDie );
	skillsData[SKL_INVESTIGATE].abbreviation = "INV";

	skillsData[SKL_KNOWLEDGE].value = &( character.ms_knowledge );
	skillsData[SKL_KNOWLEDGE].name = "Knowledge";
	skillsData[SKL_KNOWLEDGE].description = "The measure of your general knowledge.";
	skillsData[SKL_KNOWLEDGE].attrValue = &( character.stat_mentalDie );
	skillsData[SKL_KNOWLEDGE].abbreviation = "KNW";

	skillsData[SKL_CRAFT].value = &( character.ms_craft );
	skillsData[SKL_CRAFT].name = "Craft";
	skillsData[SKL_CRAFT].description = "How handy you are at making and repairing items. Also useful when breaking things as well.";
	skillsData[SKL_CRAFT].attrValue = &( character.stat_mentalDie );
	skillsData[SKL_CRAFT].abbreviation = "CFT";

	skillsData[SKL_SURVIVAL].value = &( character.ms_survival );
	skillsData[SKL_SURVIVAL].name = "Survival";
	skillsData[SKL_SURVIVAL].description = "The amount of knowledge you have about surviving in the wilderness. Very important if you're going out into the wild.";
	skillsData[SKL_SURVIVAL].attrValue = &( character.stat_mentalDie );
	skillsData[SKL_SURVIVAL].abbreviation = "SRV";

	skillsData[SKL_EMPATHY].value = &( character.ss_empathy );
	skillsData[SKL_EMPATHY].name = "Empathy";
	skillsData[SKL_EMPATHY].description = "Your ability to see other people's points of view and understand them better.";
	skillsData[SKL_EMPATHY].attrValue = &( character.stat_socialDie );
	skillsData[SKL_EMPATHY].abbreviation = "EMP";

	skillsData[SKL_INTIMIDATE].value = &( character.ss_intimidate );
	skillsData[SKL_INTIMIDATE].name = "Intimidate";
	skillsData[SKL_INTIMIDATE].description = "The measure of how well you are able to make people fear you.";
	skillsData[SKL_INTIMIDATE].attrValue = &( character.stat_socialDie );
	skillsData[SKL_INTIMIDATE].abbreviation = "INT";

	skillsData[SKL_BARTER].value = &( character.ss_barter );
	skillsData[SKL_BARTER].name = "Barter";
	skillsData[SKL_BARTER].description = "How good you are at making business deals.";
	skillsData[SKL_BARTER].attrValue = &( character.stat_socialDie );
	skillsData[SKL_BARTER].abbreviation = "BTR";

	skillsData[SKL_LIE].value = &( character.ss_lie );
	skillsData[SKL_LIE].name = "Lie";
	skillsData[SKL_LIE].description = "How good you are at bluffing and lying to others.";
	skillsData[SKL_LIE].attrValue = &( character.stat_socialDie );
	skillsData[SKL_LIE].abbreviation = "LIE";
}

void randomlyDistributeSkills( void )
{
	// randomly weight based on the size of the attribute and how many points have already
	//  been spent, don't go over the size of the die
	int skillWeights[NUM_SKILLS];
	while( character.skillPointsLeft > 0 ) {
		memset( skillWeights, 0, sizeof( skillWeights[0] ) * ARRAYSIZE( skillWeights ) );
		int totalWeights = 0;
		for( int i = 0; i < NUM_SKILLS; ++i ) {
			skillWeights[i] = (*skillsData[i].attrValue) - (*skillsData[i].value);
			totalWeights += skillWeights[i];
		}
		int choice = rand( ) % totalWeights;

		int idx = 0;
		while( skillWeights[idx] < choice ) {
			choice -= skillWeights[idx];
			++idx;
		}

		++(*skillsData[idx].value);
		--character.skillPointsLeft;
	}
}

Result skillCheck( int skillIdx, uint8_t modifiers, uint8_t difficulty )
{
	// gather all the data we need to make the roll
	uint8_t companionPhysical = 0;
	uint8_t companionMental = 0;
	uint8_t companionSocial = 0;
	for( int i = 0; i < NUM_COMPANIONS; ++i ) {
		if( character.hasCompanion[i] ) {
			companionPhysical += companionsData[i].physical;
			companionMental += companionsData[i].mental;
			companionSocial += companionsData[i].social;
		}
	}

	uint8_t stat;
	uint8_t skill;
	if( ( skillIdx >= 0 ) && ( skillIdx < 4 ) ) {
		stat = character.stat_physicalDie;
		skill = companionPhysical;
	} else if( ( skillIdx >= 4 ) && ( skillIdx < 8 ) ) {
		stat = character.stat_mentalDie;
		skill = companionMental;
	} else {
		stat = character.stat_socialDie;
		skill = companionSocial;
	}
	skill += (*skillsData[skillIdx].value);

	// make the roll
	int roll = ( rand( ) % (int)stat );
	int check = ( roll + (int)skill ) + (int)modifiers;

	Result result;
	if( check >= difficulty ) {
		// player gets what they want
		result = R_SUCCESS;
	} else if( check >= ( difficulty / 2 ) ) {
		// player gets what they want but at a cost
		result = R_COSTLY_SUCCESS;
	} else {
		// player doesn't get what they want and pays a cost
		result = R_FAILURE;
	}

	// rolling a 1 reduces the result by one step
	if( ( roll == 1 ) && ( result > R_FAILURE ) ) {
		--result;
	}

	return result;
}

int carryWeight( void )
{
	return ( character.stat_physicalDie + character.ps_acrobatics );
}

int totalGearCount( void )
{
	int total = 0;
	for( int i = 0; i < NUM_GEAR; ++i ) {
		total += character.gear[i];
	}
	return total;
}

int gearSpaceLeft( void )
{
	return ( carryWeight( ) - totalGearCount( ) );
}

void addGear( Gear g )
{
	++character.gear[g];
}

void removeGear( Gear g )
{
	if( character.gear[g] > 0 ) {
		--character.gear[g];
	}
}

#define DESC_BORDER_BOTTOM 41
#define DESC_BORDER_RIGHT 55
SMALL_RECT descriptionSafeArea = { 2, 2, DESC_BORDER_RIGHT - 2, DESC_BORDER_BOTTOM - 2 };
SMALL_RECT choicesSafeArea = { 3, DESC_BORDER_BOTTOM + 1, DESC_BORDER_RIGHT - 2, SCREEN_HEIGHT - 2 };
SMALL_RECT characterSafeArea = { DESC_BORDER_RIGHT + 2, 2, SCREEN_WIDTH - 3, SCREEN_HEIGHT - 2 };

void drawPlayScreen( void )
{
	// we already have the main border, so we just need to draw the
	//  border for the other areas (text description, choices, and character)
	WORD attr = FG_GREY | BG_BLACK;
	
	simplePutChar( 204, attr, 0, DESC_BORDER_BOTTOM );
	simplePutChar( 202, attr, DESC_BORDER_RIGHT, SCREEN_HEIGHT - 1 );
	
	CHAR c = screen.buffer[SCREEN_POS( DESC_BORDER_RIGHT, 0 )].Char.AsciiChar;
	if( ( c < 0 ) || ( c > 127 ) ) {
		simplePutChar( 203, attr, DESC_BORDER_RIGHT, 0 );
	}

	for( int i = 1; i < ( DESC_BORDER_RIGHT ); ++i ) {
		simplePutChar( 205, attr, i, DESC_BORDER_BOTTOM );
	}

	for( int i = 1; i < ( SCREEN_HEIGHT - 1 ); ++i ) {
		simplePutChar( 186, attr, DESC_BORDER_RIGHT, i );
	}

	simplePutChar( 185, attr, DESC_BORDER_RIGHT, DESC_BORDER_BOTTOM );

	// draw the character info
	//  stats and skills
	COORD pos;
	pos.X = characterSafeArea.Left + 2;
	pos.Y = characterSafeArea.Top;
	drawString( pos.X - 1, pos.Y, characterSafeArea, "Stats and Skills", FG_BROWN );
	pos.Y += 2;
	drawString( pos.X, pos.Y, characterSafeArea, "Physical: d%i", FG_DARK_GREEN, character.stat_physicalDie );
	++pos.Y;
	int i = 0;
	for( ; i < 4; ++i ) {
		drawString( pos.X, pos.Y, characterSafeArea, " %s: %i", FG_DARK_CYAN, skillsData[i].name, (*skillsData[i].value ) );
		++pos.Y;
	}

	++pos.Y;
	drawString( pos.X, pos.Y, characterSafeArea, "Mental: d%i", FG_DARK_GREEN, character.stat_mentalDie );
	++pos.Y;
	for( ; i < 8; ++i ) {
		drawString( pos.X, pos.Y, characterSafeArea, " %s: %i", FG_DARK_CYAN, skillsData[i].name, (*skillsData[i].value ) );
		++pos.Y;
	}

	++pos.Y;
	drawString( pos.X, pos.Y, characterSafeArea, "Social: d%i", FG_DARK_GREEN, character.stat_socialDie );
	++pos.Y;
	for( ; i < 12; ++i ) {
		drawString( pos.X, pos.Y, characterSafeArea, " %s: %i", FG_DARK_CYAN, skillsData[i].name, (*skillsData[i].value ) );
		++pos.Y;
	}

	// draw cumulative wounds
	++pos.Y;
	drawString( pos.X - 1, pos.Y, characterSafeArea, "Total Wounds", FG_BROWN );
	++pos.Y;
	drawString( pos.X, pos.Y, characterSafeArea, "Physical: %i", FG_MAROON, character.wounds_physical );
	++pos.Y;
	drawString( pos.X, pos.Y, characterSafeArea, "Mental: %i", FG_MAROON, character.wounds_mental );
	++pos.Y;
	drawString( pos.X, pos.Y, characterSafeArea, "Social: %i", FG_MAROON, character.wounds_social );

	pos.Y += 2;
	drawString( pos.X - 1, pos.Y, characterSafeArea, "Companion Bonuses", FG_BROWN );
	++pos.Y;
	int physicalBonus = 0;
	int mentalBonus = 0;
	int socialBonus = 0;
	for( int i = 0; i < NUM_COMPANIONS; ++i ) {
		if( character.hasCompanion[i] ) {
			physicalBonus += companionsData[i].physical;
			mentalBonus += companionsData[i].mental;
			socialBonus += companionsData[i].social;
		}
	}
	drawString( pos.X, pos.Y, characterSafeArea, "Physical: +%i", FG_DARK_GREEN, physicalBonus );
	++pos.Y;
	drawString( pos.X, pos.Y, characterSafeArea, "Mental: +%i", FG_DARK_GREEN, mentalBonus );
	++pos.Y;
	drawString( pos.X, pos.Y, characterSafeArea, "Social: +%i", FG_DARK_GREEN, socialBonus );


	drawString( characterSafeArea.Left, characterSafeArea.Bottom - 1, characterSafeArea, "C -> Character Sheet", FG_YELLOW );
	drawString( characterSafeArea.Left, characterSafeArea.Bottom, characterSafeArea, "H -> Help", FG_YELLOW );
}

void startPlayDraw( void )
{
	startDraw( );
	drawPlayScreen( );
}

void helpScene( void );
void characterDetailsScene( void );
boolean fromStatusScreen;

boolean testSharedInput( Input input )
{
	boolean handled = false;
	switch( input ) {
	case IN_C:
		handled = true;
		storedScene = currentScene;
		nextScene = characterDetailsScene;
		break;
	case IN_H:
		handled = true;
		storedScene = currentScene;
		nextScene = helpScene;
		break;
	}

	return handled;
}

typedef enum {
	CT_SKILL_BASED,
	CT_SIMPLE,
	NUM_CHOICE_TYPES
} ChoiceType;

typedef struct {
	ChoiceType type;
	char* description;

	Scene failureScene;
	Scene successScene;
	Scene costlySuccessScene;

	Skills skill;
	uint8_t difficulty;
} SkillBasedChoice;

typedef struct {
	ChoiceType type;
	char* description;

	Scene nextScene;
} SimpleChoice;

typedef struct {
	ChoiceType type;
	char* description;
} BaseChoice;

typedef union {
	ChoiceType type;

	BaseChoice base;
	SkillBasedChoice skillBased;
	SimpleChoice simple;
} Choice;

char* deathReason;

void handleSimpleChoice( SimpleChoice choice )
{
	nextScene = choice.nextScene;
}

void handleSkillBasedChoice( SkillBasedChoice choice )
{
	Result r = skillCheck( choice.skill, 0, choice.difficulty );
	switch( r ) {
	case R_SUCCESS:
		nextScene = choice.successScene;
		break;
	case R_COSTLY_SUCCESS:
		nextScene = choice.costlySuccessScene;
		break;
	case R_FAILURE:
		nextScene = choice.failureScene;
		break;
	}
}

void handleChoice( Choice choice )
{
	switch( choice.type ) {
	case CT_SIMPLE:
		handleSimpleChoice( choice.simple );
		break;
	case CT_SKILL_BASED:
		handleSkillBasedChoice( choice.skillBased );
		break;
	}
}

int choiceTop;
int standardCurrentChoice;
void standardSceneChoiceDraw( Choice* choices, size_t numChoices )
{
	WORD selected = FG_CYAN | BG_BROWN;
	WORD normal = FG_DARK_CYAN | BG_BLACK;
	WORD attr;

	COORD outPos;
	outPos.Y = choicesSafeArea.Top - 1;
	for( int i = choiceTop; i < numChoices; ++i ) {
		attr = ( standardCurrentChoice == i ) ? selected : normal;

		outPos = drawString( choicesSafeArea.Left, outPos.Y + 1, choicesSafeArea, "%s", attr, choices[i].base.description );
	}

	// if there are more below then show down arrow
	if( ( choiceTop + ( choicesSafeArea.Bottom - choicesSafeArea.Top ) ) < ( numChoices - 1 ) ) {
		drawString( choicesSafeArea.Left - 1, choicesSafeArea.Bottom, renderArea, "v", FG_YELLOW );
	}
	// if there are more above then show up arrow
	if( choiceTop > 0 ) {
		drawString( choicesSafeArea.Left - 1, choicesSafeArea.Top, renderArea, "^", FG_YELLOW );
	}
}

void fitCurrentSelection( void )
{
	// see if the current choice is visible
	int range = choicesSafeArea.Bottom - choicesSafeArea.Top;
	while( ( standardCurrentChoice - choiceTop ) > range ) {
		++choiceTop;
	}

	while( ( standardCurrentChoice - choiceTop ) < 0 ) {
		--choiceTop;
	}
}

void standardSceneChoice( Choice* choices, size_t numChoices, boolean allowDefaults )
{
	Input input;
	boolean handled = false;

	do {
		input = getNextInput( );
		if( allowDefaults ) {
			handled = testSharedInput( input );
		}
		if( !handled ) {
			switch( input ) {
			case IN_UP:
				if( standardCurrentChoice > 0 ) {
					--standardCurrentChoice;
				}
				fitCurrentSelection( );
				handled = true;
				break;
			case IN_DOWN:
				if( standardCurrentChoice < ( numChoices - 1 ) ) {
					++standardCurrentChoice;
				}
				//standardCurrentChoice = ( standardCurrentChoice + 1 ) % numChoices;
				fitCurrentSelection( );
				handled = true;
				break;
			case IN_ENTER:
				handleChoice( choices[standardCurrentChoice] );
				handled = true;
				break;
			}
		}
	} while( !handled );

	// eat the rest of the inputs
	eatAllInputs( );
}

void startScene( )
{
	nextScene = NULL;
	standardCurrentChoice = 0;
	choiceTop = 0;
	topTitle = NULL;
	fromStatusScreen = false;;
}

void pushSimpleChoice( Choice** sbChoices, char* description, Scene nextScene )
{
	Choice c;
	c.type = CT_SIMPLE;
	c.simple.description = description;
	c.simple.nextScene = nextScene;

	sb_push( (*sbChoices), c );
}

void pushSkillBasedChoice( Choice** sbChoices, char* description, Skills skill, uint8_t difficulty,
	Scene successScene, Scene costlySuccessScene, Scene failureScene )
{
	// filter skill choices by the amount of wounds they have to that statistic
	switch( skill % 4 ) {
	case 0:
		if( character.wounds_physical >= character.stat_physicalDie ) {
			return;
		}
		break;
	case 1:
		if( character.wounds_mental >= character.stat_mentalDie ) {
			return;
		}
		break;
	case 2:
		if( character.wounds_social >= character.stat_socialDie ) {
			return;
		}
		break;
	}

	Choice c;
	c.type = CT_SKILL_BASED;
	c.skillBased.description = description;
	c.skillBased.skill = skill;
	c.skillBased.difficulty = difficulty;
	c.skillBased.successScene = successScene;
	c.skillBased.costlySuccessScene = costlySuccessScene;
	c.skillBased.failureScene = failureScene;

	sb_push( (*sbChoices), c );
}

void dropGearScene( void );
void deathScene( void );

boolean checkForDeath( const char* reason )
{
	if( character.wounds_physical >= character.stat_physicalDie ) {
		deathReason = reason;
		nextScene = deathScene;
		return true;
	}
	return false;
}

boolean armoredWound = false;
void gainPhysicalWound( const char* deathReason )
{
	armoredWound = false;
	// armor absorbs wounds
	if( character.gear[G_LIGHT_ARMOR] ) {
		armoredWound = true;
		--character.gear[G_LIGHT_ARMOR];
		return;
	}
	++character.wounds_physical;
	checkForDeath( deathReason );
}

void gainNonArmorPhysicalWound( const char* deathReason )
{
	armoredWound = false;
	++character.wounds_physical;
	checkForDeath( deathReason );
}

void gainMentalWound( void )
{
	++character.wounds_mental;
}

void gainSocialWound( void )
{
	++character.wounds_social;
}

void checkTooMuchGear( Scene next )
{
	if( gearSpaceLeft( ) < 0 ) {
		currentScene = dropGearScene;
		nextScene = next;
	} else {
		currentScene = nextScene;
	}
}

COORD standardGearGainText( SHORT x, SHORT y, SMALL_RECT area, Gear newGear )
{
	return drawString( x, y, area, "You've gained a %s!", FG_GREEN, gearData[newGear].name );
}

COORD standardGearLossText( SHORT x, SHORT y, SMALL_RECT area, Gear oldGear )
{
	return drawString( x, y, area, "You've lost a %s.", FG_RED, gearData[oldGear].name );
}

COORD standardWoundText( SHORT x, SHORT y, SMALL_RECT area, const char* type, const char* reason )
{
	if( ( strcmp( type, "Physical" ) == 0 ) && armoredWound ) {
		armoredWound = false;
		return drawString( x, y, area, "Your armor absorbed the Wound!", FG_GREEN, reason, type );
	}  else {
		return drawString( x, y, area, "You %s and gained a %s Wound...", FG_RED, reason, type );
	}
}

Gear bestFightWeapon( void )
{
	if( character.gear[G_SWORD] > 0 ) {
		return G_SWORD;
	} else if( character.gear[G_KNIFE] > 0 ) {
		return G_KNIFE;
	}

	return -1;
}

int bestFightWeaponBonus( void )
{
	int g = bestFightWeapon( );
	int bonus = 0;
	if( g >= 0 ) {
		bonus = gearData[g].bonus;
	}

	return bonus;
}

Gear bestShootWeapon( void )
{
	if( character.gear[G_RIFLE] > 0 ) {
		return G_RIFLE;
	} else if( character.gear[G_PISTOL] > 0 ) {
		return G_PISTOL;
	} else if( character.gear[G_SLING] > 0 ) {
		return G_SLING;
	}

	return -1;
}

int bestShootWeaponBonus( void )
{
	int g = bestShootWeapon( );
	int bonus = 0;
	if( g >= 0 ) {
		bonus = gearData[g].bonus;
	}

	return bonus;
}

Gear randomGearToLose( void )
{
	int total = 0;
	for( int i = 0; i < NUM_GEAR; ++i ) {
		total += character.gear[i];
	}

	int choice = rand( ) % total;

	int idx = 0;
	while( character.gear[idx] < choice ) {
		choice -= character.gear[idx];
		++idx;
	}

	return idx;
}

int numCompanions( void )
{
	int total = 0;
	for( int i = 0; i < NUM_COMPANIONS; ++i ) {
		if( character.hasCompanion[i] ) ++total;
	}

	return total;
}

Companions getRandomCompanion( void )
{
	Companions c = -1;
	int i = numCompanions( );
	int idx = 0;
	while( c < 0 ) {
		if( character.hasCompanion[idx] ) {
			--i;
			if( i == 0 )  c = idx;
		}
		++idx;
	}

	return c;
}

/******* SCENES *********/

void titleScene( void );

void victoryScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Congratulations! You've won!", titleScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You quickly grab whatever you can get your hands on, filling your pack with "
				"scientific equipment and notes.\n\n"
				"When you get back to town you're welcomed with open arms. The researchers and "
				"engineers are excited to go through your findings. You also tell them where "
				"you found it and that the place isn't empty yet. They start putting together "
				"a larger group to go there, giving you a place among the scouts.\n\n"
				"It's not the cushiest job, but you get plenty of exercise and lots of free "
				"time to relax.",
				FG_GREY );
			
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void greatVictoryScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Congratulations! You got the best ending!", titleScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"Instead of just grabbing everything you can you carefully search the area, "
				"using what you remember from your lessons. You find what appears to be safe "
				"that has been rusted partially through. Reaching in you find a stack of paper "
				"full of strange symbols and numbers, written in a language you can't understand.\n\n"
				"When you get back to town you're welcomed with open arms. When you bring the stack "
				"of papers to the lead researcher his eyes widen and his jaw drops. He quickly shuffles "
				"through the papers and then quietly says it's all here to himself. He yells for one "
				"of the other researchers who quickly comes over. He hands her the stack of papers "
				"and she has a similar reaction.\n\n"
				"Looks like you've assured yourself the job of your choice.",
				FG_GREY );
			
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

// Ruin scenes
void laboratoryScene( void )
{
	// on successful search you get great victory, otherwise you get normal victory
	startScene( );

	Choice* sbChoices = NULL;

	int lightBonus = 0;
	if( ( character.gear[G_FLASHLIGHT] > 0 ) || ( character.gear[G_LANTERN] > 0 ) ) {
		lightBonus = 15;
	}

	pushSkillBasedChoice( &sbChoices, "Search the laboratory. (Investigate)", SKL_INVESTIGATE, 25 - lightBonus,
		greatVictoryScene, victoryScene, victoryScene );
	pushSkillBasedChoice( &sbChoices, "Recall general lab layouts. (Knowledge)", SKL_KNOWLEDGE, 25 - lightBonus,
		greatVictoryScene, victoryScene, victoryScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			if( ( character.gear[G_FLASHLIGHT] > 0 ) || ( character.gear[G_LANTERN] > 0 ) ) {
				pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You head in deeper into the building. As the light starts to fade you turn on "
					"your %s. You see what looks like a laboratory full of devices and notes.\n\n"
					"What do you do?",
					FG_GREY,
					( character.gear[G_FLASHLIGHT] > 0 ) ? gearData[G_FLASHLIGHT].name : gearData[G_LANTERN].name );
			} else {
				pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You head in deeper into the building. The deeper you get the darker it's getting, "
					"making it difficult to see. Eventually you enter what you think is a laboratory. "
					"Seeing what you're doing is quite difficult.\n\n"
					"What do you do?",
					FG_GREY );
			}
			
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void securityRepeatScene( void );

void securityRunSuccessScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Keep going deeper.", laboratoryScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You wonder whether the turret tracks people or just movement. You grab a "
					"small piece of rubble and throw it out from behind cover. The gun shoots "
					"at where you threw the rock. You grab a few more rocks and jump from behind "
					"cover and immediately throw a rock down the hallway. While the gun is busy "
					"shooting at nothing you charge down the hallway. When the turret starts facing "
					"towards you again you throw the second rock. This gives you enough time to get "
					"into the turret's blind spot and disable it.",
					FG_GREY );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void securityRunCostlySuccessScene( void )
{
	// gain a wound
	if( !fromStatusScreen ) {
		gainPhysicalWound( "Shot by a Turret" );
	}
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Keep going deeper.", laboratoryScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You figure if you run in a straight line down the hall the turret will "
					"have any easy time tracking you. So after jumping out of cover you follow "
					"a serpentine path down the corridor. You're able to get into the turret's "
					"blind spot and disable it easily enough. After the adrenaline wears off "
					"you notice you're bleeding.",
					FG_GREY );
			standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were shot by a turret" );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void securityRunFailureScene( void )
{
	// gain a wound, fight continues
	if( !fromStatusScreen ) {
		gainPhysicalWound( "Shot by a Turret" );
	}
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue fighting!", securityRepeatScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You charge out from behind cover and plan to run straight ahead. But the turret "
					"fires at you before you can carry through. You get back behind cover with only "
					"a single nasty wound.",
					FG_GREY );
			standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were shot by a turret" );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void securityShootSuccessScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Keep going deeper.", laboratoryScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"The thing is fast, but you wonder if it's tracking entities or just movement. "
					"You toss out a piece of rubble from behind cover and hear the turret start up and shoot. "
					"So you quickly peek around the corner, seeing where you need to aim. You take "
					"a couple deep breaths and throw another rock down the hall. While it's busy "
					"you take aim and send it crashing to the ground.",
					FG_GREY );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void securityShootCostlySuccessScene( void )
{
	// gain a wound
	if( !fromStatusScreen ) {
		gainPhysicalWound( "Shot by a Turret" );
	}
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Keep going deeper.", laboratoryScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"The thing seems to be quite fast, but you calm yourself and ready your shot. "
					"Quickly you lean around the corner, aim, and shoot. At the same time the turret "
					"lets loose some shots. When you come to the turret is on the floor and your "
					"bleeding has stopped.",
					FG_GREY );
			standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were shot by a turret" );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void securityShootFailureScene( void )
{
	// gain a wound, fight continues
	if( !fromStatusScreen ) {
		gainPhysicalWound( "Shot by a Turret" );
	}
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue fighting!", securityRepeatScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"Every time you think you have a shot lined up the turret's gun starts "
					"firing. One time you stay out a little too long and get hit in the arm.",
					FG_GREY );
			standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were shot by a turret" );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void securityWarriorScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Go deeper into the building.", laboratoryScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"%s looks at you, smiles, and says it's a good day to die. Grabbing a large piece "
					"of metal he runs out getting the turret's attention. The metal seems to be quite "
					"strong and resists most of the bullets. While it's distracted you sneak in and "
					"are able to disable the turret. %s has a few wounds but he just shrugs them off.",
					FG_GREY,
					companionsData[CMP_WARRIOR].name, companionsData[CMP_WARRIOR].name );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void securityRepeatScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;

	pushSkillBasedChoice( &sbChoices, "Make a run for it. (Acrobatics)", SKL_ACROBATICS, 18,
		securityRunSuccessScene, securityRunCostlySuccessScene, securityRunFailureScene );
	if( bestShootWeapon( ) >= 0 ) {
		pushSkillBasedChoice( &sbChoices, "Take it out from range. (Shoot)", SKL_SHOOT, 18 - bestFightWeaponBonus( ),
			securityShootSuccessScene, securityShootCostlySuccessScene, securityShootFailureScene );
	}

	if( character.hasCompanion[CMP_WARRIOR] ) {
		pushSimpleChoice( &sbChoices, "Send the warrior to distract it.", securityWarriorScene );
	}

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"The turret is still active and blocking your way.\n\n"
					"What do you do?",
					FG_GREY );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void securityScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;

	pushSkillBasedChoice( &sbChoices, "Make a run for it. (Acrobatics)", SKL_ACROBATICS, 18,
		securityRunSuccessScene, securityRunCostlySuccessScene, securityRunFailureScene );
	if( bestShootWeapon( ) >= 0 ) {
		pushSkillBasedChoice( &sbChoices, "Take it out from range. (Shoot)", SKL_SHOOT, 18 - bestFightWeaponBonus( ),
			securityShootSuccessScene, securityShootCostlySuccessScene, securityShootFailureScene );
	}

	if( character.hasCompanion[CMP_WARRIOR] ) {
		pushSimpleChoice( &sbChoices, "Send the warrior to distract it.", securityWarriorScene );
	}

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"Going deeper and deeper you eventually come to what looks like an important door. As "
					"soon as you touch it a turret comes out of the cieling and starts tracking you. It's "
					"not fast enough though and you are able to get behind some cover before it starts "
					"shooting.\n\n"
					"What do you do?",
					FG_GREY );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void entranceRepeatScene( void );

void entranceFightSuccessScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Go deeper into the building.", securityScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You quickly dodge under the robot, figuring it won't be able to aim at you as well "
					"from there. While it's trying to track your position you get a shot off on "
					"it's sensors. Blinded, the hulk begins firing wildly causing a large chunk of the "
					"building to fall on top of it. You carefully walk over and make sure it's dead.",
					FG_GREY );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void entranceFightCostlySuccessScene( void )
{
	// gain a wound
	if( !fromStatusScreen ) {
		gainPhysicalWound( "Flattened by a Robot" );
	}
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Go deeper into the building.", securityScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You have a long and grueling battle with the security robot. "
					"It was able to get a few goods shots at you, hitting you and knocking "
					"you behind some debris. You head around the other side of it and are "
					"able to get the drop on it. It takes a few well placed shots at it's "
					"joints, but it eventually falls.",
					FG_GREY );
			standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were shot by a robot" );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void entranceFightFailureScene( void )
{
	// gain a wound, fight continues
	if( !fromStatusScreen ) {
		gainPhysicalWound( "Flattened by a Robot" );
	}
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue fighting!", entranceRepeatScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"This thing must be invincible. No matter what you throw at it "
					"everything seems to just glance off it's armor. Your skin isn't "
					"nearly as resilant to it's guns.",
					FG_GREY );
			standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were shot by a robot" );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void entranceUseBanditsScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Go deeper into the building.", securityScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You send out %s to lure the brute away. They laugh and run out, getting it's "
					"attention before ducking behind cover. It follows them down a hall. After "
					"about 15 minutes you hear what sounds like a large explosion and the building "
					"shakes. Shortly after %s return, covered in soot.",
					FG_GREY,
					companionsData[CMP_BANDITS].name, companionsData[CMP_BANDITS].name );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void entranceHackScene( void )
{
	if( !fromStatusScreen ) {
		--character.gear[G_HACKING_DEVICE];
	}
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Go deeper into the building.", securityScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You run up to the guard robot before it can react and plug a %s into it and "
					"start hitting random buttons on it. The guard powers down giving you access "
					"to the compound.",
					FG_GREY );
			standardGearLossText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, G_HACKING_DEVICE );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void entranceUseRedScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Go deeper into the building.", securityScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"Red starts spitting out what seems like a bunch of noise. The guard robot "
					"responds with similar noise. After a short pause the guard lowers it's weapons "
					"and grants you access.",
					FG_GREY );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void entranceRepeatScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;

	pushSkillBasedChoice( &sbChoices, "Hack it to pieces. (Fight)", SKL_FIGHT, 20 - bestFightWeaponBonus( ),
		entranceFightSuccessScene, entranceFightCostlySuccessScene, entranceFightFailureScene );
	if( bestShootWeapon( ) >= 0 ) {
		pushSkillBasedChoice( &sbChoices, "Shoot it up. (Fight)", SKL_SHOOT, 20 - bestFightWeaponBonus( ),
			entranceFightSuccessScene, entranceFightCostlySuccessScene, entranceFightFailureScene );
	}

	if( character.hasCompanion[CMP_BANDITS] ) {
		pushSimpleChoice( &sbChoices, "Send the bandits to deal with it.", entranceUseBanditsScene );
	}

	if( character.gear[G_HACKING_DEVICE] > 0 ) {
		pushSimpleChoice( &sbChoices, "Use a strange device on the robot.", entranceUseRedScene );
	}

	if( character.hasCompanion[CMP_ROBOT] ) {
		pushSimpleChoice( &sbChoices, "Have your robot talk to it.", entranceUseRedScene );
	}

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"The guard robot is still blocking the way deeper into the building.\n\n"
					"What do you do?",
					FG_GREY );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void entranceScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;

	pushSkillBasedChoice( &sbChoices, "Hack it to pieces. (Fight)", SKL_FIGHT, 20 - bestFightWeaponBonus( ),
		entranceFightSuccessScene, entranceFightCostlySuccessScene, entranceFightFailureScene );
	if( bestShootWeapon( ) >= 0 ) {
		pushSkillBasedChoice( &sbChoices, "Shoot it up. (Shoot)", SKL_SHOOT, 20 - bestFightWeaponBonus( ),
			entranceFightSuccessScene, entranceFightCostlySuccessScene, entranceFightFailureScene );
	}

	if( character.hasCompanion[CMP_BANDITS] ) {
		pushSimpleChoice( &sbChoices, "Send the bandits to deal with it.", entranceUseBanditsScene );
	}

	if( character.gear[G_HACKING_DEVICE] > 0 ) {
		pushSimpleChoice( &sbChoices, "Use a strange device on the robot.", entranceHackScene );
	}

	if( character.hasCompanion[CMP_ROBOT] ) {
		pushSimpleChoice( &sbChoices, "Have your robot talk to it.", entranceUseRedScene );
	}

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"Entering into the building you see some clear paths through the rubble. "
					"While deciding which way to go you hear something move and see a large robot "
					"rise up out of the rubble. It turns towards and starts warming up it's weapons."
					"You have no choice, you have to get through here and it's blocking the way.\n\n"
					"What do you do?",
					FG_GREY );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void powerRobotSuccessfulScene( void )
{
	if( !fromStatusScreen ) {
		character.hasCompanion[CMP_ROBOT] = true;
	}

	startScene( );
	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Head into the building.", entranceScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"The robot starts up quickly, running through what seems to be a vocal diagnostic "
					"before turning to look at you. It introduces itself as Reactive Exploration Drone "
					"Model Gamma Production Number 0-1-2 and asks if you're in need of assistance. "
					"You decide to call it Red. It finds this acceptable.",
					FG_GREY );
			drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
				"You've gained %s as a companion!", FG_GREEN, companionsData[CMP_ROBOT].name );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void powerRobotDeviceScene( void )
{
	if( !fromStatusScreen ) {
		--character.gear[G_HACKING_DEVICE];
	}
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Hopefully it's friendly.", powerRobotSuccessfulScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"Looking through at the robot you see some ports on it that match the %s you "
					"have. Plugging it in the robot starts booting up.",
					FG_GREY,
					gearData[G_HACKING_DEVICE].name );
			standardGearLossText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, G_HACKING_DEVICE );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void powerRobotScholarScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Good job.", powerRobotSuccessfulScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"%s looks at the robot then takes a small tool kit out of her robes. After a "
					"few minutes of tinkering and explaining in detail the fascinating electronics "
					"in the robot she stops. She's puzzled for a short while before calling the robot "
					"a stupid piece of junk and kicking it. A few seconds later the robot begins to "
					"boot up.",
					FG_GREY );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void powerRobotCostlySuccessScene( void )
{
	if( !fromStatusScreen ) {
		character.hasCompanion[CMP_ROBOT] = true;
		gainPhysicalWound( "Electrocuted" );
	}

	startScene( );
	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Head into the building.", entranceScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"It takes more work than you thought to get the robot running. You get also get a "
					"very nasty burn from a shock.\n\n"
					"The robot starts up quickly, running through what seems to be a vocal diagnostic "
					"before turning to look at you. It introduces itself as Reactive Exploration Drone "
					"Model Gamma Production Number 0-1-2 and asks if you're in need of assistance. "
					"You decide to call it Red. It finds this acceptable.",
					FG_GREY );
			pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
				"You've gained %s as a companion!", FG_GREEN, companionsData[CMP_ROBOT].name );
			standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were electroted" );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void powerRobotFailureScene( void )
{
	if( !fromStatusScreen ) {
		gainPhysicalWound( "Electrocuted" );
	}

	startScene( );
	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Head into the building.", entranceScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You spend an hour fiddling with the robot, trying to get it to work. At one "
					"point you think you have it, but just end getting a really nasty shock. You "
					"kick the robot, and swear at it. The insides start whirring and it starts to "
					"boot up.\n\n"
					"Or at least it seems to. Shortly after smoke starts pouring out of it's chassis. "
					"Looks like it's a lost cause.",
					FG_GREY );
			standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were electroted" );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void outsideBuildingScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;

	pushSkillBasedChoice( &sbChoices, "Attempt to fix the robot. (Craft)", SKL_CRAFT, 20,
		powerRobotSuccessfulScene, powerRobotCostlySuccessScene, powerRobotFailureScene );
	if( character.gear[G_HACKING_DEVICE] > 0 ) {
		pushSimpleChoice( &sbChoices, "Hook up a strange device to the robot.", powerRobotDeviceScene );
	}
	if( character.hasCompanion[CMP_SCHOLAR] ) {
		pushSimpleChoice( &sbChoices, "Let the scholar try to fix the robot.", powerRobotScholarScene );
	}

	pushSimpleChoice( &sbChoices, "Enter the building.", entranceScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You find a mostly intact building with a large symbol on it. You recognize at some "
					"sort of building from an ancient government. In front of the building is a large "
					"pile of debris, including a mostly intact robot.",
					FG_GREY );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void ruinsIntroScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Make your way into the ruins.", outsideBuildingScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"Before you stands the remains of what must have been a bustling metropolis many "
					"generations ago. You carefully pick your way through the ruins, on the alert for "
					"the people and creatures that like to inhabit such places.",
					FG_GREY );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

// Wilderness scenes, you will have a number of days you have to travel, with random encounters along the way
int timeLeft;
void riverScene( void );
void travelerScene( void );
void giantCatScene( void );
void madmanScene( void );
void giantSpidersScene( void );
void shrineScene( void );
void cacheScene( void );
void sirenScene( void );
void cliffScene( void );
void banditsScene( void );
void wanderScene( void );

Scene wildernessScenes[17];

void setupWilderness( void )
{
	int i = 0;
	for( ; i < 7; ++i ) {
		wildernessScenes[i] = wanderScene;
	}
	wildernessScenes[i++] = riverScene;
	wildernessScenes[i++] = travelerScene;
	wildernessScenes[i++] = giantCatScene;
	wildernessScenes[i++] = madmanScene;
	wildernessScenes[i++] = giantSpidersScene;
	wildernessScenes[i++] = shrineScene;
	wildernessScenes[i++] = cacheScene;
	wildernessScenes[i++] = sirenScene;
	wildernessScenes[i++] = cliffScene;
	wildernessScenes[i++] = banditsScene;
	SHUFFLE( wildernessScenes, Scene );
	timeLeft = 16;
}

void gotoNextWildernessScene( void )
{
	timeLeft -= 2;
	if( timeLeft <= 0 ) {
		currentScene = ruinsIntroScene;
	} else {
		int idx = 0;
		while( wildernessScenes[idx] == NULL ) {
			++idx;
		}

		if( idx < ARRAYSIZE( wildernessScenes ) ) {
			currentScene = wildernessScenes[idx];
			wildernessScenes[idx] = NULL;
		} else {
			currentScene = wanderScene;
		}
	}
}

void riverBridgeScene( void )
{
	if( !fromStatusScreen ) {
		timeLeft += 2;
	}

	startScene( );

	Choice* sbChoices = NULL;
	
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You spend half a day walking along the shore and find another bridge.",
					FG_GREY );

			drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
					"You lost a day worth of travel...",
					FG_RED );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void riverMapSuccessScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;
	
	pushSimpleChoice( &sbChoices, "Continue on your journey", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You find a place on the map where the river widens out and is nearby. "
					"The water only reaches up to your knees here and isn't too fast so "
					"crossing is easy.",
					FG_GREY );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void riverMapCostlySuccessScene( void )
{
	// physical wound, unless you have a towel
	if( !fromStatusScreen ) {
		if( character.gear[G_TOWEL] == 0 ) {
			gainNonArmorPhysicalWound( "Drowned" );
		}
	}

	startScene( );

	Choice* sbChoices = NULL;
	
	pushSimpleChoice( &sbChoices, "Continue on your journey", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"Following the map you find a spot in the river that has lots of rocks so you can "
					"brace yourself while crossing. You're able to eventually make it across but it "
					"takes a lot longer than you'd like and you end up soaked to the bone.",
					FG_GREY );
			if( character.gear[G_TOWEL] > 0 ) {
				pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
					"Luckily you have your trusty towel and are able to dry yourself off before you "
					"catch a cold.",
					FG_GREEN );
			} else {
				standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "start to feel sick" );
			}

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void riverMapFailureScene( void )
{
	// physical wound, unless you have a towel
	if( !fromStatusScreen ) {
		if( character.gear[G_TOWEL] == 0 ) {
			gainNonArmorPhysicalWound( "Drowned" );
		}
	}

	startScene( );

	Choice* sbChoices = NULL;
	
	pushSimpleChoice( &sbChoices, "You'll have to find a bridge.", riverBridgeScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"Following the map you find a spot in the river that is thin enough that you "
					"think it should be easy to cross. You are quickly overwhelmed while wading "
					"through and turn back.",
					FG_GREY );
			if( character.gear[G_TOWEL] > 0 ) {
				pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
					"Luckily you have your trusty towel and are able to dry yourself off before you "
					"catch a cold.",
					FG_GREEN );
			} else {
				standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "start to feel sick" );
			}

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void riverRopeSwimSuccessScene( void )
{
	// physical wound, unless you have a towel, lose rope
	if( !fromStatusScreen ) {
		--character.gear[G_ROPE];
	}

	startScene( );

	Choice* sbChoices = NULL;
	
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You tie the rope to a nearby tree and around yourself as well. You start wading "
					"through a shallower part of the river. You're able to keep your footing all the "
					"way across.\n\n"
					"You untie the rope and leave it behind.",
					FG_GREY );
			standardGearLossText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, G_ROPE );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void riverRopeSwimCostlySuccessScene( void )
{
	// physical wound, unless you have a towel, lose rope
	if( !fromStatusScreen ) {
		--character.gear[G_ROPE];
		if( character.gear[G_TOWEL] == 0 ) {
			gainNonArmorPhysicalWound( "Drowned" );
		}
	}

	startScene( );

	Choice* sbChoices = NULL;
	
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You tie the rope to a nearby tree and around yourself as well. You start wading "
					"through a shallower part of the river. You get most of the way across before you "
					"slip and the river starts to carry you away. You're able to stop yourself with the "
					"rope and get to the other shore. "
					"Although you're completely soaked and shivering.\n\n"
					"You untie the rope and leave it behind.",
					FG_GREY );
			if( character.gear[G_TOWEL] > 0 ) {
				pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
					"Luckily you have your trusty towel and are able to dry yourself off before you "
					"catch a cold.",
					FG_GREEN );
			} else {
				pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "start to feel sick" );
			}
			standardGearLossText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, G_ROPE );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void riverRopeFailureScene( void )
{
	// physical wound, unless you have a towel
	if( !fromStatusScreen ) {
		if( character.gear[G_TOWEL] == 0 ) {
			gainNonArmorPhysicalWound( "Drowned" );
		}
	}

	startScene( );

	Choice* sbChoices = NULL;
	
	pushSimpleChoice( &sbChoices, "You'll have to find a bridge.", riverBridgeScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You tie the rope to a nearby tree and around yourself as well. You start wading "
					"through a shallower part of the river. Only a little bit in you slip and are carried "
					"down the river. Luckily you're able to pull yourself back along the rope to shore. "
					"Although you're completely soaked and shivering.",
					FG_GREY );
			if( character.gear[G_TOWEL] > 0 ) {
				pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
					"Luckily you have your trusty towel and are able to dry yourself off before you "
					"catch a cold.",
					FG_GREEN );
			} else {
				standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "start to feel sick" );
			}

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void riverSwimSuccessScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;
	
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You fasten all your gear and attempt to swim across the river. You're able to "
					"power through the river faster than you thought and get to the other side "
					"with an issue.",
					FG_GREY );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void riverSwimCostlySuccessScene( void )
{
	// physical wound, unless you have a towel
	if( !fromStatusScreen ) {
		if( character.gear[G_TOWEL] == 0 ) {
			gainNonArmorPhysicalWound( "Drowned" );
		}
	}

	startScene( );

	Choice* sbChoices = NULL;
	
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You fasten all your gear and attempt to swim across the river. More than half way across "
					"an undercurrent catches you. You barely make it to the other shore. "
					"Although you're completely soaked and shivering.",
					FG_GREY );
			if( character.gear[G_TOWEL] > 0 ) {
				pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
					"Luckily you have your trusty towel and are able to dry yourself off before you "
					"catch a cold.",
					FG_GREEN );
			} else {
				standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "start to feel sick" );
			}

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void riverSwimFailureScene( void )
{
	// physical wound, unless you have a towel
	if( !fromStatusScreen ) {
		if( character.gear[G_TOWEL] == 0 ) {
			gainNonArmorPhysicalWound( "Drowned" );
		}
	}

	startScene( );

	Choice* sbChoices = NULL;
	
	pushSimpleChoice( &sbChoices, "You'll have to find a bridge.", riverBridgeScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You fasten all your gear and attempt to swim across the river. Half way across "
					"an undercurrent catches you. You're barely able to make it back to shore. "
					"Although you're completely soaked and shivering.",
					FG_GREY );
			if( character.gear[G_TOWEL] > 0 ) {
				pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
					"Luckily you have your trusty towel and are able to dry yourself off before you "
					"catch a cold.",
					FG_GREEN );
			} else {
				standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "start to feel sick" );
			}

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void riverScene( void )
{
	// use map, rope, swim, or find bridge
	startScene( );

	Choice* sbChoices = NULL;
	
	pushSkillBasedChoice( &sbChoices, "Swim across. (Acrobatics)", SKL_ACROBATICS, 12,
		riverSwimSuccessScene, riverSwimCostlySuccessScene, riverSwimFailureScene );
	if( character.gear[G_ROPE] > 0 ) {
		pushSkillBasedChoice( &sbChoices, "Use your rope to get across. (Acrobatics)", SKL_BARTER, 10,
			riverRopeSwimSuccessScene, riverRopeSwimCostlySuccessScene, riverRopeFailureScene );
	}
	if( character.gear[G_MAP] > 0 ) {
		pushSkillBasedChoice( &sbChoices, "Use your map to find a path. (Survival)", SKL_BARTER, 8,
			riverMapSuccessScene, riverMapCostlySuccessScene, riverMapFailureScene );
	}
	pushSimpleChoice( &sbChoices, "Find a bridge.", riverBridgeScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"Following the path you see the bridge you were supposed to cross is no longer "
					"there.\n\n"
					"What do you do?",
					FG_GREY );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void travelerTradeSuccessScene( void )
{
	// lose money, gain two items
	static Gear gOne, gTwo;
	if( !fromStatusScreen ) {
		--character.gear[G_MONEY];

		do {
			gOne = rand( ) % NUM_GEAR;
		} while( gOne == G_MONEY );
		++character.gear[gOne];

		do {
			gTwo = rand( ) % NUM_GEAR;
		} while( gTwo == G_MONEY );
		++character.gear[gTwo];
	}
	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You both haggle for a while. After a few minutes you have her eating out of the "
					"palm of your hand. She is willing to part with more than you were expecting. She "
					"walks off smiling and counting the cash you gave her.",
					FG_GREY );
			pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, gOne );
			pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, gOne );
			standardGearLossText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, G_MONEY );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
}

void travelerTradeCostlySuccessScene( void )
{
	// lose money, gain item
	static Gear newGear;
	if( !fromStatusScreen ) {
		--character.gear[G_MONEY];
		do {
			newGear = rand( ) % NUM_GEAR;
		} while( newGear == G_MONEY );
		++character.gear[newGear];
	}
	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You both haggle for a while. After a few minutes you're both able to come "
					"to something that satisfies you both. She leaves smiling and counting the "
					"cash you handed her.",
					FG_GREY );
			pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, newGear );
			standardGearLossText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, G_MONEY );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
}

void travelerTradeFailureScene( void )
{
	// social wound
	if( !fromStatusScreen ) {
		gainSocialWound( );
	}
	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You both haggle for a while. Eventually she becomes frustrated and walks away.",
					FG_GREY );
			standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Social", "were unable to seal the deal" );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void travelerTravelScene( void )
{
	// extra day of travel
	if( !fromStatusScreen ) {
		timeLeft -= 2;
	}
	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"While walking you talk about your history and the trial in front of you. When you "
					"mention where your headed she perks up and says she knows about a shortcut. She "
					"points out a nearly hidden path as you approach it. You thank her and take the path "
					"she pointed out.",
					FG_GREY );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void travelerScene( void )
{
	// gain a day of travel, or trade money for a random item
	startScene( );

	Choice* sbChoices = NULL;

	if( character.gear[G_MONEY] > 0 ) {
		pushSkillBasedChoice( &sbChoices, "Offer to buy some of her gear. (Barter)", SKL_BARTER, 15,
			travelerTradeSuccessScene, travelerTradeCostlySuccessScene, travelerTradeFailureScene );
	}
	pushSimpleChoice( &sbChoices, "Travel together for a while.", travelerTravelScene );
	pushSimpleChoice( &sbChoices, "Ignore her and continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"At a fork in the road a traveller starts heading the same way as you. The both of "
					"you talk for a while. You and her seem to be headed in the same direction.\n\n"
					"What do you do?",
					FG_GREY );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void giantCatFightMeleeSuccess( void )
{
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"The cat pounces but you're ready. You sidestep the pounce and get a good shot in, "
					"knocking the cat to the ground. It gets up and runs off.",
					FG_GREY );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void giantCatFightMeleeCostlySuccess( void )
{
	if( !fromStatusScreen ) {
		gainPhysicalWound( "Eaten by a Large Animal" );
	}
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"The cat pounces but you're ready. You brace yourself and redirect the impact "
					"causing the cat to hit the ground hard. While it's down you get a good shot in "
					"on it. Once it's able to get up it takes off into the forest.",
					FG_GREY );
			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were mauled" );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void giantCatFightMeleeFailure( void )
{
	if( !fromStatusScreen ) {
		gainPhysicalWound( "Eaten by a Large Animal" );
		gainPhysicalWound( "Eaten by a Large Animal" );
	}
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"The cat pounces before you're ready. It lands right on you. After escaping with "
					"a few gashes you hit it in the side of the head. The cat staggers for a bit and "
					"then runs off.",
					FG_GREY );
			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were mauled" );
			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were mauled" );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void giantCatFightShootSuccess( void )
{
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You quickly aim and shoot. The cat is hit in the shoulder and before you can "
					"get off another shot it's run off.",
					FG_GREY );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void giantCatFightShootCostlySuccess( void )
{
	if( !fromStatusScreen ) {
		gainPhysicalWound( "Eaten by a Large Animal" );
	}
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"The cat pounces at you while you were still aiming. Your shot hits it in the shoulder "
					"but it's momentum keeps it going, although the resulting gash isn't nearly as deep as it "
					"could have been. While you're getting up it runs off.",
					FG_GREY );
			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were mauled" );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void giantCatFightShootFailure( void )
{
	if( !fromStatusScreen ) {
		gainPhysicalWound( "Eaten by a Large Animal" );
		gainPhysicalWound( "Eaten by a Large Animal" );
	}
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"The cat pounces before you can even draw your weapon. You try to get out of the way "
					"but it still gets a few hits in. Quickly pulling back you fire and hit the ground "
					"next to the cat. This seems to be sufficient to scare it off however.",
					FG_GREY );
			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were mauled" );
			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were mauled" );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void giantCatSmartSuccess( void )
{
	startScene( );
	Choice* sbChoices = NULL;
	
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You know exactly what to do. Slowly you back away while maintaining eye contact. "
					"Raising your arms up to make yourself look bigger you start yelling at the cat "
					"as loudly as you can. The cat continues watching you for a while before deciding "
					"you're not worth the trouble.",
					FG_GREY );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void giantCatSmartCostlySuccess( void )
{
	static Gear lostGear;
	if( !fromStatusScreen ) {
		if( totalGearCount( ) > 0 ) {
			lostGear = randomGearToLose( );
			--character.gear[lostGear];
		} else {
			lostGear = -1;
			gainPhysicalWound( "Eaten by a Large Animal" );
		}
	}
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			if( lostGear > 0 ) {
				pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
						"You know that these cats will attack if you look away or run from them. But "
						"you have nothing you can throw at it. So you quickly kneel down and pick up "
						"large branch. The cat takes the opportunity to attack. It gets a good hit on "
						"you but after you're able to use the branch to keep it at a distance. It "
						"eventually gives up and runs into the forest.",
						FG_GREY );
				pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were mauled" );
			} else {
				pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
						"You know that these cats will attack if you look away or run from them. So "
						"without losing eye contact you grab something from your pack and throw it at "
						"the cat. It runs off. You search for what you threw for an hour before giving "
						"up.",
						FG_GREY );
				standardGearLossText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, lostGear );
			}
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void giantCatSmartFailure( void )
{
	if( !fromStatusScreen ) {
		gainPhysicalWound( "Eaten by a Large Animal" );
		gainPhysicalWound( "Eaten by a Large Animal" );
	}
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You remember something about some large animals ignoring you if you play dead. "
					"It doesn't work. The cat is able to get both it's claws into you before you "
					"punch it in the face. This dislodges the cat and it runs off.",
					FG_GREY );
			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were mauled" );
			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were mauled" );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void giantCatScene( void )
{
	// scare off, or attack
	startScene( );

	Choice* sbChoices = NULL;

	pushSkillBasedChoice( &sbChoices, "Attack! (Fight)", SKL_FIGHT, 15 - bestFightWeaponBonus( ),
		giantCatFightMeleeSuccess, giantCatFightMeleeCostlySuccess, giantCatFightMeleeFailure );
	if( bestShootWeapon( ) > 0 ) {
		pushSkillBasedChoice( &sbChoices, "Attack! (Shoot)", SKL_FIGHT, 15 - bestShootWeaponBonus( ), 
			giantCatFightShootSuccess, giantCatFightShootCostlySuccess, giantCatFightShootFailure );
	}
	pushSkillBasedChoice( &sbChoices, "Try to outsmart it. (Knowledge)", SKL_KNOWLEDGE, 10, 
		giantCatSmartSuccess, giantCatSmartCostlySuccess, giantCatSmartFailure );

	COORD pos;
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"Walking along in the woods you suddenly see two large slit eyes looking at you "
					"from the foliage. An extremely large cat is staring at you and looks ready "
					"to pounce.\n\n"
					"What do you do?",
					FG_GREY );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void madmanScene( void )
{
	// gain a mental wound, social wound, or a random item
	static int type;
	static Gear newGear;
	static int cOne;
	static int cTwo;
	if( !fromStatusScreen ) {
		newGear = -1;
		if( numCompanions( ) > 1 ) {
			// if we have multiple companions gain a social wound
			type = 0;
			gainSocialWound( );
			cOne = getRandomCompanion( );
			cTwo = getRandomCompanion( );
		} else if( numCompanions( ) == 1 ) {
			// if we have one companion gain a mental wound
			type = 1;
			gainMentalWound( );
			cOne = getRandomCompanion( );
		} else {
			// if we have none than gain a random item
			type = 2;
			newGear = rand( ) % NUM_GEAR;
			++character.gear[newGear];
		}
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"While walking along the path you hear some chanting coming from farther ahead. "
					"A few minutes later you come upon an old naked man sitting in the mud and "
					"chanting. When you start getting close he suddenly stops and looks up at you.",
					FG_GREY );

			if( type == 0 ) {
				pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
						"He points at you and %s, making embarrassing insinuations about the two of you. "
						"%s laughs a bit before you all continue on your way.",
						FG_GREY,
						companionsData[cOne].name, companionsData[cTwo].name );
				standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Social", "were embarrassed" );
			} else if( type == 1 ) {
				pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
						"You see that it isn't just mud that he's sitting in. He picks up some of the "
						"the brown stuff and flings it at you, hitting you square in the face. %s stands "
						"aghast as you wipe off your face.%s",
						FG_GREY,
						companionsData[cOne].name,
						( character.gear[G_TOWEL] > 0 ) ? " Luckily you have your trusty Towel with you." : "" );
				standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Mental", "had \"mud\" thrown at you" );
			} else {
				pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
						"Suddenly he seems sad, mutters something about how nobody should be so alone. "
						"He then hands you something wrapped in an old, crusty blanket. A tear rolls down "
						"his soiled face as he says that she has kept him company for a long time, but you "
						"need her more.",
						FG_GREY );
				standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, newGear );
			}

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void giantSpiderGoAroundScene( void )
{
	if( !fromStatusScreen ) {
		timeLeft += 2;
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You spend a day searching, but eventually you find the path that goes over "
					"instead of through the cave.",
					FG_GREY );

			drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
				"You lose a day worth of travel...", FG_RED );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void giantSpiderSneakSuccessScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You carefully watch where you're stepping and make your way through the cave. It "
					"takes you longer than you'd like and more close calls than you'd like. You emerge "
					"on the other side in one piece.",
					FG_GREY );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void giantSpiderSneakCostlySuccessScene( void )
{
	if( !fromStatusScreen ) {
		gainNonArmorPhysicalWound( "Eaten by Spiders" );
	}
	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You carefully watch where you're stepping and make your way through the cave. While "
					"waiting for some spiders to leave an area you slip and fall onto a web. Immediately "
					"a horde of spiders emerges. You run faster than you've run before through the rest "
					"of the cave. Luckily you were near the end and quickly get into sunlight. You continue "
					"running the rest of the day.",
					FG_GREY );
			standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "are exhausted from too much running" );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void giantSpiderSneakFailureScene( void )
{
	if( !fromStatusScreen ) {
		gainPhysicalWound( "Eaten by Spiders" );
		gainMentalWound( );
	}
	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "You'll have to go around.", giantSpiderGoAroundScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You carefully watch where you're stepping and make your way through the cave. While "
					"waiting for some spiders to leave an area you slip and fall onto a web. Immediately "
					"a horde of spiders emerges. Unable to get up fast enough you get swarmed and bitten. "
					"You scramble back the way you came from and reach the entrace before you're completely "
					"overcome.",
					FG_GREY );
			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were bitten" );
			standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Mental", "were almost eaten" );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void giantSpiderFireScene( void )
{
	if( !fromStatusScreen ) {
		--character.gear[G_LANTERN];
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"The lantern crashes into the ground. It seems to almost burn out before all the "
					"webs burst into flame. Horrible shrieking emits from the cave for the next hour "
					"and a half. After two hours the shrieking dies down and the flames seem to have "
					"completely gone out.\n\n"
					"The cave is nice and warm as you make your way through it.",
					FG_GREY );
			standardGearLossText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, G_LANTERN );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void giantSpidersScene( void )
{
	// sneak around
	// set on fire
	// go around it
	startScene( );

	Choice* sbChoices = NULL;
	pushSkillBasedChoice( &sbChoices, "Attempt to sneak through. (Stealth)", SKL_STEALTH, 8,
		giantSpiderSneakSuccessScene, giantSpiderSneakCostlySuccessScene, giantSpiderSneakFailureScene );
	if( character.gear[G_LANTERN] > 0 ) {
		pushSimpleChoice( &sbChoices, "Throw your lantern down there.", giantSpiderFireScene );
	}
	pushSimpleChoice( &sbChoices, "Find a way around it.", giantSpiderGoAroundScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"The path you're following seems to dip down into a cave for a short while. "
					"Looking down you see the entire area is filled with webs and you see a "
					"spider the size of your torso crawling down there.\n\n"
					"What do you do?",
					FG_GREY );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void shrineScene( void )
{
	// recover from a wound of one type
	static int type;
	static boolean noWounds;
	static char* name;
	if( !fromStatusScreen ) {
		// first social, then mental, then physical
		noWounds = false;
		if( character.wounds_social > 0 ) {
			type = 0;
			name = "Social";
			--character.wounds_social;
		} else if( character.wounds_mental > 0 ) {
			type = 1;
			name = "Mental";
			--character.wounds_mental;
		} else if( character.wounds_physical > 0 ) {
			type = 2;
			name = "Physical";
			--character.wounds_physical;
		} else {
			noWounds = true;
		}
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"While walking you come upon a shrine on the side of the road. Your "
					"training instinctively takes over and you kneel before it, saying a "
					"short prayer to The Hidden Emperor. After asking for protection and "
					"safety to help bring back something that will help people you feel "
					"slightly better.",
					FG_GREY );
			if( !noWounds ) {
				pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
					"You lose a %s Wound!", FG_GREEN, name );
			}

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void cacheSuccessScene( void )
{
	// get the items
	static Gear gOne;
	static Gear gTwo;
	if( !fromStatusScreen ) {
		gOne = rand( ) % NUM_GEAR;
		gTwo = rand( ) % NUM_GEAR;
		++character.gear[gOne];
		++character.gear[gTwo];
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"After you take the bolts out of the hinges your able to open up the box. You "
					"find some items inside.",
					FG_GREY );
			pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, gOne );
			standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, gTwo );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void cacheCostlySuccessScene( void )
{
	// lose a day but get the items
	if( !fromStatusScreen ) {
		++timeLeft;
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "What's in the box?!", cacheSuccessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You spend the better part of a day trying to break it open, but nothing you "
					"do seems to work at all. Finally you notice that hinges are on the outside. "
					"You pry out the bolts holding it in and open the box.",
					FG_GREY );
			drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
				"You lost about half a day of time...", FG_RED );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void cacheFailureScene( void )
{
	// lose time and gain a mental wound
	if( !fromStatusScreen ) {
		gainMentalWound( );
		++timeLeft;
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You spend the better part of a day trying to break it open, but nothing you "
					"do seems to work at all. Finally you give up in frustration.",
					FG_GREY );
			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
				"Mental", "were outsmarted by an inanimate object" );
			drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
				"You lost about half a day of time...", FG_RED );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void cacheScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;
	pushSkillBasedChoice( &sbChoices, "Attempt to break it open. (Craft)", SKL_CRAFT, 8,
		cacheSuccessScene, cacheCostlySuccessScene, cacheFailureScene );
	pushSimpleChoice( &sbChoices, "Ignore it.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"While walking you go off the trail for a bit of rest. Setting your pack down "
					"you're suprised to hear that whatever you put it on sounds hollow. Beneath some "
					"leaves you find a large metal box with a lock on it.\n\n"
					"What do you do?",
					FG_GREY );
			//pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

char* sirenPronoun;
char* sirenCapPronoun;
char* sirenGender;
char* sirenSelfPro;
char* sirenSelfCapPro;
char* sirenMeFailEnglish; // that's unpossible!

void sirenJoinSuccessScene( void )
{
	// gain an item
	static newGear;
	if( !fromStatusScreen ) {
		++timeLeft;
		newGear = rand( ) % NUM_GEAR;
		++character.gear[newGear];
	}
	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You leave your gear near the shore and dive in after Leucosia. %s smiles at you, "
					"and without warning splashes you. After wiping off your face you splash %s back. "
					"Both of you continue playing in the water.\n\n"
					"After a couple hours Leucosia thanks you for the wonderful time and tells you "
					"there's a farewell gift waiting for you on the shore.",
					FG_GREY,
					sirenCapPronoun, sirenMeFailEnglish, sirenPronoun );
			pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, newGear );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void sirenJoinCostlySuccessScene( void )
{
	// lose a day and gain an item
	static newGear;
	if( !fromStatusScreen ) {
		++timeLeft;
		newGear = rand( ) % NUM_GEAR;
		++character.gear[newGear];
	}
	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You leave your gear near the shore and dive in after Leucosia. %s smiles at you, "
					"and without warning splashes you. After wiping off your face you splash %s back. "
					"Both of you continue playing in the water.\n\n"
					"You wake up the next morning with an item laying on the ground with a note saying "
					"what a good time %s had.",
					FG_GREY,
					sirenCapPronoun, sirenPronoun, sirenPronoun );
			pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, newGear );
			drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
				"You've lost about half a day...", FG_RED );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void sirenJoinFailureScene( void )
{
	// gain two wounds
	if( !fromStatusScreen ) {
		gainPhysicalWound( "Drowned by a Fae" );
		gainSocialWound( );
	}
	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You leave your gear near the shore and dive in after Leucosia. %s swims farther and "
					"farther away from shore. Once you both are at a point where you can't feel the bottom "
					"of the lake %s turns to you and smiles. The smile keeps growing and growing, revealing "
					"rows of sharp teeth.\n\n"
					"You turn to swim away and you feel something trying pull you under. Immediately you "
					"fight back and swim towards shore. Leucosia is laughing the whole while but makes "
					"no attempt to follow you. You gather your gear and leave.",
					FG_GREY,
					sirenCapPronoun, sirenPronoun );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void sirenTowelScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;
	
	pushSimpleChoice( &sbChoices, "Join them.", sirenJoinSuccessScene );
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"%s lets out a loud and clear laugh, and says you can use it later after we've "
					"had some fun.\n\n"
					"What do you do?",
					FG_GREY, sirenCapPronoun );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void sirenApproachScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;
	if( character.gear[G_TOWEL] > 0 ) {
		pushSimpleChoice( &sbChoices, "Offer them your towel.", sirenTowelScene );
	}
	pushSkillBasedChoice( &sbChoices, "Join them. (Empathy)", SKL_EMPATHY, 8,
		sirenJoinSuccessScene, sirenJoinCostlySuccessScene, sirenJoinFailureScene );
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You reach a clearing in the woods where a clear lake sits. A short way off shore "
					"a beautiful %s with long, nearly white hair is bathing and singing. %s turns "
					"and look at you as you approach, waving at you. After a short while %s swims over "
					"and introduces %s as Leucosia.\n\n"
					"What do you do?",
					FG_GREY,
					sirenGender, sirenCapPronoun, sirenPronoun, sirenSelfPro );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void sirenStealScene( void )
{
	static newGear;
	if( !fromStatusScreen ) {
		newGear = rand( ) % NUM_GEAR;
		++character.gear[newGear];
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"While the %s is distracted you go through their belongings and find something "
					"interesting among them. You grab it and quickly leave.",
					FG_GREY,
					sirenGender );
			standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, newGear );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void sirenSneakSuccessScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Approach the singer.", sirenApproachScene );
	pushSimpleChoice( &sbChoices, "Steal something from them and leave.", sirenStealScene );
	pushSimpleChoice( &sbChoices, "Leave and continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You reach a clearing in the woods where a clear lake sits. A short way off shore "
					"a beautiful %s with long, nearly white hair is bathing and singing. Near your "
					"feet appears to be their belongings.\n\n"
					"What do you do?",
					FG_GREY,
					sirenGender );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void sirenSneakCostlySuccessScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Approach the singer.", sirenApproachScene );
	pushSimpleChoice( &sbChoices, "Leave and continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You reach a clearing in the woods where a clear lake sits. A short way off shore "
					"a beautiful %s with long, nearly white hair is bathing and singing.\n\n"
					"What do you do?",
					FG_GREY,
					sirenGender );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void sirenSneakFailureScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Approach the singer.", sirenApproachScene );
	pushSimpleChoice( &sbChoices, "Leave and continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"While sneaking up to where you here the singing you step on a branch. The singing "
					"stops and you here a voice call out asking who's there.\n\n"
					"What do you do?",
					FG_GREY );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void sirenScene( void )
{
	// sneak up or approach directly
	if( !fromStatusScreen ) {
		if( rand( ) % 2 ) {
			sirenPronoun = "she";
			sirenCapPronoun = "She";
			sirenGender = "woman";
			sirenSelfPro = "herself";
			sirenSelfCapPro = "Herself";
			sirenMeFailEnglish = "her";
		} else {
			sirenPronoun = "he";
			sirenCapPronoun = "He";
			sirenGender = "man";
			sirenSelfPro = "himself";
			sirenSelfCapPro = "Himself";
			sirenMeFailEnglish = "him";
		}
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Investigate the singing.", sirenApproachScene );
	pushSkillBasedChoice( &sbChoices, "Investigate the music cautiously. (Stealth)", SKL_STEALTH, 8,
		sirenSneakSuccessScene, sirenSneakCostlySuccessScene, sirenSneakFailureScene );
	pushSimpleChoice( &sbChoices, "Ignore it and continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"Walking through the woods you become aware of some singing in the distance. As "
					"you continue on you hear it getting louder and louder. It's beautiful.\n\n"
					"What do you do?",
					FG_GREY );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void cliffPathScene( void )
{
	if( !fromStatusScreen ) {
		timeLeft += 2;
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You go back through the woods and find the path. Continuing on it is much safer, "
					"but also slower.",
					FG_GREY );
			drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
				"You lose about a day of time...", FG_RED );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void cliffMapSuccessScene( void )
{
	// success
	startScene( );

	COORD pos;
	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You recognize the symbols on the map as some nearby ancient structures. "
					"Figuring that they would be useless you find an area where there looks "
					"to be much less of slope. After getting there you are able to safely get down.",
					FG_GREY );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void cliffMapCostlySuccessScene( void )
{
	// mental wound and find shortcut
	if( !fromStatusScreen ) {
		gainMentalWound( );
	}

	startScene( );

	COORD pos;
	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You wander around for a few hours, sure that there should be a path nearby. "
					"Eventually your realize that the spot you were looking for would be an ancient "
					"structure and probably of no use. You find a different spot that looks promising "
					"and are able to find a safe way down.",
					FG_GREY );
			standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Mental", "became frustrated and lost" );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void cliffMapFailureScene( void )
{
	// mental wound and follow path
	if( !fromStatusScreen ) {
		gainMentalWound( );
	}

	startScene( );

	COORD pos;
	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "You'll have to go back to the path.", cliffPathScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You wander around for a few hours, sure that there should be a path nearby. "
					"Eventually you find what looks like an old broken stairway that goes down the "
					"cliff. The only problem is the cliff is much taller here, and the broken stairs "
					"look like they would probably hurt you more than they would help.",
					FG_GREY );
			standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Mental", "became frustrated and lost" );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void cliffClimbRopeSuccessScene( void )
{
	// success
	startScene( );

	COORD pos;
	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You tie the rope around a tree and fix it with a knot so you should be able to "
					"get it back when you get down the cliff. You get down the cliff safely, grab the "
					"other end of the rope and pull, releasing it from the tree.",
					FG_GREY );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void cliffClimbRopeCostlySuccessScene( void )
{
	// lose rope
	if( !fromStatusScreen ) {
		--character.gear[G_ROPE];
	}

	startScene( );

	COORD pos;
	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You tie the rope around a tree and fix it with a knot so you should be able to "
					"get it back when you get down the cliff. Near the bottom of the cliff the rope starts "
					"fraying and then snaps. You're low enough though that you're able to safely jump "
					"to the ground.",
					FG_GREY );
			standardGearLossText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, G_ROPE );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void cliffClimbRopeFailureScene( void )
{
	// lose rope and physical wound
	if( !fromStatusScreen ) {
		--character.gear[G_ROPE];
		gainPhysicalWound( "Fell Off a Cliff" );
	}

	startScene( );

	COORD pos;
	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You tie the rope around a tree and fix it with a knot so you should be able to "
					"get it back when you get down the cliff. Half-way down the cliff the rope starts "
					"fraying and then snaps. You slide down the cliff face. After checking to make "
					"sure nothing is broken you stand up.",
					FG_GREY );
			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "fell off a cliff" );
			standardGearLossText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, G_ROPE );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void cliffClimbSuccessScene( void )
{
	// success!
	startScene( );

	COORD pos;
	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You start climbing down the cliff. There are some tricky spots but you are able "
					"to avoid slipping or losing your grip.",
					FG_GREY );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void cliffClimbCostlySuccessScene( void )
{
	// wound
	if( !fromStatusScreen ) {
		gainPhysicalWound( "Fell Off a Cliff" );
	}

	startScene( );

	COORD pos;
	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You start climbing down the cliff. Half way down your distracted by the howling "
					"of a wolf in the distance and lose your grip. You slide down the side of the cliff. "
					"After making sure nothing is broken you look around to make sure there's no wolves.",
					FG_GREY );

			standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "fell off a cliff" );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void cliffClimbFailureScene( void )
{
	// two wounds, physical and mental
	if( !fromStatusScreen ) {
		gainMentalWound( );
		gainPhysicalWound( "Fell Off a Cliff" );
	}

	startScene( );

	COORD pos;
	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You start climbing down the cliff. Half way down your distracted by the howling "
					"of a wolf in the distance and lose your grip. You slide down the side of the cliff. "
					"After making sure nothing is broken you curse your lack of focus.",
					FG_GREY );
			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "fell off a cliff" );
			standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Mental", "were discouraged" );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void cliffScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;
	
	pushSkillBasedChoice( &sbChoices, "Climb down. (Acrobatics)", SKL_ACROBATICS, 10,
		cliffClimbSuccessScene, cliffClimbCostlySuccessScene, cliffClimbFailureScene);
	if( character.gear[G_ROPE] > 0 ) {
		pushSkillBasedChoice( &sbChoices, "Use your rope to climb down. (Acrobatics)", SKL_BARTER, 8,
			cliffClimbRopeSuccessScene, cliffClimbRopeCostlySuccessScene, cliffClimbRopeFailureScene );
	}
	if( character.gear[G_MAP] > 0 ) {
		pushSkillBasedChoice( &sbChoices, "Use your map to find a path. (Survival)", SKL_BARTER, 8,
			cliffMapSuccessScene, cliffMapCostlySuccessScene, cliffMapFailureScene );
	}
	pushSimpleChoice( &sbChoices, "Go back to the path.", cliffPathScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"Following the path you see it turning, you decide to try and take a short cut. "
					"A few hours later you find yourself on the edge of a cliff.\n\n"
					"What do you do?",
					FG_GREY );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void banditsLoseEquipmentScene( void )
{
	static Gear gOne, gTwo;
	if( storedScene != banditsLoseEquipmentScene ) {
		gOne = -1;
		gTwo = -1;

		// choose two random pieces of equipment to lose
		if( totalGearCount( ) > 0 ) {
			gOne = randomGearToLose( );
			--character.gear[gOne];
		} else {
			gainPhysicalWound( "Killed by Bandits" );
		}

		if( totalGearCount( ) > 0 ) {
			gTwo = randomGearToLose( );
			--character.gear[gTwo];
		} else {
			gainPhysicalWound( "Killed by Bandits" );
		}
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			if( ( gOne >= 0 ) && ( gTwo >= 0 ) ) {
				pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
						"You raise your hands in the air to show them you've surrendered. Two of the bandits "
						"go through your pack and take some things. They thank you and laugh before disappearing "
						"into the woods.",
						FG_GREY );
				pos = standardGearLossText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, gOne );
				standardGearLossText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, gTwo );
			} else if( gOne >= 0 ) {
				pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
						"You raise your hands in the air to show them you've surrendered. Two of the bandits "
						"go through your pack and take some things. They take your last remaining %s and when "
						"they find nothing else the tall one hits you in the knee with a club. They thank you "
						"and laugh before disappearing into the woods.",
						FG_GREY,
						gearData[gOne].name );
				pos = standardGearLossText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, gOne );
				standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "had your knee smashed by a bandit" );
			} else {
				pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
						"You raise your hands in the air to show them you've surrendered. Two of the bandits "
						"go through your pack. When they find it empty of anything valuable the knock you down "
						"and start repeatedly kicking. After a short while they stop, one of them spits on you "
						"and they disappear into the woods.",
						FG_GREY );
				pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were beaten by a group of bandits" );
				standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were beaten by a group of bandits" );
			}
			
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void banditsDriveOffSuccess( char* description )
{
	// drive off
	startScene( );

	COORD pos;
	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					description,
					FG_GREY );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void banditsDriveOffCostlySuccess( const char* description, const char* wound )
{
	// wound and driven off
	if( !fromStatusScreen ) {
		gainPhysicalWound( "Killed By Bandits" );
	}

	startScene( );

	COORD pos;
	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					description,
					FG_GREY );
			standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", wound );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void banditsDriveOffFailure( const char* description, const char* wound )
{
	// wound and mugging
	if( !fromStatusScreen ) {
		gainPhysicalWound( "Killed By Bandits" );
	}

	startScene( );

	COORD pos;
	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "This sucks.", banditsLoseEquipmentScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					description,
					FG_GREY );
			standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", wound );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void banditsDriveOffSuccessRangedScene( void )
{
	banditsDriveOffSuccess( 
		"You grab your weapon and dive behind cover. After few well placed shots the "
		"bandits are limping away. You take a few more shots to make sure they keep "
		"moving." );
}

void banditsDriveOffCostlySuccessRangedSuccess( void )
{
	banditsDriveOffCostlySuccess( 
		"You grab your weapon and start taking shots. After few well placed shots the "
		"bandits are limping away. You look down to see you're bleeding from a gun shot "
		"wound. After some inspection you can see that is was just graze, nothing too "
		"serious. You bandage it up.", "were shot" );
}

void banditsDriveOffFailureRangedScene( void )
{
	banditsDriveOffFailure(
		"You grab your weapon and start taking shots. After you take a good shot to the "
		"leg you jump behind some cover. The bandits take this chance to surround you "
		"and demand payment.", "were shot" );
}

void banditsDriveOffSuccessMeleeScene( void )
{
	if( bestFightWeapon( ) > -1 ) {
		banditsDriveOffSuccess( 
			"You grab your weapon and charge at the two bandits with pistols. This catches "
			"them off guard and your able to get a good shot on the short guy. While he's "
			"recovering the others grab him and flee." );
	} else {
		banditsDriveOffSuccess( 
			"You charge at the two bandits with pistols and start swinging your fists. This catches "
			"them off guard and your able to get a good shot on the short guy. While he's "
			"recovering the others grab him and flee." );
	}
}

void banditsDriveOffCostlySuccessMeleeScene( void )
{
	if( bestFightWeapon( ) > -1 ) {
		banditsDriveOffCostlySuccess( 
			"You grab your weapon and charge at the two bandits with pistols. This catches "
			"them off guard and your able to get a good shot on the skinny woman. However, the short "
			"guy was able to hit you with a shot from his pistol. While you're distracted "
			"the grab their friend and flee.", "were shot" );
	} else {
		banditsDriveOffCostlySuccess( 
			"You charge at the two bandits with pistols and start swinging you fists. This catches "
			"them off guard and your able to get a good shot on the skinny woman. However, the short "
			"guy was able to hit you with a shot from his pistol. While you're distracted "
			"the grab their friend and flee.", "were shot" );
	}
}

void banditsDriveOffFailureMeleeScene( void )
{
	if( bestFightWeapon( ) > -1 ) {
		banditsDriveOffFailure( 
			"You grab your weapon and charge at the bandit with a club. He side steps your attack and "
			"smacks you on the head. They kick away your weapon and demand payment.", "were hit in the head" );
	} else {
		banditsDriveOffFailure( 
			"Charge at the bandit with a club and start swinging your fists. He side steps your attack and "
			"smacks you on the head. They kick away your weapon and demand payment.", "were hit in the head" );
	}
}

void banditsJoinSuccessScene( void )
{
	if( !fromStatusScreen ) {
		character.hasCompanion[CMP_BANDITS];
	}

	startScene( );

	COORD pos;
	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"They seem impressed when you flash some of the cash you have on you. They "
					"look at each other nod, and ask where to next.",
					FG_GREY );
			drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
				"You've gained %s as a companion!", FG_GREEN, companionsData[CMP_BANDITS].name );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void banditsJoinCostlySuccessScene( void )
{
	if( storedScene != banditsJoinCostlySuccessScene ) {
		--character.gear[G_MONEY];
		character.hasCompanion[CMP_BANDITS];
	}

	startScene( );

	COORD pos;
	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"They seem impressed but demand some payment up front. You hand them some "
					"money have you have on you. They smile and ask where to next.",
					FG_GREY );
			pos = standardGearLossText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, G_MONEY );
			drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
				"You've gained %s as a companion!", FG_GREEN, companionsData[CMP_BANDITS].name );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void banditsJoinFailureScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "That didn't go as planned.", banditsLoseEquipmentScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"They laugh at you. The short one points his pistol at you and scowls.",
					FG_GREY );
			
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void banditsRunSuccessScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );
	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"Before they have time to react you break off into a sprint down the trail "
					"at top speed. You quickly outpace them, after about 15 minutes you slow down "
					"and catch your breath. You hear nothing but the sounds of the wilderness. "
					"Seems like you lost them.",
					FG_GREY );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void banditsRunCostlySuccessScene( void )
{
	if( storedScene != banditsRunCostlySuccessScene ) {
		gainPhysicalWound( "Tripped While Fleeing" );
	}
	// wound
	startScene( );

	COORD pos;
	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"Before they have time to react you break off into a sprint down the trail "
					"at top speed. You are too focussed on the bandits behind you to notice the "
					"root you trip over. After getting up and rubbing the knot on your head a bit "
					"you notice that the only thing you hear is the wilderness. Seems like you lost "
					"them.",
					FG_GREY );
			standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "hit your head after a trip" );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void banditsRunFailureScene( void )
{
	// wound and then mugging
	if( storedScene != banditsRunCostlySuccessScene ) {
		gainPhysicalWound( "Tripped While Fleeing" );
	}
	startScene( );

	COORD pos;
	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Ouch.", banditsLoseEquipmentScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"Before they have time to react you break off into a sprint down the trail "
					"at top speed. You are too focussed on the bandits behind you to notice the "
					"root you trip over. Your twisted ankle makes it easy for the bandits to "
					"catch up to you.",
					FG_GREY );
			standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "hit your head after a trip" );
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void banditsScene( void )
{
	// three bandits
	//  outcomes: you drive them off (fight or shoot)
	//            they take a random piece of equipment
	//            give them some money and convince them to join you
	//            give them some money intimidate them into joining you
	//	          run away

	startScene( );

	Choice* sbChoices = NULL;
	
	if( bestShootWeapon( ) > 0 ) {
		pushSkillBasedChoice( &sbChoices, "Fight them off from range. (Shoot)", SKL_SHOOT, 12 - bestShootWeaponBonus( ),
			banditsDriveOffSuccessRangedScene, banditsDriveOffCostlySuccessRangedSuccess, banditsDriveOffFailureRangedScene );
	}
	pushSkillBasedChoice( &sbChoices, "Charge at them. (Fight)", SKL_FIGHT, 12 - bestFightWeaponBonus( ),
		banditsDriveOffSuccessMeleeScene, banditsDriveOffCostlySuccessMeleeScene, banditsDriveOffFailureMeleeScene );
	if( character.gear[G_MONEY] > 0 ) {
		pushSkillBasedChoice( &sbChoices, "Bribe them to join you. (Barter)", SKL_BARTER, 10,
			banditsJoinSuccessScene, banditsJoinCostlySuccessScene, banditsJoinFailureScene );

		pushSkillBasedChoice( &sbChoices, "Show them you're a better leader. (Intimidate)", SKL_BARTER, 8,
			banditsJoinSuccessScene, banditsJoinCostlySuccessScene, banditsJoinFailureScene );
	}
	pushSimpleChoice( &sbChoices, "Give them some of your gear.", banditsLoseEquipmentScene );
	pushSkillBasedChoice( &sbChoices, "Run away. (Acrobatics)", SKL_ACROBATICS, 10,
		banditsRunSuccessScene, banditsRunCostlySuccessScene, banditsRunFailureScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"While walking through the woods you bend down to pick up a coin that was left lying on the "
					"trail. When you look back up you see that you're surrounded by three people in shaggy cloths. "
					"There is a tall, young man with a club, a skinny woman with a pistol, and a short guy with "
					"another pistol. They demand a toll for using their road.\n\n"
					"What do you do?",
					FG_GREY );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void huntSuccessScene( void )
{
	static bool hunt;
	static bool which;
	if( storedScene != huntSuccessScene ) {
		++character.gear[G_RATION];
		hunt = ( ( rand( ) % 2 ) == 0 );
		which = ( ( rand( ) % 2 ) == 0 );
	}

	startScene( );
	
	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	do {
		startPlayDraw( ); {
			if( hunt ) {
				pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
						"After a short search you're able to trap a %s. You skin and cook it up before "
						"returning to the path you were following.",
						FG_GREY,
						which ? "squirrel" : "rabbit");
			} else {
				pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
						"After a short search you're able to find some edible %s. You stick some in your "
						"pack before returning to the path you were following.",
						FG_GREY,
						which ? "berries" : "roots");
			}
			standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, G_RATION );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );
	currentScene = nextScene;
}

void huntCostlySuccessScene( void )
{
	static bool hunt;
	static bool which;
	if( storedScene != huntSuccessScene ) {
		++character.gear[G_RATION];
		hunt = ( ( rand( ) % 2 ) == 0 );
		which = ( ( rand( ) % 2 ) == 0 );
		++timeLeft;
	}

	startScene( );
	
	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	do {
		startPlayDraw( ); {
			if( hunt ) {
				pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
						"After a short search you're able to trap a %s. You skin and cook it up before "
						"attempting to find the path you were following. It takes you a while and you figure "
						"you've lost about half a day.",
						FG_GREY,
						which ? "squirrel" : "rabbit");
			} else {
				pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
						"After a short search you're able to find some edible %s. You stick some in your "
						"pack before attempting to find the path you were following. It takes you a while "
						"and you figure you've lost about half a day.",
						FG_GREY,
						which ? "berries" : "roots");
			}
			pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, G_RATION );
			pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
				"You've lost some time.", FG_RED );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );
	currentScene = nextScene;
}

void huntFailureScene( void )
{
	static bool hunt;
	static bool which;
	if( storedScene != huntSuccessScene ) {
		hunt = ( ( rand( ) % 2 ) == 0 );
		which = ( ( rand( ) % 2 ) == 0 );
		++timeLeft;
	}

	startScene( );
	
	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	do {
		startPlayDraw( ); {
			if( hunt ) {
				pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
						"You spend about half the day hunting down a %s you saw. It evades you every time "
						"you get close to it. You eventually give up and find the path you had been following "
						"before.",
						FG_GREY,
						which ? "squirrel" : "rabbit");
			} else {
				pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
						"You find some %s that look delicious. After trying a little bit of them to make sure "
						"they're safe you spend about half the day vomiting. Once you recover you go find "
						"the path you had been following before.",
						FG_GREY,
						which ? "berries" : "roots");
			}
			pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
				"You've lost some time.", FG_RED );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );
	currentScene = nextScene;
}

void wanderScene( void )
{
	startScene( );

	static boolean ateFood;
	if( !fromStatusScreen ) {
		if( character.gear[G_RATION] > 0 ) {
			--character.gear[G_RATION];
			ateFood = true;
		} else {
			gainPhysicalWound( "Starved to Death" );
			ateFood = false;
		}
	}

	COORD pos;

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );
	pushSkillBasedChoice( &sbChoices, "Forage and hunt for food while traveling. (Survival)", SKL_SURVIVAL, 10,
		huntSuccessScene, huntCostlySuccessScene, huntFailureScene );
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			if( ateFood ) {
				pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You spend the day traveling through the wilderness, snacking on a ration as you go.\n\n"
					"What do you want to do?",
					FG_GREY );
				standardGearLossText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, G_RATION );
			} else {
				pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You spend the day traveling through the wilderness, your stomach complaining all the way.\n\n"
					"What do you want to do?",
					FG_GREY );
				standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "go hungry" );
			}

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

// Town scenes
void leaveTownLateScene( void )
{
	if( storedScene != leaveTownLateScene ) {
		setupWilderness( );
	}

	// they make no progress towards the goal, but hopefully they got something from their encounter
	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Go into the wilderness.", gotoNextWildernessScene );
	COORD pos;

	do {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You exit through the main gate while the sun starts to set. You set up a small camp site "
				"a short way from the walls. The following day you head out on your journey.",
				FG_GREY );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );

	currentScene = nextScene;
}

void leaveTownScene( void )
{
	if( storedScene != leaveTownScene ) {
		setupWilderness( );
		timeLeft -= 1;
	}

	// they make a days worth of progress towards the goal
	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Go into the wilderness.", gotoNextWildernessScene );
	COORD pos;

	do {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You leave through the main gate. There's still plenty of daylight left today.",
				FG_GREY );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );

	currentScene = nextScene;
}

void librarySuccessScene( void )
{
	if( storedScene != librarySuccessScene ) {
		++character.gear[G_MAP];
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Start your journey.", leaveTownScene );
	COORD pos;

	do {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You head towards the stack of maps. Immediately you see what sort of order they're "
				"being stored in and go to the part that will have local maps. You find exactly what "
				"you need and with plenty of daylight to spare.",
				FG_GREY,
				companionsData[CMP_SCHOLAR].name, companionsData[CMP_SCHOLAR].name );
			standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, G_MAP );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );

	currentScene = nextScene;
}

void libraryCostlySuccessScene( void )
{
	if( storedScene != libraryCostlySuccessScene ) {
		++character.gear[G_MAP];
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Start your journey.", leaveTownLateScene );
	COORD pos;

	do {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You spend the rest of the day searching a large stack of papers. Near the bottom you "
				"find a map of the local area. This may prove useful.",
				FG_GREY,
				companionsData[CMP_SCHOLAR].name, companionsData[CMP_SCHOLAR].name );
			standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, G_MAP );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );

	currentScene = nextScene;
}

void libraryFailureScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Start your journey.", leaveTownLateScene );
	COORD pos;

	do {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You spend the rest of the day searching through a large stack of papers and find nothing of use.",
				FG_GREY,
				companionsData[CMP_SCHOLAR].name, companionsData[CMP_SCHOLAR].name );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );

	currentScene = nextScene;
}

void libraryScene( void )
{
	// have a chance to find a map
	startScene( );

	Choice* sbChoices = NULL;
	pushSkillBasedChoice( &sbChoices, "Search for anything of interest. (Investigate)", SKL_INVESTIGATE, 8,
		librarySuccessScene, libraryCostlySuccessScene, libraryFailureScene );
	pushSimpleChoice( &sbChoices, "Start your journey.", leaveTownLateScene );
	COORD pos;

	do {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You head across town to the library, hoping to find something useful. You spend a "
				"good portion of the day searching through various books about legends. Finding nothing "
				"of use you go and talk to the librarian. He points you toward a section that contains "
				"a large stack of useful maps.\n\n"
				"What do you do?",
				FG_GREY );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );
	currentScene = nextScene;
}

void mentorScene( void )
{
	Gear newGear;

	// will give you a random item
	if( storedScene != mentorScene ) {
		// gain an item
		switch( rand( ) % 3 ) {
		case 0:
			newGear = G_HACKING_DEVICE;
			break;
		case 1:
			newGear = G_RIFLE;
			break;
		default:
			newGear = G_SWORD;
			break;
		}
		++character.gear[newGear];
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Start your journey.", leaveTownLateScene );
	COORD pos;

	do {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"After a short walk you come to your mentor's house. The old man lets in and brings "
				"you to his living room before heading off to the kitche. He returns shortly with some "
				"sandwiches and tea. The both of you spend a good portion of the day talking.\n\n"
				"It starts getting late, but before you leave your mentor says to wait, and leaves. "
				"He comes back with a %s which he gives to you, saying you'll probably have more "
				"use it now than he will.\n\n"
				"You head out to the edge of town while the sun starts setting.",
				FG_GREY,
				gearData[newGear].name );
			standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, newGear );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );

	checkTooMuchGear( nextScene );
}

void scholarSuccessScene( void )
{
	if( storedScene != scholarSuccessScene ) {
		character.hasCompanion[CMP_SCHOLAR] = true;
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Start your journey.", leaveTownLateScene );
	COORD pos;

	do {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You go up and start talking to %s. After a few drinks she spills to you that "
				"she's worried about the trial. She was always more interested in books and "
				"doesn't think she's ready for the traveling through the wilderness.\n\n"
				"While she's talking an idea occurs to you. The rules say nothing about having "
				"to travel along. You mention this to her and she brightens up. %s and you "
				"leave the tavern and head out of town.",
				FG_GREY,
				companionsData[CMP_SCHOLAR].name, companionsData[CMP_SCHOLAR].name );
			drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
				"You've gained %s as a companion!", FG_GREEN, companionsData[CMP_SCHOLAR].name );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );

	currentScene = nextScene;
}

void scholarCostlySuccessScene( void )
{
	if( storedScene != scholarCostlySuccessScene ) {
		gainMentalWound( );
		character.hasCompanion[CMP_SCHOLAR] = true;
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Start your journey.", leaveTownLateScene );
	COORD pos;

	do {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You go up and start talking to %s. After you both are pretty drunk she "
				"confides in you that she's nervous about the trial. She doesn't think "
				"she'll be able to handle going through the wilderness by herself.\n\n"
				"In a moment of drunken enlightenment you suggest that you two should "
				"travel together. She seems hesitant, wondering aloud if that's against "
				"the rules. You yell out screw the rules and you both stumble out of the "
				"tavern to the edge of town.",
				FG_GREY,
				companionsData[CMP_SCHOLAR].name );
			pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
				"You've gained %s as a companion!", FG_GREEN, companionsData[CMP_SCHOLAR].name );
			drawString( descriptionSafeArea.Left, pos.Y + 1, descriptionSafeArea,
				"You're hungover and gained a Mental wound...", FG_RED );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );

	currentScene = nextScene;
}

void scholarFailureScene( void )
{
	if( storedScene != scholarFailureScene ) {
		gainMentalWound( );
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Start your journey.", leaveTownLateScene );
	COORD pos;

	do {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You go up and start talking to %s. Attempting to get her to drink a bit more "
				"to open up you overextend yourself. The conversation ends with you vomiting "
				"all over her robes.\n\n"
				"You stumble out of town alone.",
				FG_GREY,
				companionsData[CMP_SCHOLAR].name );
			drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
				"You're hungover and gained a Mental wound...", FG_RED );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );

	currentScene = nextScene;
}

void warriorSuccessScene( void )
{
	if( storedScene != warriorSuccessScene ) {
		character.hasCompanion[CMP_WARRIOR] = true;
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Start your journey.", leaveTownLateScene );
	COORD pos;

	do {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"%s seems to be engaged in some sort of game with one of the patrons. "
				"Each takes turn telling of some increasing outrageous deed they did and "
				"have to tell if the other is lying. The patron loses shortly after you "
				"start watching. %s turns to you and asks if you want to play. You take "
				"a swig from your mug and agree.\n\n"
				"After a few hours of boasting you're both laughing and having a good time. "
				"%s says you should travel together and you agree.\n\n"
				"You both head out to the edge of town.",
				FG_GREY,
				companionsData[CMP_WARRIOR].name, companionsData[CMP_WARRIOR].name, companionsData[CMP_WARRIOR].name );
			drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
				"You've gained %s as a companion!", FG_GREEN, companionsData[CMP_WARRIOR].name );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );

	currentScene = nextScene;
}

void warriorCostlySuccessScene( void )
{
	if( storedScene != warriorCostlySuccessScene ) {
		character.hasCompanion[CMP_WARRIOR] = true;
		gainMentalWound( );
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Start your journey.", leaveTownLateScene );
	COORD pos;

	do {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"%s seems to be engaged in some sort of game with one of the patrons. "
				"Each takes turn telling of some increasing outrageous deed they did and "
				"have to tell if the other is lying. The patron loses shortly after you "
				"start watching. %s turns to you and asks if you want to play. You finish "
				"off your mug and agree.\n\n"
				"After a few hours of boasting and far too much drink you're both laughing "
				"and having a good time. %s says you should travel together and you agree.\n\n"
				"You both stumble out to the edge of town.",
				FG_GREY,
				companionsData[CMP_WARRIOR].name, companionsData[CMP_WARRIOR].name, companionsData[CMP_WARRIOR].name );
			pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
				"You've gained %s as a companion!", FG_GREEN, companionsData[CMP_WARRIOR].name );
			drawString( descriptionSafeArea.Left, pos.Y + 1, descriptionSafeArea,
				"You're hungover and gained a Mental wound...", FG_RED );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );

	currentScene = nextScene;
}

void warriorFailureScene( void )
{
	if( storedScene != warriorFailureScene ) {
		gainMentalWound( );
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Start your journey.", leaveTownLateScene );
	COORD pos;

	do {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"%s seems to be engaged in some sort of game with one of the patrons. "
				"Each takes turn telling of some increasing outrageous deed they did and "
				"have to tell if the other is lying. The patron loses shortly after you "
				"start watching. %s turns to you and asks if you want to play. You finish "
				"off your mug and agree.\n\n"
				"You drink far too much and it becomes obvious that you're lying almost "
				"instantly. %s laughs it off and finds a new opponent.\n\n"
				"You stumble to the edge of town alone.",
				FG_GREY,
				companionsData[CMP_WARRIOR].name, companionsData[CMP_WARRIOR].name, companionsData[CMP_WARRIOR].name );
			drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
				"You're hungover and gained a Mental wound...", FG_RED );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );

	currentScene = nextScene;
}

void tavernScene( void )
{
	// have a chance to get a companion
	startScene( );

	Choice* sbChoices = NULL;
	pushSkillBasedChoice( &sbChoices, "Go talk to the scholar. (Empathy)", SKL_EMPATHY, 6,
		scholarSuccessScene, scholarCostlySuccessScene, scholarFailureScene );
	pushSkillBasedChoice( &sbChoices, "Go talk to the warrior. (Lie)", SKL_LIE, 6,
		warriorSuccessScene, warriorCostlySuccessScene, warriorFailureScene );
	pushSimpleChoice( &sbChoices, "Start your jouryney.", leaveTownLateScene );

	do {
		startPlayDraw( ); {
			drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You quickly head over to The Wet Mug. It's not the classiest place, but you're "
				"a regular there and hope to get some free food and drinks before you leave.\n\n"
				"Once inside a toast is raised in your honor. The barkeep gets you a fresh plate "
				"of lamb and bread. Patrons buy you drinks all day.\n\n"
				"While there you notice two people. %s, a young scholarly lady. She's starting "
				"her initiation today as well and had seemed extremely nervous about it. There's "
				"also %s. He's a young warrior also starting his initiation today.\n\n"
				"What do you do?",
				FG_GREY,
				companionsData[CMP_SCHOLAR].name, companionsData[CMP_WARRIOR].name );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );

	currentScene = nextScene;
}

void initialTownScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Leave right away.", leaveTownScene );
	pushSimpleChoice( &sbChoices, "Go talk to your mentor.", mentorScene );
	pushSimpleChoice( &sbChoices, "Spend some time in the tavern.", tavernScene );
	pushSimpleChoice( &sbChoices, "Check the library for useful information.", libraryScene );

	do {
		startPlayDraw( ); {
			drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You step out of your small home with a pack on your back. Filled with excitement for "
				"the journey ahead of you and anxious about the challenges you'll face.\n\n"
				"The rules for the initiation states you don't have to be out of town "
				"until it's reached nightfall.\n\nIs there anything else you wish to do before you leave?",
				FG_GREY );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );

	currentScene = nextScene;
}

// Character creation scenes
void itemSelectionScene( void )
{
	topTitle = " Choose Your Gear ";

	// based on their barter skill they can get more equipment to select
	// only have a number of equipment slots equal to their Physical die size
	int baseGear = 4;
	int bonusGear = character.ss_barter;
	int totalGear = baseGear + bonusGear;

	int selectedItem = 0;

	int totalAllowed = min( totalGear, carryWeight( ) );

	boolean showError = false;
	boolean done = false;

	WORD normalAttr = FG_DARK_CYAN | BG_BLACK;
	WORD highlightAttr = FG_CYAN | BG_BROWN;
	WORD attr;

	SMALL_RECT descriptionArea = { 30, 21, SCREEN_WIDTH - 2, SCREEN_HEIGHT - 2 };
	SMALL_RECT gearColumn = { 5, 17, 25, 30 };

	do {
		startDraw( ); {
			drawString( safeWriteArea.Left, safeWriteArea.Left, safeWriteArea,
				"You spend the week before your departure checking and double checking everything. "
				"You call in favors and trade what little you have for the equipment you'll need to survive your ordeal.\n\n"
				"For a base you'll get to choose %i pieces of gear, with a bonus of %i items due to your Bartering skill, "
				"giving you a total of %i pieces you can purchase.\n\n"
				"Due to your Physical attribute and Acrobatics you'll be able to carry %i pieces of gear total.\n\n"
				"Use the up and down arrow keys to select a piece of gear, press right to add one to your inventory and left "
				"to remove one.",
				FG_GREY, baseGear, bonusGear, totalGear, carryWeight( ) );

			drawString( 30, 17, safeWriteArea, "Gear Allowed Left: %i", FG_GREEN | BG_BLACK, ( totalAllowed - totalGearCount( ) ) );

			if( showError ) {
				drawString( 30, 19, safeWriteArea,
					"Not enough allowed gear left.", FG_RED | BG_BLACK );
			}

			// display all the gear
			SHORT y = gearColumn.Top;
			for( int i = 0; i < NUM_GEAR; ++i ) {
				if( gearData[i].availableAtStore ) {
					attr = ( selectedItem == i ) ? highlightAttr : normalAttr;
					centerStringHoriz( gearColumn, y, "%s: %i", attr, gearData[i].name, character.gear[i] );
					++y;
				}
			}

			drawString( descriptionArea.Left, descriptionArea.Top, descriptionArea, gearData[selectedItem].description, FG_CYAN | BG_BLACK );

			centerStringHoriz( safeWriteArea, safeWriteArea.Bottom,
					"Press Enter to start your adventure",
					FG_YELLOW | BG_BLACK );
		} endDraw( );

		// process input
		Input input;
		boolean handled = false;
		do {
			input = getNextInput( );
			switch( input ) {
			case IN_UP:
				showError = false;
				handled = true;
				do {
					--selectedItem;
					if( selectedItem < 0 ) {
						selectedItem = NUM_GEAR - 1;
					}
				} while( !gearData[selectedItem].availableAtStore );
				break;
			case IN_DOWN:
				showError = false;
				handled = true;
				do {
					selectedItem = ( selectedItem + 1 ) % NUM_GEAR;
				} while( !gearData[selectedItem].availableAtStore );
				break;
			case IN_LEFT:
				showError = false;
				handled = true;
				removeGear( selectedItem );
				break;
			case IN_RIGHT:
				handled = true;
				if( ( totalAllowed - totalGearCount( ) ) > 0 ) {
					showError = false;
					addGear( selectedItem );
				} else {
					showError = true;
				}
				break;
			case IN_ENTER:
				handled = true;
				done = true;
				break;
			}
		} while( !handled );

		eatAllInputs( );
	} while( !done );

	currentScene = initialTownScene;
}

void skillSelectionScene( void )
{
	topTitle = " Choose Your Skills ";
	boolean done = false;
	int selectedSkill = 0;

	WORD normalSkill = FG_DARK_CYAN | BG_BLACK;
	WORD highlightSkill = FG_CYAN | BG_BROWN;
	WORD attr;

	SMALL_RECT physicalColumn = { 5, 17, 25, 22 };
	SMALL_RECT mentalColumn = { 5, 24, 25, 29 };
	SMALL_RECT socialColumn = { 5, 31, 25, 36 };
	SMALL_RECT descriptionArea = { 28, 21, SCREEN_WIDTH - 2, SCREEN_HEIGHT - 2 };

	boolean showError = false;

	do {
		startDraw( ); {
			drawString( 2, 2, safeWriteArea,
				"Choose what skills you want to have learned. Use the up and down arrow keys to highlight a skill then use the right "
				"arrow key to increase it and the left arrow key to decrease it. "
				"Each level in a skill will cost 1 point up until the skill is at the same value as the skills "
				"matching attribute, after that it will cost 2 points.\n\n"
				"During your adventure you will be presented with a situation and a number of ways to resolve the situation. How successful "
				"you are will be based on a skill check, which is calculated by rolling the die of the attribute associated with the skill "
				"and adding the skill level to it (e.g. an Acrobatics check would roll your Physical die and adding your Acrobatics skill "
				"along with any bonuses from equipment and subtracting penalties from wounds).",
				FG_GREY | BG_BLACK );

			// draw the attributes
			centerStringHoriz( physicalColumn, physicalColumn.Top, "Physical: d%i", FG_GREEN | BG_BLACK, character.stat_physicalDie );
			centerStringHoriz( mentalColumn, mentalColumn.Top, "Mental: d%i", FG_GREEN | BG_BLACK, character.stat_mentalDie );

			centerStringHoriz( socialColumn, socialColumn.Top, "Social: d%i", FG_GREEN | BG_BLACK, character.stat_socialDie );

			// draw the skill sheet
			//  physical
			SHORT y = physicalColumn.Top + 1;
			int i = 0;
			for( ; i < 4; ++i ) {
				attr = ( selectedSkill == i ) ? highlightSkill : normalSkill;
				centerStringHoriz( physicalColumn, y, "%s: %i", attr, skillsData[i].name, (*(skillsData[i].value ) ) );
				++y;
			}

			//  mental
			y = mentalColumn.Top + 1;
			for( ; i < 8; ++i ) {
				attr = ( selectedSkill == i ) ? highlightSkill : normalSkill;
				centerStringHoriz( mentalColumn, y, "%s: %i", attr, skillsData[i].name, (*(skillsData[i].value ) ) );
				++y;
			}

			//  social
			y = socialColumn.Top + 1;
			for( ; i < 12; ++i ) {
				attr = ( selectedSkill == i ) ? highlightSkill : normalSkill;
				centerStringHoriz( socialColumn, y, "%s: %i", attr, skillsData[i].name, (*(skillsData[i].value ) ) );
				++y;
			}

			drawString( 28, 17, safeWriteArea, "Skill points left: %i", FG_GREEN | BG_BLACK, character.skillPointsLeft );

			if( showError ) {
				drawString( 28, 19, safeWriteArea,
					"Not enough points left.", FG_RED | BG_BLACK );
			}

			drawString( descriptionArea.Left, descriptionArea.Top, descriptionArea,
				skillsData[selectedSkill].description, FG_CYAN | BG_BLACK );

			if( character.skillPointsLeft <= 0 ) {
				centerStringHoriz( safeWriteArea, safeWriteArea.Bottom,
					"Press Enter to continue to equipment selection",
					FG_YELLOW | BG_BLACK );
			} else {
				centerStringHoriz( safeWriteArea, safeWriteArea.Bottom,
					"Press Enter to randomly distribute the points you have left",
					FG_GREEN | BG_BLACK );
			}

		} endDraw( );

		// handle input
		Input input;
		boolean handled = false;
		do {
			input = getNextInput( );
			switch( input ) {
			case IN_UP:
				handled = true;
				--selectedSkill;
				if( selectedSkill < 0 ) {
					selectedSkill = ARRAYSIZE( skillsData ) - 1;
				}
				showError = false;
				break;
			case IN_DOWN:
				handled = true;
				selectedSkill = ( selectedSkill + 1 ) % ARRAYSIZE( skillsData );
				showError = false;
				break;
			case IN_LEFT:
				handled = true;
				if( (*skillsData[selectedSkill].value) > 0 ) {
					--(*skillsData[selectedSkill].value);
					uint8_t refund = ( ( (*skillsData[selectedSkill].value) ) >= (*skillsData[selectedSkill].attrValue ) ) ? 2 : 1;
					character.skillPointsLeft += refund;
					showError = false;
				}
				break;
			case IN_RIGHT: {
					handled = true;
					uint8_t cost = ( (*skillsData[selectedSkill].value) >= (*skillsData[selectedSkill].attrValue ) ) ? 2 : 1;
					if( character.skillPointsLeft < cost ) {
						showError = true;
					} else {
						character.skillPointsLeft -= cost;
						++(*skillsData[selectedSkill].value);
						showError = false;
					}
				} break;
			case IN_ENTER:
				handled = true;
				if( character.skillPointsLeft <= 0 ) {
					done = true;
				} else {
					randomlyDistributeSkills( );
				}
				break;
			}
		} while( !handled );
		eatAllInputs( );
	} while( !done );

	currentScene = itemSelectionScene;
}

#define NUM_HISTORY_EVENTS 3
void historyScene( void )
{
	topTitle = " What Has Shaped You? ";

	// we'll have ten events, choose three of them, will affect stats and how
	//  many skill points you start with
	int indices[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	int choices[NUM_HISTORY_EVENTS];
	SHUFFLE( indices, int );

	for( int i = 0; i < NUM_HISTORY_EVENTS; ++i ) {
		choices[i] = -1;
	}

	int currDisplay = 0;
	initCharacter( );

	while( currDisplay < NUM_HISTORY_EVENTS ) {
		COORD currPos = { 2, 4 };
#define NEXTLINE { currPos.X = 2; ++currPos.Y; }
#define NEXTEVENT { currPos.X = 2; currPos.Y += 2; }
		startDraw( ); {
			currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
					"The following events are the major ones that happened to you during your stay with The Coalition:",
					FG_GREY | BG_BLACK );
			NEXTEVENT;

			for( int i = 0; i <= currDisplay; ++i ) {

				boolean currEvent = ( i == currDisplay );

				switch( indices[i] ) {

				case 0:
					currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
						"One of the teachers takes a special interest in you and gives you some extra instruction. "
						"You spend many evenings with them, learning all that they can teach you.",
						FG_GREY | BG_BLACK );
					NEXTLINE;
					currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
						"( You gain some extra skill points! )",
						FG_GREEN | BG_BLACK );

					if( currEvent ) {
						character.skillPointsLeft += 6;
					}
					break;

				case 1:
					currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
						"You spend lots of time helping one of the older members with physical tasks they are no longer able to do.",
						FG_GREY | BG_BLACK );
					NEXTLINE;
					currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
						"( Your Physical stat increases! )",
						FG_GREEN | BG_BLACK );
					NEXTLINE;
					if( currEvent ) {
						choices[i] = rand( ) % 2;
						switch( choices[i] ) {
						case 0:
							decreaseStat( &( character.stat_mentalDie ) );
							break;
						case 1:
							decreaseStat( &( character.stat_socialDie ) );
							break;
						}
						increaseStat( &( character.stat_physicalDie ) );
					}
					switch( choices[i] ) {
					case 0:
						currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
							"However you spend less time reading and learning.",
							FG_GREY | BG_BLACK );
						NEXTLINE;
						currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
							"( Your Mental stat decreases... )",
							FG_RED | BG_BLACK );
						break;
					case 1:
						currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
							"However you spend less time hanging out with the other children.",
							FG_GREY | BG_BLACK );
						NEXTLINE;
						currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
							"( Your Social stat decreases... )",
							FG_RED | BG_BLACK );
						break;
					}
					break;

				case 2:
					currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
						"You spend a large amount of time in the library, reading everything you can.",
						FG_GREY | BG_BLACK );
					NEXTLINE;
					currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
						"( Your Mental stat increases! )",
						FG_GREEN | BG_BLACK );
					NEXTLINE;
					if( currEvent ) {
						choices[i] = rand( ) % 2;
						switch( choices[i] ) {
						case 0:
							decreaseStat( &( character.stat_physicalDie ) );
							break;
						case 1:
							decreaseStat( &( character.stat_socialDie ) );
							break;
						}
						increaseStat( &( character.stat_mentalDie ) );
					}
					switch( choices[i] ) {
					case 0:
						currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
							"However you spend less time playing sports with the other children.",
							FG_GREY | BG_BLACK );
						NEXTLINE;
						currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
							"( Your Physical stat decreases... )",
							FG_RED | BG_BLACK );
						break;
					case 1:
						currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
							"However you spend less time hanging out with the other children.",
							FG_GREY | BG_BLACK );
						NEXTLINE;
						currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
							"( Your Social stat decreases... )",
							FG_RED | BG_BLACK );
						break;
					}
					break;

				case 3:
					currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
						"One of the older children takes a liking to you and shows you the ins and outs of the various factions of The Coalition.",
						FG_GREY | BG_BLACK );
					NEXTLINE;
					currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
						"( Your Social stat increases! )",
						FG_GREEN | BG_BLACK );
					NEXTLINE;
					if( currEvent ) {
						choices[i] = rand( ) % 2;
						switch( choices[i] ) {
						case 0:
							decreaseStat( &( character.stat_physicalDie ) );
							break;
						case 1:
							decreaseStat( &( character.stat_mentalDie ) );
							break;
						}
						increaseStat( &( character.stat_socialDie ) );
					}
					switch( choices[i] ) {
					case 0:
						currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
							"However you spend less time playing sports with the other children.",
							FG_GREY | BG_BLACK );
						NEXTLINE;
						currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
							"( Your Physical stat decreases... )",
							FG_RED | BG_BLACK );
						break;
					case 1:
						currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
							"However you spend less time reading and learning.",
							FG_GREY | BG_BLACK );
						NEXTLINE;
						currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
							"( Your Mental stat decreases... )",
							FG_RED | BG_BLACK );
						break;
					}
					break;

				case 4:
					currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
						"You are chosen as a subject for an experiment with an old formula found in some nearby ruins.",
						FG_GREY | BG_BLACK );
					NEXTLINE;
					if( currEvent ) {
						choices[i] = rand( ) % 3;
						switch( choices[i] ) {
						case 0:
							increaseStat( &( character.stat_physicalDie ) );
							break;
						case 1:
							increaseStat( &( character.stat_mentalDie ) );
							break;
						default:
							increaseStat( &( character.stat_socialDie ) );
							break;
						}
					}
					switch( choices[i] ) {
					case 0:
						currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
							"It causes your muscles to develop faster and to a larger size.",
							FG_GREY | BG_BLACK );
						NEXTLINE;
						currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
							"( Your Physical stat increases! )",
							FG_GREEN | BG_BLACK );
						break;
					case 1:
						currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
							"It causes your mind to become clearer and you find you can grasp difficult concepts easier.",
							FG_GREY | BG_BLACK );
						NEXTLINE;
						currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
							"( Your Mental stat increases! )",
							FG_GREEN | BG_BLACK );
						break;
					case 2:
						currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
							"It causes you to emit a pleasent scent, putting those around in a better mood.",
							FG_GREY | BG_BLACK );
						NEXTLINE;
						currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
							"( Your Social stat increases! )",
							FG_GREEN | BG_BLACK );
						break;
					}
					break;

				case 5:
					currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
						"This group's merchant tutors you. You spend a large amount of time looking through ledgers and haggling with strangers.",
						FG_GREY | BG_BLACK );
					NEXTLINE;
					currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
							"( Your Mental and Social stat increases! )",
							FG_GREEN | BG_BLACK );
					NEXTLINE;
					currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
						"Most of the physical work is done by other students.",
						FG_GREY | BG_BLACK );
					NEXTLINE;
					currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
							"( Your Physical stat decreases... )",
							FG_RED | BG_BLACK );
					if( currEvent ) {
						decreaseStat( &( character.stat_physicalDie ) );
						increaseStat( &( character.stat_mentalDie ) );
						increaseStat( &( character.stat_socialDie ) );
					}
					break;

				case 6:
					currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
						"You join the local football team and quickly become a star player and team leader.",
						FG_GREY | BG_BLACK );
					NEXTLINE;
					currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
							"( Your Physical and Social stat increases! )",
							FG_GREEN | BG_BLACK );
					NEXTLINE;
					currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
						"This leaves you little time for studying.",
						FG_GREY | BG_BLACK );
					NEXTLINE;
					currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
							"( Your Mental stat decreases... )",
							FG_RED | BG_BLACK );
					if( currEvent ) {
						increaseStat( &( character.stat_physicalDie ) );
						decreaseStat( &( character.stat_mentalDie ) );
						increaseStat( &( character.stat_socialDie ) );
					}
					break;

				case 7:
					currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
						"An old book detailing a hand-to-hand fighting technique is brought back from some old ruins."
						"You spend time studying and practicing what's in it. You find it's good exercise "
						"and increases your mental focus.",
						FG_GREY | BG_BLACK );
					NEXTLINE;
					currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
							"( Your Physical and Mental stat increases! )",
							FG_GREEN | BG_BLACK );
					NEXTLINE;
					currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
						"However you were the only one interested in it.",
						FG_GREY | BG_BLACK );
					NEXTLINE;
					currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
							"( Your Social stat decreases... )",
							FG_RED | BG_BLACK );
					if( currEvent ) {
						increaseStat( &( character.stat_physicalDie ) );
						increaseStat( &( character.stat_mentalDie ) );
						decreaseStat( &( character.stat_socialDie ) );
					}
					break;

				case 8:
					if( currEvent ) {
						choices[i] = rand( ) % 3;
						switch( choices[i] ) {
						case 0:
							decreaseStat( &( character.stat_physicalDie ) );
							break;
						case 1:
							decreaseStat( &( character.stat_mentalDie ) );
							break;
						default:
							decreaseStat( &( character.stat_socialDie ) );
							break;
						}
					}
					currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
						"There is a horrible accident and you're injured.",
						FG_GREY | BG_BLACK );
					NEXTLINE;
					switch( choices[i] ) {
					case 0:
						currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
							"You're crushed under a beam while helping construct a house. "
							"You recover but walk with a slight limp and tire more easily.",
							FG_GREY | BG_BLACK );
						NEXTLINE;
						currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
							"( Your Physical stat decreases... )",
							FG_RED | BG_BLACK );
						break;
					case 1:
						currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
							"A container containing an ancient biological weapon is shattered "
							"near the edge of town. You spend a few weeks with an extremely high fever. "
							"Afterwards your thoughts are cloudier than they had been before.",
							FG_GREY | BG_BLACK );
						NEXTLINE;
						currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
							"( Your Mental stat decreases... )",
							FG_RED | BG_BLACK );
						break;
					case 2:
						currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
							"While helping an elder member with making some chemical mixture "
							"you're handed the wrong vial to mix in. The resulting explosion "
							"burns you. You recover but the scars unnerve those around you.",
							FG_GREY | BG_BLACK );
						NEXTLINE;
						currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
							"( Your Social stat decreases... )",
							FG_RED | BG_BLACK );
						break;
					default:
						break;
					}
					break;

				case 9:
					if( currEvent ) {
						character.skillPointsLeft += 8;
					}
					currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
						"You find an old virtual reality training device. When you're not busy you "
						"spend time with it, learning all that you can.",
						FG_GREY | BG_BLACK );
					NEXTLINE;
					currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
						"( Your gain some extra skill points! )",
						FG_GREEN | BG_BLACK );
					break;
				}
				NEXTEVENT;

				if( currEvent ) {
					if( currDisplay == ( NUM_HISTORY_EVENTS - 1 ) ) {
						currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
							"Press any key to select your skills",
							FG_YELLOW | BG_BLACK );
					} else {
						currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
							"Press any key to see the next event",
							FG_WHITE | BG_BLACK );
					}
					NEXTEVENT;
				}


			}

			char buffer[1024];
			sprintf( buffer, "Physical: d%i   Mental: d%i    Social: d%i",
				character.stat_physicalDie, character.stat_mentalDie, character.stat_socialDie );
			drawStringIgnoreSize( 10, 45, safeWriteArea, buffer, FG_CYAN | BG_BLACK );

			sprintf( buffer, "Skill Points: %i", character.skillPointsLeft );
			drawString( 10, 46, safeWriteArea, buffer, FG_CYAN | BG_BLACK );
			
#undef NEXTLINE
#undef NEXTEVENT
		} endDraw( );

		waitForAnyInput( );
		++currDisplay;
	}

	currentScene = skillSelectionScene;
}

void introScene( void )
{
	topTitle = " Welcome! ";
	startDraw( ); {
		drawString( 2, 2, safeWriteArea,
			"The Golden Age of Technology ended a few centuries ago.\n\n"
			"Humanity barely survived, but have been slowly regaining knowledge and "
			"expanding.\n\n"
			"As a baby you were sent to stay with The Coalition, a group created by "
			"the new nations to rediscover what once was lost.\n\n"
			"It is your sixteenth year here and it is time to take your initiation. "
			"You will have to travel many days through the wilderness, go out into "
			"a ruined city of your ancestors, and bring back something valuable.\n\n"
			"Not everyone comes back.\n\n"
			"But before that, lets figure out who you are and what events shaped your "
			"life.",
			FG_GREY | BG_BLACK );

		centerStringHoriz( renderArea, renderArea.Bottom, "Press any key to start", FG_YELLOW | BG_BLACK );
	} endDraw( );

	waitForAnyInput( );
	currentScene = historyScene;
}

void titleScene( void )
{
	startDraw( ); {
		centerStringHoriz( renderArea, 11, "Welcome To", FG_GREY | BG_BLACK );
		centerStringHoriz( renderArea, 14, "The Ruins of", FG_GREY | BG_BLACK );
		char blockText[] = {
			201,205,205,205,205,205,187, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 32, 32, 32, 32, 32, 32, 32, 10,
			186, 32, 32, 32, 32, 32,186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 32, 32, 32, 32, 32, 32, 32, 32,205,206,205, 32, 32, 32, 32, 32, 32, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 32, 32, 32, 32, 32, 32, 32, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32,186, 32, 32, 32,186, 32, 32,201,205,205,205,187, 32, 32,186, 32, 32,201,205,205,205,187, 32, 32,186, 32, 32,201,205,205,205,187, 10,
			186, 32, 32, 32, 32, 32,186, 32, 32,186, 32, 32, 32,186, 32, 32,186, 32, 32, 32,186, 32, 32,186, 32, 32,186, 32, 32, 32,186, 32, 32,186, 32, 32,186, 32, 32, 32, 32, 10,
			200,205,205,205,205,205,188, 32, 32,200,205,205,205,188, 32, 32,200,205,205,205,185, 32, 32,186, 32, 32,200,205,205,205,185, 32, 32,186, 32, 32,186, 32, 32, 32, 32, 10,
			 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 10,
			 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,200,205,205,205,188, 0
		};
		centerStringHoriz( renderArea, 16, blockText, FG_CYAN | BG_BLACK );

		centerStringHoriz( renderArea, 28, "A Game Created For", FG_GREY | BG_BLACK );
		centerStringHoriz( renderArea, 29, "Ludum Dare 36", FG_BROWN | BG_BLACK );

		centerStringHoriz( renderArea, renderArea.Bottom, "Press any key to begin", FG_YELLOW | BG_BLACK );

	} endDraw( );

	waitForAnyInput( );

	currentScene = introScene;
}

void deathScene( void )
{
	startDraw( ); {
		centerStringHoriz( renderArea, 10, "You Have Died...", FG_RED | BG_BLACK );
		char graveOutLine[] =
		{
			201,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,187, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 0
		};
		centerStringHoriz( renderArea, 16, graveOutLine, FG_GREY | BG_BLACK );


		char graveInside[] =
		{
			177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177, 10,
			177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177, 10,
			177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177, 10,
			177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177, 10,
			177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177, 10,
			177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177, 10,
			177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177, 10,
			177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177, 10,
			177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177, 10,
			177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177, 10,
			177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177, 10,
			177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177, 10,
			177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177, 10,
			177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177, 10,
			177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177, 10,
			177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177, 10,
			177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177, 0
		};
		centerStringHoriz( renderArea, 17, graveInside, FG_GREY | BG_BLACK );

		char ground[] =
		{
			219,219,219,219,219,219,219,219,219,219,219,219,219,219,219,219,219,219,219,219,219,219,219,219,219,219,219,219,219,219,219,219,219, 0
		};
		centerStringHoriz( renderArea, 33, ground, FG_GREEN | BG_BROWN );

		drawStringIgnoreSize( 34, 32, renderArea, "\\/", FG_GREEN | BG_GREY );

		drawStringIgnoreSize( 42, 32, renderArea, "/", FG_GREEN | BG_GREY );

		SMALL_RECT deathTextArea = { 32, 20, 48, 30 };
		drawString( deathTextArea.Left, deathTextArea.Top, deathTextArea, deathReason, FG_WHITE | BG_GREY );

		centerStringHoriz( renderArea, renderArea.Bottom, "Press any key to continue", FG_YELLOW | BG_BLACK );
	} endDraw( );

	waitForAnyInput( );
	currentScene = titleScene;
}

void helpScene( void )
{
	topTitle = " Help ";
	nextScene = NULL;
	COORD outPos;
	fromStatusScreen = true;

	startDraw( ); {
		outPos = drawString( safeWriteArea.Left, safeWriteArea.Top, safeWriteArea, "Controls", FG_WHITE );
		outPos = drawString( safeWriteArea.Left + 1, outPos.Y + 1, safeWriteArea,
			"When presented with a choice press up and down to select the choice you want. Then press enter to make that choice.\n"
			"Pressing H will bring up this help screen, pressing C will bring up a more detailed character sheet.",
			FG_GREY );

		outPos = drawString( safeWriteArea.Left, outPos.Y + 2, safeWriteArea, "Choices", FG_WHITE );
		outPos = drawString( safeWriteArea.Left + 1, outPos.Y + 1, safeWriteArea,
			"When a choice has a skill name behind it in paranthesis that means that choice will require a skill check. If you "
			"roll too low you will not succeed at the task and you will take a penalty of some kind (e.g. gain a wound or lose an "
			"item). If you roll high enough you succeed. If you roll somewhere in the middle you'll have a costly success. You will "
			"still succeed at the task but take a penalty like you would with a failure.\n"
			"Some choices will also require an item, if you do not have the correct item you will not see that choice available.\n"
			"If you see an arrow at the bottom or top of the choice section of the screen that means there are more choices in that "
			"direction.",
			FG_GREY );

		outPos = drawString( safeWriteArea.Left, outPos.Y + 2, safeWriteArea, "Wounds", FG_WHITE );
		outPos = drawString( safeWriteArea.Left + 1, outPos.Y + 1, safeWriteArea,
			"Some failures will result in you gaining a Wound. These wounds are associated with a specific Statistic (Physical, "
			"Mental, or Social). They will give you a penalty on any roll involving that Statistic. When the number of Wounds "
			"you have for a Statistic matches the die size of that Statistic you are no longer to use any skills associated "
			"with that Statistic. The only exception is Physical. If you Physical wounds equals your Physical die size you "
			"lose the game.",
			FG_GREY );

		outPos = drawString( safeWriteArea.Left, outPos.Y + 2, safeWriteArea, "Gear", FG_WHITE );
		outPos = drawString( safeWriteArea.Left + 1, outPos.Y + 1, safeWriteArea,
			"Equipment you find and can use to help in certain Choices. There is a chance you will lose the equipment if you "
			"get a Failure or Costly Success.",
			FG_GREY );

		outPos = drawString( safeWriteArea.Left, outPos.Y + 2, safeWriteArea, "Companions", FG_WHITE );
		outPos = drawString( safeWriteArea.Left + 1, outPos.Y + 1, safeWriteArea,
			"Companions are listed with bonuses they give to your Skill checks. Some Choices will only be available if you "
			"have a Companion with you. They take care of themselves so you don't need to worry about their equipment.",
			FG_GREY );

		outPos = drawString( safeWriteArea.Left, outPos.Y + 2, safeWriteArea, "Goal", FG_WHITE );
		outPos = drawString( safeWriteArea.Left + 1, outPos.Y + 1, safeWriteArea,
			"The final goal is to make your way into the ruins of a collapsed civiliation and bring back something that "
			"would be deemd valuable by The Coalition. They are a group working to retrieve and rebuild what old technology "
			"they can.",
			FG_GREY );

		centerStringHoriz( renderArea, renderArea.Bottom, "Press any key to continue", FG_YELLOW | BG_BLACK );
	} endDraw( );

	waitForAnyInput( );
	eatAllInputs( );

	topTitle = NULL;
	currentScene = storedScene;
}

void characterDetailsScene( void )
{
	topTitle = " Character Details ";
	nextScene = NULL;
	COORD outPos;
	fromStatusScreen = true;

	SMALL_RECT physicalColumn = { 5, 5, 25, 11 };
	SMALL_RECT mentalColumn = { 30, 5, 50, 11 };
	SMALL_RECT socialColumn = { 55, 5, 75, 11 };
	SMALL_RECT companionArea = { 5, 14, SCREEN_WIDTH - 6, 19 };
	SMALL_RECT inventoryArea = { 5, 22, SCREEN_WIDTH - 6, 45 };
	
	startDraw( ); {

		// physical
		outPos = drawString( physicalColumn.Left, physicalColumn.Top, physicalColumn, "Physical: d%i", FG_GREEN, character.stat_physicalDie );
		int skillIdx = 0;
		for( ; skillIdx < 4; ++skillIdx ) {
			outPos = drawString( physicalColumn.Left + 1, outPos.Y + 1, physicalColumn, "%s: %i", FG_CYAN,
				skillsData[skillIdx].name, (*skillsData[skillIdx].value) );
		}
		outPos = drawString( physicalColumn.Left + 1, outPos.Y + 2, physicalColumn, "Wounds: %i", FG_MAROON, character.wounds_physical );

		// mental
		outPos = drawString( mentalColumn.Left, mentalColumn.Top, mentalColumn, "Mental: d%i", FG_GREEN, character.stat_mentalDie );
		for( ; skillIdx < 8; ++skillIdx ) {
			outPos = drawString( mentalColumn.Left + 1, outPos.Y + 1, mentalColumn, "%s: %i", FG_CYAN,
				skillsData[skillIdx].name, (*skillsData[skillIdx].value) );
		}
		outPos = drawString( mentalColumn.Left + 1, outPos.Y + 2, mentalColumn, "Wounds: %i", FG_MAROON, character.wounds_mental );

		// social
		outPos = drawString( socialColumn.Left, socialColumn.Top, socialColumn, "Social: d%i", FG_GREEN, character.stat_socialDie );
		for( ; skillIdx < 12; ++skillIdx ) {
			outPos = drawString( socialColumn.Left + 1, outPos.Y + 1, socialColumn, "%s: %i", FG_CYAN,
				skillsData[skillIdx].name, (*skillsData[skillIdx].value) );
		}
		outPos = drawString( socialColumn.Left + 1, outPos.Y + 2, socialColumn, "Wounds: %i", FG_MAROON, character.wounds_social );

		// companions
		outPos = drawString( companionArea.Left, companionArea.Top, companionArea, "Companions", FG_GREEN );
		for( int i = 0; i < NUM_COMPANIONS; ++i ) {
			if( character.hasCompanion[i] ) {
				outPos = drawString( companionArea.Left, outPos.Y + 1, companionArea, "%s", FG_CYAN,
					companionsData[i].name );

				outPos = drawString( companionArea.Right - 45, outPos.Y, companionArea, "(P: %i, M: %i, S: %i)", FG_DARK_CYAN,
					companionsData[i].physical, companionsData[i].mental, companionsData[i].social );
			}
		}

		// equipment
		outPos = drawString( inventoryArea.Left, inventoryArea.Top, inventoryArea, "Gear", FG_GREEN );
		SHORT x = inventoryArea.Left + 1;
		for( int i = 0; i < NUM_GEAR; ++i ) {
			if( character.gear[i] > 0 ) {
				outPos = drawString( x, outPos.Y + 1, inventoryArea, "%s x %i", FG_CYAN,
					gearData[i].name, character.gear[i] );

				if( outPos.Y > inventoryArea.Bottom ) {
					x += ( inventoryArea.Right - ( inventoryArea.Left + 1 ) ) / 3;
					outPos.Y = inventoryArea.Top;
				}
			}
		}
		
		centerStringHoriz( renderArea, renderArea.Bottom, "Press any key to continue", FG_YELLOW | BG_BLACK );
	} endDraw( );

	waitForAnyInput( );
	eatAllInputs( );

	currentScene = storedScene;
	topTitle = NULL;
}

void dropGearScene( void )
{
	topTitle = " Over-Encumbered ";
	COORD outPos;
	int selectedItem = 0;
	// make sure the selected item is valid
	while( character.gear[selectedItem] == 0 ) {
		++selectedItem;
	}

	WORD normalAttr = FG_DARK_CYAN | BG_BLACK;
	WORD highlightAttr = FG_CYAN | BG_BROWN;
	WORD attr;

	SMALL_RECT gearColumn = { 5, 7, 25, 45 };

	do {
		startDraw( ); {
			outPos = drawString( safeWriteArea.Left, safeWriteArea.Top, safeWriteArea,
				"You're carrying too much stuff, use the up and down arrow keys to choose an item, and press enter to drop it.", FG_GREY );
			drawString( safeWriteArea.Left, outPos.Y + 2, safeWriteArea,
				"You have to drop %i more %s.", FG_RED, -gearSpaceLeft( ), ( gearSpaceLeft( ) == -1 ) ? "item" : "items" );

			outPos.Y = gearColumn.Top - 1;
			for( int i = 0; i < NUM_GEAR; ++i ) {
				if( character.gear[i] != 0 ) {
					attr = ( i == selectedItem ) ? highlightAttr : normalAttr;
					outPos = drawString( gearColumn.Left, outPos.Y + 1, gearColumn, "%s x %i", attr,
						gearData[i].name, character.gear[i] );
				}
			}

		} endDraw( );

		// process input
		Input input;
		boolean handled = false;
		do {
			input = getNextInput( );
			switch( input ) {
			case IN_UP:
				handled = true;
				do {
					--selectedItem;
					if( selectedItem < 0 ) {
						selectedItem = NUM_GEAR - 1;
					}
				} while( character.gear[selectedItem] == 0 );
				break;
			case IN_DOWN:
				handled = true;
				do {
					selectedItem = ( selectedItem + 1 ) % NUM_GEAR;
				} while( character.gear[selectedItem] == 0 );
				break;
			case IN_ENTER:
				--character.gear[selectedItem];
				if( character.gear[selectedItem] == 0 ) {
					do {
						selectedItem = ( selectedItem + 1 ) % NUM_GEAR;
					} while( character.gear[selectedItem] == 0 );
				}
				handled = true;
				break;
			}
		} while( !handled );

		eatAllInputs( );
	} while( gearSpaceLeft( ) < 0 );

	currentScene = nextScene;
}

/******* END SCENES *********/

int main( int argc, char** argv )
{
	screen.write = GetStdHandle( STD_OUTPUT_HANDLE );
	screen.read = GetStdHandle( STD_INPUT_HANDLE );

	SetConsoleWindowInfo( screen.write, TRUE, &windowSize );
	SetConsoleScreenBufferSize( screen.write, bufferSize );

	SetConsoleTitle( TEXT( "The Ruins of Cuglatr" ) );

	srand( (unsigned int)time( NULL ) );
	topTitle = NULL;

	testDevCharacter( );

	setupSkillsData( );
	setupGearData( );
	setupCompanionData( );

	deathReason = "UNKNOWN DEATH REASON";

	currentScene = titleScene;

	while( 1 ) {
		currentScene( );
	}

	return 0;
}