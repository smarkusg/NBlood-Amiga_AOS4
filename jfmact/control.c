/*
 * control.c
 * MACT library controller handling
 *
 * Derived from MACT386.LIB disassembly by Jonathon Fowler
 *
 */

#include "build.h"
#include "baselayer.h"
#include "pragmas.h"

#include "types.h"
#include "keyboard.h"
#include "mouse.h"
#include "control.h"
#include "_control.h"
#include "util_lib.h"


boolean CONTROL_JoyPresent = false;
boolean CONTROL_JoystickEnabled = false;
boolean CONTROL_MousePresent = false;
boolean CONTROL_MouseEnabled = false;
uint32  CONTROL_ButtonState1 = 0;
uint32  CONTROL_ButtonHeldState1 = 0;
uint32  CONTROL_ButtonState2 = 0;
uint32  CONTROL_ButtonHeldState2 = 0;

static int32 CONTROL_UserInputDelay = -1;
static int32 CONTROL_MouseSensitivity = DEFAULTMOUSESENSITIVITY;
static int32 CONTROL_NumMouseButtons = 0;
static int32 CONTROL_NumMouseAxes = 0;
static int32 CONTROL_NumJoyButtons = 0;
static int32 CONTROL_NumJoyAxes = 0;
static controlflags       CONTROL_Flags[CONTROL_NUM_FLAGS];
static controlbuttontype  CONTROL_MouseButtonMapping[MAXMOUSEBUTTONS];
static controlbuttontype  CONTROL_JoyButtonMapping[MAXJOYBUTTONS];
static controlkeymaptype  CONTROL_KeyMapping[CONTROL_NUM_FLAGS];
static controlaxismaptype CONTROL_MouseAxesMap[MAXMOUSEAXES];	// maps physical axes onto virtual ones
static controlaxismaptype CONTROL_JoyAxesMap[MAXJOYAXES];
static controlaxistype    CONTROL_MouseAxes[MAXMOUSEAXES];	// physical axes
static controlaxistype    CONTROL_JoyAxes[MAXJOYAXES];
static controlaxistype    CONTROL_LastMouseAxes[MAXMOUSEAXES];
static controlaxistype    CONTROL_LastJoyAxes[MAXJOYAXES];
static int32   CONTROL_MouseAxesScale[MAXMOUSEAXES];
static int32   CONTROL_JoyAxesScale[MAXJOYAXES];
static int32   CONTROL_JoyAxesDead[MAXJOYAXES];
static int32   CONTROL_JoyAxesSaturate[MAXJOYAXES];
static int32   CONTROL_MouseButtonState[MAXMOUSEBUTTONS];
static int32   CONTROL_JoyButtonState[MAXJOYBUTTONS];
static int32   CONTROL_MouseButtonClickedTime[MAXMOUSEBUTTONS];
static int32   CONTROL_JoyButtonClickedTime[MAXJOYBUTTONS];
static boolean CONTROL_MouseButtonClickedState[MAXMOUSEBUTTONS];
static boolean CONTROL_JoyButtonClickedState[MAXJOYBUTTONS];
static boolean CONTROL_MouseButtonClicked[MAXMOUSEBUTTONS];
static boolean CONTROL_JoyButtonClicked[MAXJOYBUTTONS];
static byte    CONTROL_MouseButtonClickedCount[MAXMOUSEBUTTONS];
static byte    CONTROL_JoyButtonClickedCount[MAXJOYBUTTONS];
static boolean CONTROL_UserInputCleared[3];
static int32 (*GetTime)(void);
static boolean CONTROL_Started = false;
static int32 ticrate;
static int32 CONTROL_DoubleClickSpeed;


void CONTROL_GetMouseDelta(void)
{
	int32 x,y;

	MOUSE_GetDelta(&x, &y);

	/* What in the name of all things sacred is this?
	if (labs(*x) > labs(*y)) {
		*x /= 3;
	} else {
		*y /= 3;
	}
	*y = *y * 96;
	*x = (*x * 32 * CONTROL_MouseSensitivity) >> 15;
	*/

	//CONTROL_MouseAxes[0].analog = mulscale8(x, CONTROL_MouseSensitivity);
	//CONTROL_MouseAxes[1].analog = mulscale6(y, CONTROL_MouseSensitivity);

	// DOS axis coefficients = 32,96
	CONTROL_MouseAxes[0].analog = (x * CONTROL_MouseSensitivity) >> 15;
	CONTROL_MouseAxes[1].analog = (y * CONTROL_MouseSensitivity) >> 15;
}

int32 CONTROL_GetMouseSensitivity(void)
{
	return (CONTROL_MouseSensitivity - MINIMUMMOUSESENSITIVITY);
}

void CONTROL_SetMouseSensitivity(int32 newsensitivity)
{
	CONTROL_MouseSensitivity = newsensitivity + MINIMUMMOUSESENSITIVITY;
}

void CONTROL_SetJoyAxisDead(int32 axis, int32 newdead)
{
    if ((unsigned)axis >= MAXJOYAXES) return;
    if (newdead < 0) newdead = 0;
    if (newdead > 32767) newdead = 32767;
    CONTROL_JoyAxesDead[axis] = newdead;
}

void CONTROL_SetJoyAxisSaturate(int32 axis, int32 newsatur)
{
    if ((unsigned)axis >= MAXJOYAXES) return;
    if (newsatur < 0) newsatur = 0;
    if (newsatur > 32767) newsatur = 32767;
    CONTROL_JoyAxesSaturate[axis] = newsatur;
}

boolean CONTROL_StartMouse(void)
{
	CONTROL_NumMouseButtons = MAXMOUSEBUTTONS;
	return MOUSE_Init();
}

void CONTROL_GetJoyAbs( void )
{
}

void CONTROL_FilterJoyDelta(void)
{
}

void CONTROL_GetJoyDelta( void )
{
	int32 i, delta, sign;

    // for paired axes, like with a dual-stick controller,
    // it would make more sense to use a circular dead-zone

	for (i=0; i<joynumaxes; i++) {
        sign = (joyaxis[i] < 0) ? -1 : 1;
        delta = abs(joyaxis[i]);
        if (delta < CONTROL_JoyAxesDead[i]) {
            delta = 0;
        } else if (delta > CONTROL_JoyAxesSaturate[i]) {
            delta = 32767;
        } else {
            // scale the axis delta value to cover the clipped range
            delta = (32767 * (delta - CONTROL_JoyAxesDead[i]))
                / (CONTROL_JoyAxesSaturate[i] - CONTROL_JoyAxesDead[i]);
        }

        // apply a cube transform to make the axis react more strongly
        // the further it is pushed. The multiply-dividing is to avoid
        // overflowing the 32bit integer. It's really doing
        //    (delta*delta*delta) / (32767*32767)
        // to yield 0-32767
        delta = (((delta * delta) / 32767) * delta) / 32767;

		CONTROL_JoyAxes[i].analog = delta * sign;
    }
}


void CONTROL_SetJoyScale( void )
{
}

void CONTROL_CenterJoystick
   (
   void ( *CenterCenter )( void ),
   void ( *UpperLeft )( void ),
   void ( *LowerRight )( void ),
   void ( *CenterThrottle )( void ),
   void ( *CenterRudder )( void )
   )
{
   (void)CenterCenter;
   (void)UpperLeft;
   (void)LowerRight;
   (void)CenterThrottle;
   (void)CenterRudder;
}

boolean CONTROL_StartJoy(int32 joy)
{
	(void)joy;
	return (inputdevices & 4) == 4;
}

void CONTROL_ShutJoy(int32 joy)
{
	(void)joy;
	CONTROL_JoyPresent = false;
}

int32 CONTROL_GetTime(void)
{
	static int32 t = 0;
	t += 5;
	return t;
}

boolean CONTROL_CheckRange(int32 which)
{
	if ((uint32)which < (uint32)CONTROL_NUM_FLAGS) return false;
	//Error("CONTROL_CheckRange: Index %d out of valid range for %d control flags.",
	//	which, CONTROL_NUM_FLAGS);
	return true;
}

void CONTROL_SetFlag(int32 which, boolean active)
{
	if (CONTROL_CheckRange(which)) return;

	if (CONTROL_Flags[which].toggle == INSTANT_ONOFF) {
		CONTROL_Flags[which].active = active;
	} else {
		if (active) {
			CONTROL_Flags[which].buttonheld = false;
		} else if (CONTROL_Flags[which].buttonheld == false) {
			CONTROL_Flags[which].buttonheld = true;
			CONTROL_Flags[which].active = (CONTROL_Flags[which].active ? false : true);
		}
	}
}

boolean CONTROL_KeyboardFunctionPressed(int32 which)
{
	boolean key1 = 0, key2 = 0;

	if (CONTROL_CheckRange(which)) return false;

	if (!CONTROL_Flags[which].used) return false;

	if (CONTROL_KeyMapping[which].key1 != KEYUNDEFINED)
		key1 = KB_KeyDown[ CONTROL_KeyMapping[which].key1 ] ? true : false;

	if (CONTROL_KeyMapping[which].key2 != KEYUNDEFINED)
		key2 = KB_KeyDown[ CONTROL_KeyMapping[which].key2 ] ? true : false;

	return (key1 | key2);
}

void CONTROL_ClearKeyboardFunction(int32 which)
{
	if (CONTROL_CheckRange(which)) return;

	if (!CONTROL_Flags[which].used) return;

	if (CONTROL_KeyMapping[which].key1 != KEYUNDEFINED)
		KB_KeyDown[ CONTROL_KeyMapping[which].key1 ] = 0;

	if (CONTROL_KeyMapping[which].key2 != KEYUNDEFINED)
		KB_KeyDown[ CONTROL_KeyMapping[which].key2 ] = 0;
}

void CONTROL_DefineFlag( int32 which, boolean toggle )
{
	if (CONTROL_CheckRange(which)) return;

	CONTROL_Flags[which].active     = false;
	CONTROL_Flags[which].used       = true;
	CONTROL_Flags[which].toggle     = toggle;
	CONTROL_Flags[which].buttonheld = false;
	CONTROL_Flags[which].cleared    = 0;
}

boolean CONTROL_FlagActive( int32 which )
{
	if (CONTROL_CheckRange(which)) return false;

	return CONTROL_Flags[which].used;
}

void CONTROL_MapKey( int32 which, kb_scancode key1, kb_scancode key2 )
{
	if (CONTROL_CheckRange(which)) return;

	CONTROL_KeyMapping[which].key1 = key1 ? key1 : KEYUNDEFINED;
	CONTROL_KeyMapping[which].key2 = key2 ? key2 : KEYUNDEFINED;
}

void CONTROL_PrintKeyMap(void)
{
	int32 i;

	for (i=0;i<CONTROL_NUM_FLAGS;i++) {
		buildprintf("function %2d key1=%3x key2=%3x\n",
			i, CONTROL_KeyMapping[i].key1, CONTROL_KeyMapping[i].key2);
	}
}

void CONTROL_PrintControlFlag(int32 which)
{
	buildprintf("function %2d active=%d used=%d toggle=%d buttonheld=%d cleared=%d\n",
		which, CONTROL_Flags[which].active, CONTROL_Flags[which].used,
		CONTROL_Flags[which].toggle, CONTROL_Flags[which].buttonheld,
		CONTROL_Flags[which].cleared);
}

void CONTROL_PrintAxes(void)
{
	int32 i;

	buildprintf("nummouseaxes=%d\n", CONTROL_NumMouseAxes);
	for (i=0;i<CONTROL_NumMouseAxes;i++) {
		buildprintf("axis=%d analog=%d digital1=%d digital2=%d\n",
				i, CONTROL_MouseAxesMap[i].analogmap,
				CONTROL_MouseAxesMap[i].minmap, CONTROL_MouseAxesMap[i].maxmap);
	}

	buildprintf("numjoyaxes=%d\n", CONTROL_NumJoyAxes);
	for (i=0;i<CONTROL_NumJoyAxes;i++) {
		buildprintf("axis=%d analog=%d digital1=%d digital2=%d\n",
				i, CONTROL_JoyAxesMap[i].analogmap,
				CONTROL_JoyAxesMap[i].minmap, CONTROL_JoyAxesMap[i].maxmap);
	}
}

void CONTROL_MapButton( int32 whichfunction, int32 whichbutton, boolean doubleclicked, controldevice device )
{
	controlbuttontype *set;

	if (CONTROL_CheckRange(whichfunction)) whichfunction = BUTTONUNDEFINED;

	switch (device) {
		case controldevice_mouse:
			if ((uint32)whichbutton >= (uint32)MAXMOUSEBUTTONS) {
				//Error("CONTROL_MapButton: button %d out of valid range for %d mouse buttons.",
				//		whichbutton, CONTROL_NumMouseButtons);
				return;
			}
			set = CONTROL_MouseButtonMapping;
			break;

		case controldevice_joystick:
			if ((uint32)whichbutton >= (uint32)MAXJOYBUTTONS) {
				//Error("CONTROL_MapButton: button %d out of valid range for %d joystick buttons.",
				//		whichbutton, CONTROL_NumJoyButtons);
				return;
			}
			set = CONTROL_JoyButtonMapping;
			break;

		default:
			//Error("CONTROL_MapButton: invalid controller device type");
			return;
	}

	if (doubleclicked)
		set[whichbutton].doubleclicked = whichfunction;
	else
		set[whichbutton].singleclicked = whichfunction;
}

void CONTROL_MapAnalogAxis( int32 whichaxis, int32 whichanalog, controldevice device )
{
	controlaxismaptype *set;

	if ((uint32)whichanalog >= (uint32)analog_maxtype) {
		//Error("CONTROL_MapAnalogAxis: analog function %d out of valid range for %d analog functions.",
		//		whichanalog, analog_maxtype);
		return;
	}

	switch (device) {
		case controldevice_mouse:
			if ((uint32)whichaxis >= (uint32)MAXMOUSEAXES) {
				//Error("CONTROL_MapAnalogAxis: axis %d out of valid range for %d mouse axes.",
				//		whichaxis, MAXMOUSEAXES);
				return;
			}

			set = CONTROL_MouseAxesMap;
			break;

		case controldevice_joystick:
			if ((uint32)whichaxis >= (uint32)MAXJOYAXES) {
				//Error("CONTROL_MapAnalogAxis: axis %d out of valid range for %d joystick axes.",
				//		whichaxis, MAXJOYAXES);
				return;
			}

			set = CONTROL_JoyAxesMap;
			break;

		default:
			//Error("CONTROL_MapAnalogAxis: invalid controller device type");
			return;
	}

	set[whichaxis].analogmap = whichanalog;
}

void CONTROL_SetAnalogAxisScale( int32 whichaxis, int32 axisscale, controldevice device )
{
	int32 *set;

	switch (device) {
		case controldevice_mouse:
			if ((uint32)whichaxis >= (uint32)MAXMOUSEAXES) {
				//Error("CONTROL_SetAnalogAxisScale: axis %d out of valid range for %d mouse axes.",
				//		whichaxis, MAXMOUSEAXES);
				return;
			}

			set = CONTROL_MouseAxesScale;
			break;

		case controldevice_joystick:
			if ((uint32)whichaxis >= (uint32)MAXJOYAXES) {
				//Error("CONTROL_SetAnalogAxisScale: axis %d out of valid range for %d joystick axes.",
				//		whichaxis, MAXJOYAXES);
				return;
			}

			set = CONTROL_JoyAxesScale;
			break;

		default:
			//Error("CONTROL_SetAnalogAxisScale: invalid controller device type");
			return;
	}

	set[whichaxis] = axisscale;
}

void CONTROL_MapDigitalAxis( int32 whichaxis, int32 whichfunction, int32 direction, controldevice device )
{
	controlaxismaptype *set;

	if (CONTROL_CheckRange(whichfunction)) whichfunction = AXISUNDEFINED;

	switch (device) {
		case controldevice_mouse:
			if ((uint32)whichaxis >= (uint32)MAXMOUSEAXES) {
				//Error("CONTROL_MapDigitalAxis: axis %d out of valid range for %d mouse axes.",
				//		whichaxis, MAXMOUSEAXES);
				return;
			}

			set = CONTROL_MouseAxesMap;
			break;

		case controldevice_joystick:
			if ((uint32)whichaxis >= (uint32)MAXJOYAXES) {
				//Error("CONTROL_MapDigitalAxis: axis %d out of valid range for %d joystick axes.",
				//		whichaxis, MAXJOYAXES);
				return;
			}

			set = CONTROL_JoyAxesMap;
			break;

		default:
			//Error("CONTROL_MapDigitalAxis: invalid controller device type");
			return;
	}

	switch (direction) {	// JBF: this is all very much a guess. The ASM puzzles me.
		case axis_up:
		case axis_left:
			set[whichaxis].minmap = whichfunction;
			break;
		case axis_down:
		case axis_right:
			set[whichaxis].maxmap = whichfunction;
			break;
		default:
			break;
	}
}

void CONTROL_ClearFlags(void)
{
	int32 i;

	for (i=0;i<CONTROL_NUM_FLAGS;i++)
		CONTROL_Flags[i].used = false;
}

void CONTROL_ClearAssignments( void )
{
	int32 i;

	memset(CONTROL_MouseButtonMapping,  BUTTONUNDEFINED, sizeof(CONTROL_MouseButtonMapping));
	memset(CONTROL_JoyButtonMapping,    BUTTONUNDEFINED, sizeof(CONTROL_JoyButtonMapping));
	memset(CONTROL_KeyMapping,          KEYUNDEFINED,    sizeof(CONTROL_KeyMapping));
	memset(CONTROL_MouseAxesMap,        AXISUNDEFINED,   sizeof(CONTROL_MouseAxesMap));
	memset(CONTROL_JoyAxesMap,          AXISUNDEFINED,   sizeof(CONTROL_JoyAxesMap));
	memset(CONTROL_MouseAxes,           0,               sizeof(CONTROL_MouseAxes));
	memset(CONTROL_JoyAxes,             0,               sizeof(CONTROL_JoyAxes));
	memset(CONTROL_LastMouseAxes,       0,               sizeof(CONTROL_LastMouseAxes));
	memset(CONTROL_LastJoyAxes,         0,               sizeof(CONTROL_LastJoyAxes));
	for (i=0;i<MAXMOUSEAXES;i++)
		CONTROL_MouseAxesScale[i] = NORMALAXISSCALE;
	for (i=0;i<MAXJOYAXES;i++) {
		CONTROL_JoyAxesScale[i] = NORMALAXISSCALE;
        CONTROL_JoyAxesDead[i] = 0;
        CONTROL_JoyAxesSaturate[i] = 32767;
    }
}

static void DoGetDeviceButtons(
	int32 buttons, int32 tm,
	int32 NumButtons,
	int32 *DeviceButtonState,
	int32 *ButtonClickedTime,
	boolean *ButtonClickedState,
	boolean *ButtonClicked,
	byte  *ButtonClickedCount
) {
	int32 i, bs;

	for (i=0;i<NumButtons;i++) {
		bs = (buttons >> i) & 1;

		DeviceButtonState[i] = bs;
		ButtonClickedState[i] = false;

		if (bs) {
			if (ButtonClicked[i] == false) {
				ButtonClicked[i] = true;

				if (ButtonClickedCount[i] == 0 || tm > ButtonClickedTime[i]) {
					ButtonClickedTime[i] = tm + CONTROL_DoubleClickSpeed;
					ButtonClickedCount[i] = 1;
				}
				else if (tm < ButtonClickedTime[i]) {
					ButtonClickedState[i] = true;
					ButtonClickedTime[i]  = 0;
					ButtonClickedCount[i] = 2;
				}
			}
			else if (ButtonClickedCount[i] == 2) {
				ButtonClickedState[i] = true;
			}
		} else {
			if (ButtonClickedCount[i] == 2)
				ButtonClickedCount[i] = 0;

			ButtonClicked[i] = false;
		}
	}
}

void CONTROL_GetDeviceButtons(void)
{
	int32 t;

	t = GetTime();

	if (CONTROL_MouseEnabled) {
		DoGetDeviceButtons(
			MOUSE_GetButtons(), t,
			CONTROL_NumMouseButtons,
			CONTROL_MouseButtonState,
			CONTROL_MouseButtonClickedTime,
			CONTROL_MouseButtonClickedState,
			CONTROL_MouseButtonClicked,
			CONTROL_MouseButtonClickedCount
		);
	}

	if (CONTROL_JoystickEnabled) {
		int32 buttons = joyb;
		DoGetDeviceButtons(
			buttons, t,
			CONTROL_NumJoyButtons,
			CONTROL_JoyButtonState,
			CONTROL_JoyButtonClickedTime,
			CONTROL_JoyButtonClickedState,
			CONTROL_JoyButtonClicked,
			CONTROL_JoyButtonClickedCount
		);
	}
}

void CONTROL_DigitizeAxis(int32 axis, controldevice device)
{
	controlaxistype *set, *lastset;
	int32 delta, shiftval, threshold;

	switch (device) {
		case controldevice_mouse:
			set = CONTROL_MouseAxes;
			lastset = CONTROL_LastMouseAxes;
			shiftval = 0;
			threshold = MINTHRESHOLD;
			break;

		case controldevice_joystick:
			set = CONTROL_JoyAxes;
			lastset = CONTROL_LastJoyAxes;
			shiftval = 8;
			threshold = THRESHOLD;
			break;

		default:
			return;
	}

	delta = set[axis].analog >> shiftval;

	if (delta > 0) {
		if (delta > threshold) {			// if very much in one direction,
			set[axis].digital = 1;				// set affirmative
		} else {
			if (delta > MINTHRESHOLD) {		// if hanging in limbo,
				if (lastset[axis].digital == 1)	// set if in same direction as last time
					set[axis].digital = 1;
			}
		}
	} else if (delta < 0) {
		if (delta < -threshold) {
			set[axis].digital = -1;
		} else {
			if (delta < -MINTHRESHOLD) {
				if (lastset[axis].digital == -1)
					set[axis].digital = -1;
			}
		}
	}
}

void CONTROL_ScaleAxis(int32 axis, controldevice device)
{
	controlaxistype *set;
	int32 *scale;

	switch (device) {
		case controldevice_mouse:
			set = CONTROL_MouseAxes;
			scale = CONTROL_MouseAxesScale;
			break;

		case controldevice_joystick:
			set = CONTROL_JoyAxes;
			scale = CONTROL_JoyAxesScale;
			break;

		default: return;
	}

    set[axis].analog = mulscale16(set[axis].analog, scale[axis]);
}

void CONTROL_ApplyAxis(int32 axis, ControlInfo *info, controldevice device)
{
	controlaxistype *set;
	controlaxismaptype *map;
    int32 shiftval;

	switch (device) {
		case controldevice_mouse:
			set = CONTROL_MouseAxes;
			map = CONTROL_MouseAxesMap;
            shiftval = 0;
			break;

		case controldevice_joystick:
			set = CONTROL_JoyAxes;
			map = CONTROL_JoyAxesMap;

            // joystick axes are |0-32767| so chop them back to something
            // closer to a mouse input range so the game sees similar sort
            // of motion regardless of the device that contributed it
            shiftval = 8;
			break;

		default: return;
	}

	switch (map[axis].analogmap) {
		case analog_turning:          info->dyaw   += set[axis].analog >> shiftval; break;
		case analog_strafing:         info->dx     += set[axis].analog >> shiftval; break;
		case analog_lookingupanddown: info->dpitch += set[axis].analog >> shiftval; break;
		case analog_elevation:        info->dy     += set[axis].analog >> shiftval; break;
		case analog_rolling:          info->droll  += set[axis].analog >> shiftval; break;
		case analog_moving:           info->dz     += set[axis].analog >> shiftval; break;
		default: break;
	}
}

void CONTROL_PollDevices(ControlInfo *info)
{
	int32 i;

	memcpy(CONTROL_LastMouseAxes, CONTROL_MouseAxes, sizeof(CONTROL_MouseAxes));
	memcpy(CONTROL_LastJoyAxes,   CONTROL_JoyAxes,   sizeof(CONTROL_JoyAxes));

	memset(CONTROL_MouseAxes, 0, sizeof(CONTROL_MouseAxes));
	memset(CONTROL_JoyAxes,   0, sizeof(CONTROL_JoyAxes));
	memset(info, 0, sizeof(ControlInfo));

	if (CONTROL_MouseEnabled) {
		CONTROL_GetMouseDelta();

		for (i=0; i<MAXMOUSEAXES; i++) {
			CONTROL_DigitizeAxis(i, controldevice_mouse);
			CONTROL_ScaleAxis(i, controldevice_mouse);
			LIMITCONTROL(&CONTROL_MouseAxes[i].analog);
			CONTROL_ApplyAxis(i, info, controldevice_mouse);
		}
	}
	if (CONTROL_JoystickEnabled) {
		CONTROL_GetJoyDelta();

		for (i=0; i<MAXJOYAXES; i++) {
			CONTROL_DigitizeAxis(i, controldevice_joystick);
			CONTROL_ScaleAxis(i, controldevice_joystick);
			LIMITCONTROL(&CONTROL_JoyAxes[i].analog);
			CONTROL_ApplyAxis(i, info, controldevice_joystick);
		}
	}

	CONTROL_GetDeviceButtons();
}

void CONTROL_AxisFunctionState(int32 *p1)
{
	int32 i, j;

	for (i=0; i<CONTROL_NumMouseAxes; i++) {
		if (!CONTROL_MouseAxes[i].digital) continue;

		if (CONTROL_MouseAxes[i].digital < 0)
			j = CONTROL_MouseAxesMap[i].minmap;
		else
			j = CONTROL_MouseAxesMap[i].maxmap;

		if (j != AXISUNDEFINED)
			p1[j] = 1;
	}

	for (i=0; i<CONTROL_NumJoyAxes; i++) {
		if (!CONTROL_JoyAxes[i].digital) continue;

		if (CONTROL_JoyAxes[i].digital < 0)
			j = CONTROL_JoyAxesMap[i].minmap;
		else
			j = CONTROL_JoyAxesMap[i].maxmap;

		if (j != AXISUNDEFINED)
			p1[j] = 1;
	}
}

void CONTROL_ButtonFunctionState( int32 *p1 )
{
	int32 i, j;

	for (i=0; i<CONTROL_NumMouseButtons; i++) {
		j = CONTROL_MouseButtonMapping[i].doubleclicked;
		if (j != KEYUNDEFINED)
			p1[j] |= CONTROL_MouseButtonClickedState[i];

		j = CONTROL_MouseButtonMapping[i].singleclicked;
		if (j != KEYUNDEFINED)
			p1[j] |= CONTROL_MouseButtonState[i];
	}

	for (i=0; i<CONTROL_NumJoyButtons; i++) {
		j = CONTROL_JoyButtonMapping[i].doubleclicked;
		if (j != KEYUNDEFINED)
			p1[j] |= CONTROL_JoyButtonClickedState[i];

		j = CONTROL_JoyButtonMapping[i].singleclicked;
		if (j != KEYUNDEFINED)
			p1[j] |= CONTROL_JoyButtonState[i];
	}
}

void CONTROL_GetUserInput( UserInput *info )
{
	ControlInfo ci;

	CONTROL_PollDevices( &ci );

	info->dir = dir_None;

	// checks if CONTROL_UserInputDelay is too far in the future due to clock skew?
	if (GetTime() + ((ticrate * USERINPUTDELAY) / 1000) < CONTROL_UserInputDelay)
		CONTROL_UserInputDelay = -1;

	if (GetTime() >= CONTROL_UserInputDelay) {
		if (CONTROL_MouseAxes[1].digital == -1)
			info->dir = dir_North;
		else if (CONTROL_MouseAxes[1].digital == 1)
			info->dir = dir_South;
		else if (CONTROL_MouseAxes[0].digital == -1)
			info->dir = dir_West;
		else if (CONTROL_MouseAxes[0].digital == 1)
			info->dir = dir_East;

		if (CONTROL_JoyAxes[1].digital == -1)
			info->dir = dir_North;
		else if (CONTROL_JoyAxes[1].digital == 1)
			info->dir = dir_South;
		else if (CONTROL_JoyAxes[0].digital == -1)
			info->dir = dir_West;
		else if (CONTROL_JoyAxes[0].digital == 1)
			info->dir = dir_East;

		if (CONTROL_JoyButtonState[joybutton_DpadUp])
            info->dir = dir_North;
		else if (CONTROL_JoyButtonState[joybutton_DpadDown])
            info->dir = dir_South;
		else if (CONTROL_JoyButtonState[joybutton_DpadLeft])
            info->dir = dir_West;
		else if (CONTROL_JoyButtonState[joybutton_DpadRight])
            info->dir = dir_East;

        if (KB_KeyDown[sc_kpad_8] || KB_KeyDown[sc_UpArrow])
            info->dir = dir_North;
        else if (KB_KeyDown[sc_kpad_2] || KB_KeyDown[sc_DownArrow])
            info->dir = dir_South;
        else if (KB_KeyDown[sc_kpad_4] || KB_KeyDown[sc_LeftArrow])
            info->dir = dir_West;
        else if (KB_KeyDown[sc_kpad_6] || KB_KeyDown[sc_RightArrow])
            info->dir = dir_East;
    }

	info->button0 = CONTROL_MouseButtonState[mousebutton_Left] | CONTROL_JoyButtonState[joybutton_A];
	info->button1 = CONTROL_MouseButtonState[mousebutton_Right] | CONTROL_JoyButtonState[joybutton_B];

	if (KB_KeyDown[BUTTON0_SCAN_1] || KB_KeyDown[BUTTON0_SCAN_2] || KB_KeyDown[BUTTON0_SCAN_3])
		info->button0 = 1;
	if (KB_KeyDown[BUTTON1_SCAN])
		info->button1 = 1;

	if (CONTROL_UserInputCleared[1]) {
		if (!info->button0)
			CONTROL_UserInputCleared[1] = false;
		else
			info->button0 = false;
	}
	if (CONTROL_UserInputCleared[2]) {
		if (!info->button1)
			CONTROL_UserInputCleared[2] = false;
		else
			info->button1 = false;
	}
}

void CONTROL_ClearUserInput( UserInput *info )
{
	switch (info->dir) {
		case dir_North:
		case dir_South:
		case dir_East:
		case dir_West:
			CONTROL_UserInputCleared[0] = true;
			CONTROL_UserInputDelay = GetTime() + ((ticrate * USERINPUTDELAY) / 1000);
			break;
		default: break;
	}
	if (info->button0) CONTROL_UserInputCleared[1] = true;
	if (info->button1) CONTROL_UserInputCleared[2] = true;
}

void CONTROL_ClearButton( int32 whichbutton )
{
	if (CONTROL_CheckRange( whichbutton )) return;
	BUTTONCLEAR( whichbutton );
	CONTROL_Flags[whichbutton].cleared = true;
}

void CONTROL_GetInput( ControlInfo *info )
{
	int32 i, periphs[CONTROL_NUM_FLAGS];

	CONTROL_PollDevices( info );

	memset(periphs, 0, sizeof(periphs));
	CONTROL_ButtonFunctionState(periphs);
	CONTROL_AxisFunctionState(periphs);

	CONTROL_ButtonHeldState1 = CONTROL_ButtonState1;
	CONTROL_ButtonHeldState2 = CONTROL_ButtonState2;
	CONTROL_ButtonState1 = CONTROL_ButtonState2 = 0;

	for (i=0; i<CONTROL_NUM_FLAGS; i++) {
		CONTROL_SetFlag(i, CONTROL_KeyboardFunctionPressed(i) | periphs[i]);

		if (CONTROL_Flags[i].cleared == false) BUTTONSET(i, CONTROL_Flags[i].active);
		else if (CONTROL_Flags[i].active == false) CONTROL_Flags[i].cleared = 0;
	}
}

void CONTROL_WaitRelease( void )
{
}

void CONTROL_Ack( void )
{
}

boolean CONTROL_Startup(controltype which, int32 ( *TimeFunction )( void ), int32 ticspersecond)
{
	int32 i;

	(void)which;

	if (CONTROL_Started) return false;

	if (TimeFunction) GetTime = TimeFunction;
	else GetTime = CONTROL_GetTime;

	ticrate = ticspersecond;

	CONTROL_DoubleClickSpeed = (ticspersecond*57)/100;
	if (CONTROL_DoubleClickSpeed <= 0)
		CONTROL_DoubleClickSpeed = 1;

	if (initinput()) return true;

	CONTROL_MousePresent = CONTROL_MouseEnabled    = false;
	CONTROL_JoyPresent   = CONTROL_JoystickEnabled = false;
	CONTROL_NumMouseButtons = CONTROL_NumJoyButtons = 0;
	CONTROL_NumMouseAxes    = CONTROL_NumJoyAxes    = 0;
	KB_Startup();

	CONTROL_NumMouseAxes      = MAXMOUSEAXES;
	CONTROL_NumMouseButtons   = MAXMOUSEBUTTONS;
	CONTROL_MousePresent      = MOUSE_Init();
	CONTROL_MouseEnabled      = CONTROL_MousePresent;

	CONTROL_NumJoyAxes    = min(MAXJOYAXES,joynumaxes);
	CONTROL_NumJoyButtons = min(MAXJOYBUTTONS,joynumbuttons);
	CONTROL_JoyPresent    = CONTROL_StartJoy(0);
	CONTROL_JoystickEnabled = CONTROL_JoyPresent;

	if (CONTROL_MousePresent)
		buildprintf("CONTROL_Startup: Mouse Present\n");
	if (CONTROL_JoyPresent)
		buildprintf("CONTROL_Startup: Joystick Present\n");

	CONTROL_ButtonState1     = 0;
	CONTROL_ButtonState2     = 0;
	CONTROL_ButtonHeldState1 = 0;
	CONTROL_ButtonHeldState2 = 0;

	memset(CONTROL_UserInputCleared, 0, sizeof(CONTROL_UserInputCleared));

	for (i=0; i<CONTROL_NUM_FLAGS; i++)
		CONTROL_Flags[i].used = false;

	CONTROL_Started = true;

	return false;
}

void CONTROL_Shutdown(void)
{
	if (!CONTROL_Started) return;

	CONTROL_JoyPresent = false;

	MOUSE_Shutdown();
	uninitinput();

	CONTROL_Started = false;
}

