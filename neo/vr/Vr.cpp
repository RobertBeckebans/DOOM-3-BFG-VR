/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 2013-2021 Samson Koz and contributors
Copyright (C) 2024 Robert Beckebans

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include"precompiled.h"
#pragma hdrstop

#undef strncmp
#undef vsnprintf
#undef _vsnprintf

#include "Vr.h"
#include "Voice.h"
#include "d3xp/Game_local.h"
#ifdef _WIN32
	#include "sys\win32\win_local.h"
#endif
#include "d3xp/physics/Clip.h"

#include "../renderer/Framebuffer.h"

// *** Oculus HMD Variables

idCVar vr_pixelDensity( "vr_pixelDensity", "1", CVAR_FLOAT | CVAR_GAME, "" );
idCVar vr_enable( "vr_enable", "1", CVAR_INTEGER | CVAR_INIT | CVAR_GAME, "Enable VR mode. 0 = Disabled 1 = Enabled." );
idCVar vr_scale( "vr_scale", "1.0", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "World scale. Everything virtual is this times as big." );
idCVar vr_useOculusProfile( "vr_useOculusProfile", "1", CVAR_INTEGER | CVAR_ARCHIVE | CVAR_GAME, "TODO REMOVE, Use official profile values. 0 = use user defined profile, 1 = use official profile." );
idCVar vr_manualIPDEnable( "vr_manualIPDEnable", "0", CVAR_INTEGER | CVAR_ARCHIVE | CVAR_GAME, " Override the HMD provided IPD value with value in vr_manualIPD 0 = disable 1= use manual iPD\n" );
idCVar vr_manualIPD( "vr_manualIPD", "64", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "User defined IPD value in MM" );
idCVar vr_manualHeight( "vr_manualHeight", "70", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "Unused. User defined player height in inches" );
idCVar vr_minLoadScreenTime( "vr_minLoadScreenTime", "6000", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "Min time to display load screens in ms.", 0.0f, 10000.0f );
idCVar vr_useFloorHeight( "vr_useFloorHeight", "0", CVAR_INTEGER | CVAR_ARCHIVE | CVAR_GAME, "0 = Custom eye height. 1 = Marine Eye Height. 2 = Normal View Height. 3 = make floor line up by Doomguy crouching. 4 = make everything line up by scaling world to your height.", 0, 4 );
idCVar vr_normalViewHeight( "vr_normalViewHeight", "73", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "Height of player's view while standing, in real world inches." );

idCVar vr_weaponHand( "vr_weaponHand", "0", CVAR_INTEGER | CVAR_ARCHIVE | CVAR_GAME, "Which hand holds weapon.\n 0 = Right hand\n 1 = Left Hand\n", 0, 1 );

//flashlight cvars

idCVar vr_flashlightMode( "vr_flashlightMode", "3", CVAR_INTEGER | CVAR_ARCHIVE | CVAR_GAME, "Flashlight mount.\n0 = Body\n1 = Head\n2 = Gun\n3= Hand ( if motion controls available.)" );

idCVar vr_flashlightBodyPosX( "vr_flashlightBodyPosX", "0", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "Flashlight vertical offset for body mount." );
idCVar vr_flashlightBodyPosY( "vr_flashlightBodyPosY", "0", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "Flashlight horizontal offset for body mount." );
idCVar vr_flashlightBodyPosZ( "vr_flashlightBodyPosZ", "0", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "Flashlight forward offset for body mount." );

idCVar vr_flashlightHelmetPosX( "vr_flashlightHelmetPosX", "6", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "Flashlight vertical offset for helmet mount." );
idCVar vr_flashlightHelmetPosY( "vr_flashlightHelmetPosY", "-6", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "Flashlight horizontal offset for helmet mount." );
idCVar vr_flashlightHelmetPosZ( "vr_flashlightHelmetPosZ", "-20", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "Flashlight forward offset for helmet mount." );

idCVar vr_offHandPosX( "vr_offHandPosX", "0", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "X position for off hand when not using motion controls." );
idCVar vr_offHandPosY( "vr_offHandPosY", "0", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "Y position for off hand when not using motion controls." );
idCVar vr_offHandPosZ( "vr_offHandPosZ", "0", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "Z position for off hand when not using motion controls." );

idCVar vr_forward_keyhole( "vr_forward_keyhole", "11.25", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "Forward movement keyhole in deg. If view is inside body direction +/- this value, forward movement is in view direction, not body direction" );

idCVar vr_PDAfixLocation( "vr_PDAfixLocation", "0", CVAR_BOOL | CVAR_ARCHIVE | CVAR_GAME, "Fix PDA position in space in front of player\n instead of holding in hand." );

idCVar vr_weaponPivotOffsetForward( "vr_weaponPivotOffsetForward", "3", CVAR_GAME | CVAR_ARCHIVE | CVAR_FLOAT, "" );
idCVar vr_weaponPivotOffsetHorizontal( "vr_weaponPivotOffsetHorizontal", "0", CVAR_GAME | CVAR_ARCHIVE | CVAR_FLOAT, "" );
idCVar vr_weaponPivotOffsetVertical( "vr_weaponPivotOffsetVertical", "0", CVAR_GAME | CVAR_ARCHIVE | CVAR_FLOAT, "" );
idCVar vr_weaponPivotForearmLength( "vr_weaponPivotForearmLength", "16", CVAR_GAME | CVAR_ARCHIVE | CVAR_FLOAT, "" );;

idCVar vr_guiScale( "vr_guiScale", "1", CVAR_FLOAT | CVAR_RENDERER | CVAR_ARCHIVE, "scale reduction factor for full screen menu/pda scale in VR", 0.0001f, 1.0f ); // Koz allow scaling of full screen guis/pda
idCVar vr_guiSeparation( "vr_guiSeparation", ".01", CVAR_FLOAT | CVAR_ARCHIVE, " Screen separation value for fullscreen guis." );

idCVar vr_guiMode( "vr_guiMode", "2", CVAR_INTEGER | CVAR_GAME | CVAR_ARCHIVE, "Gui interaction mode.\n 0 = Weapon aim as cursor\n 1 = Look direction as cursor\n 2 = Touch screen\n" );

idCVar vr_hudScale( "vr_hudScale", "1.0", CVAR_FLOAT | CVAR_GAME | CVAR_ARCHIVE, "Hud scale", 0.1f, 2.0f );
idCVar vr_hudPosHor( "vr_hudPosHor", "0", CVAR_FLOAT | CVAR_GAME | CVAR_ARCHIVE, "Hud Horizontal offset in inches" );
idCVar vr_hudPosVer( "vr_hudPosVer", "7", CVAR_FLOAT | CVAR_GAME | CVAR_ARCHIVE, "Hud Vertical offset in inches" );
idCVar vr_hudPosDis( "vr_hudPosDis", "32", CVAR_FLOAT | CVAR_GAME | CVAR_ARCHIVE, "Hud Distance from view in inches" );
idCVar vr_hudPosAngle( "vr_hudPosAngle", "30", CVAR_FLOAT | CVAR_GAME | CVAR_ARCHIVE, "Hud View Angle" );
idCVar vr_hudPosLock( "vr_hudPosLock", "1", CVAR_INTEGER | CVAR_GAME | CVAR_ARCHIVE, "Lock Hud to:  0 = Face, 1 = Body" );


idCVar vr_hudType( "vr_hudType", "2", CVAR_INTEGER | CVAR_GAME | CVAR_ARCHIVE, "VR Hud Type. 0 = Disable.\n1 = Full\n2=Look Activate", 0, 2 );
idCVar vr_hudRevealAngle( "vr_hudRevealAngle", "48", CVAR_FLOAT | CVAR_GAME | CVAR_ARCHIVE, "HMD pitch to reveal HUD in look activate mode." );
idCVar vr_hudTransparency( "vr_hudTransparency", "1", CVAR_FLOAT | CVAR_GAME | CVAR_ARCHIVE, " Hud transparency. 0.0 = Invisible thru 1.0 = full", 0.0, 100.0 );
idCVar vr_hudOcclusion( "vr_hudOcclusion", "1", CVAR_BOOL | CVAR_GAME | CVAR_ARCHIVE, " Hud occlusion. 0 = Objects occlude HUD, 1 = No occlusion " );
idCVar vr_hudHealth( "vr_hudHealth", "1", CVAR_BOOL | CVAR_GAME | CVAR_ARCHIVE, "Show Armor/Health in Hud." );
idCVar vr_hudAmmo( "vr_hudAmmo", "1", CVAR_BOOL | CVAR_GAME | CVAR_ARCHIVE, "Show Ammo in Hud." );
idCVar vr_hudPickUps( "vr_hudPickUps", "1", CVAR_BOOL | CVAR_GAME | CVAR_ARCHIVE, "Show item pick ups in Hud." );
idCVar vr_hudTips( "vr_hudTips", "1", CVAR_BOOL | CVAR_GAME | CVAR_ARCHIVE, "Show tips Hud." );
idCVar vr_hudLocation( "vr_hudLocation", "1", CVAR_BOOL | CVAR_GAME | CVAR_ARCHIVE, "Show player location in Hud." );
idCVar vr_hudObjective( "vr_hudObjective", "1", CVAR_BOOL | CVAR_GAME | CVAR_ARCHIVE, "Show objectives in Hud." );
idCVar vr_hudStamina( "vr_hudStamina", "1", CVAR_BOOL | CVAR_GAME | CVAR_ARCHIVE, "Show stamina in Hud." );
idCVar vr_hudPills( "vr_hudPills", "1", CVAR_BOOL | CVAR_GAME | CVAR_ARCHIVE, "Show weapon pills in Hud." );
idCVar vr_hudComs( "vr_hudComs", "1", CVAR_BOOL | CVAR_GAME | CVAR_ARCHIVE, "Show communications in Hud." );
idCVar vr_hudWeap( "vr_hudWeap", "1", CVAR_BOOL | CVAR_GAME | CVAR_ARCHIVE, "Show weapon pickup/change icons in Hud." );
idCVar vr_hudNewItems( "vr_hudNewItems", "1", CVAR_BOOL | CVAR_GAME | CVAR_ARCHIVE, "Show new items acquired in Hud." );
idCVar vr_hudFlashlight( "vr_hudFlashlight", "1", CVAR_BOOL | CVAR_GAME | CVAR_ARCHIVE, "Show flashlight in Hud." );
idCVar vr_hudLowHealth( "vr_hudLowHealth", "20", CVAR_INTEGER | CVAR_GAME | CVAR_ARCHIVE, " 0 = Disable, otherwise force hud if heath below this value." );

idCVar vr_voiceRepeat( "vr_voiceRepeat", "0", CVAR_BOOL, "1 = computer speaks back whatever commands or lines you say" );
idCVar vr_voiceMinVolume( "vr_voiceMinVolume", "2", CVAR_INTEGER | CVAR_GAME | CVAR_ARCHIVE, "Minimum volume required to recognise voice commands. Out of 100. Set this higher if background sounds trigger voice commands.", 0, 100 );
idCVar vr_voiceCommands( "vr_voiceCommands", "2", CVAR_INTEGER | CVAR_GAME | CVAR_ARCHIVE, "Enable voice commands. 0 = none, 1 = menus, 2 = menus and weapons", 0, 2 );
idCVar vr_voicePushToTalk( "vr_voicePushToTalk", "0", CVAR_INTEGER | CVAR_ARCHIVE, "'Push to Talk' button must be pressed before voice commands recognized\n 0 = disabled, 1 = enabled \n", 0, 1 );
idCVar vr_talkWakeMonsters( "vr_talkWakeMonsters", "1", CVAR_INTEGER | CVAR_GAME | CVAR_ARCHIVE, "Talking wakes monsters. 0 = no, 1 = both methods, 2 = like flashlight, 3 = like weapon", 0, 3 );
idCVar vr_talkWakeMonsterRadius( "vr_talkWakeMonsterRadius", "120", CVAR_FLOAT | CVAR_GAME | CVAR_ARCHIVE, "Radius in inches within which talking quietly can wake enemies. Talking louder wakes monsters further than this." );
idCVar vr_talkMode( "vr_talkMode", "2", CVAR_INTEGER | CVAR_GAME | CVAR_ARCHIVE, "Talk to NPC 0 = buttons, 1 = buttons or voice, 2 = voice only, 3 = voice no cursor", 0, 3 );
idCVar vr_tweakTalkCursor( "vr_tweakTalkCursor", "25", CVAR_FLOAT | CVAR_GAME | CVAR_ARCHIVE, "Tweak talk cursor y pos in VR. % val", 0, 99 );

idCVar vr_wristStatMon( "vr_wristStatMon", "1", CVAR_INTEGER | CVAR_ARCHIVE, "Use wrist status monitor. 0 = Disable 1 = Right Wrist 2 = Left Wrist " );

// Koz display windows monitor name in the resolution selection menu, helpful to ID which is the rift if using extended mode
idCVar vr_listMonitorName( "vr_listMonitorName", "0", CVAR_BOOL | CVAR_ARCHIVE | CVAR_GAME, "List monitor name with resolution." );

idCVar vr_disableWeaponAnimation( "vr_disableWeaponAnimation", "1", CVAR_BOOL | CVAR_ARCHIVE | CVAR_GAME, "Disable weapon animations in VR. ( 1 = disabled )" );
idCVar vr_headKick( "vr_headKick", "0", CVAR_BOOL | CVAR_ARCHIVE | CVAR_GAME, "Damage can 'kick' the players view. 0 = Disabled in VR." );
//idCVar vr_showBody( "vr_showBody", "1", CVAR_BOOL | CVAR_ARCHIVE | CVAR_GAME, "Dont change this! Will be removed shortly, modifying will cause the player to have extra hands." );
idCVar vr_joystickMenuMapping( "vr_joystickMenuMapping", "1", CVAR_BOOL | CVAR_ARCHIVE | CVAR_GAME, " Use alternate joy mapping\n in menus/PDA.\n 0 = D3 Standard\n 1 = VR Mode.\n(Both joys can nav menus,\n joy r/l to change\nselect area in PDA." );


idCVar vr_deadzonePitch( "vr_deadzonePitch", "90", CVAR_FLOAT | CVAR_GAME | CVAR_ARCHIVE, "Vertical Aim Deadzone", 0, 180 );
idCVar vr_deadzoneYaw( "vr_deadzoneYaw", "30", CVAR_FLOAT | CVAR_GAME | CVAR_ARCHIVE, "Horizontal Aim Deadzone", 0, 180 );
idCVar vr_comfortDelta( "vr_comfortDelta", "10", CVAR_FLOAT | CVAR_GAME | CVAR_ARCHIVE, "Comfort Mode turning angle ", 0, 180 );
idCVar vr_comfortJetStrafeDelta( "vr_comfortJetStrafeDelta", "90", CVAR_FLOAT | CVAR_GAME | CVAR_ARCHIVE, "Comfort Mode jetStrafe angle ", 0, 90 );

idCVar vr_headingBeamMode( "vr_headingBeamMode", "3", CVAR_INTEGER | CVAR_GAME | CVAR_ARCHIVE, "0 = disabled, 1 = solid, 2 = arrows, 3 = scrolling arrows" );

idCVar vr_weaponSight( "vr_weaponSight", "0", CVAR_INTEGER | CVAR_ARCHIVE, "Weapon Sight.\n 0 = Lasersight\n 1 = Red dot\n 2 = Circle dot\n 3 = Crosshair\n 4 = Beam + Dot\n" );
idCVar vr_weaponSightToSurface( "vr_weaponSightToSurface", "1", CVAR_INTEGER | CVAR_ARCHIVE, "Map sight to surface. 0 = Disabled 1 = Enabled\n" );

idCVar vr_motionWeaponPitchAdj( "vr_motionWeaponPitchAdj", "40", CVAR_FLOAT | CVAR_ARCHIVE, "Weapon controller pitch adjust" );
idCVar vr_motionFlashPitchAdj( "vr_motionFlashPitchAdj", "40", CVAR_FLOAT | CVAR_ARCHIVE, "Flashlight controller pitch adjust" );

idCVar vr_nodalX( "vr_nodalX", "-3", CVAR_FLOAT | CVAR_ARCHIVE, "Forward offset from eyes to neck" );
idCVar vr_nodalZ( "vr_nodalZ", "-6", CVAR_FLOAT | CVAR_ARCHIVE, "Vertical offset from neck to eye height" );


idCVar vr_controllerOffsetX( "vr_controllerOffsetX", "-3.5", CVAR_FLOAT | CVAR_ARCHIVE, "Controller X offset to handle center" ); // these values work for steam
idCVar vr_controllerOffsetY( "vr_controllerOffsetY", "0", CVAR_FLOAT | CVAR_ARCHIVE, "Controller Y offset to handle center" );
idCVar vr_controllerOffsetZ( "vr_controllerOffsetZ", "-.5", CVAR_FLOAT | CVAR_ARCHIVE, "Controller Z offset to handle center" );

idCVar vr_mountx( "vr_mountx", "0", CVAR_FLOAT | CVAR_ARCHIVE, "If motion controller mounted on object, X offset from controller to object handle.\n (Eg controller mounted on Topshot)" );
idCVar vr_mounty( "vr_mounty", "0", CVAR_FLOAT | CVAR_ARCHIVE, "If motion controller mounted on object, Y offset from controller to object handle.\n (Eg controller mounted on Topshot)" );
idCVar vr_mountz( "vr_mountz", "0", CVAR_FLOAT | CVAR_ARCHIVE, "If motion controller mounted on object, Z offset from controller to object handle.\n (Eg controller mounted on Topshot)" );

idCVar vr_mountedWeaponController( "vr_mountedWeaponController", "0", CVAR_BOOL | CVAR_ARCHIVE, "If physical controller mounted on object (eg topshot), enable this to apply mounting offsets\n0=disabled 1 = enabled" );

idCVar vr_3dgui( "vr_3dgui", "1", CVAR_BOOL | CVAR_ARCHIVE, "3d effects for in game guis. 0 = disabled 1 = enabled\n" );
idCVar vr_shakeAmplitude( "vr_shakeAmplitude", "1.0", CVAR_FLOAT | CVAR_ARCHIVE, "Screen shake amplitude 0.0 = disabled to 1.0 = full\n", 0.0f, 1.0f );


idCVar vr_controllerGamepad( "vr_controllerGamepad", "0", CVAR_INTEGER | CVAR_ARCHIVE, "If 1, use standard controller, not motion controllers\nRestart after changing\n" );

idCVar vr_padDeadzone( "vr_padDeadzone", ".25", CVAR_FLOAT | CVAR_ARCHIVE, "Deadzone for steam pads.\n 0.0 = no deadzone 1.0 = dead\n" );
idCVar vr_jsDeadzone( "vr_jsDeadzone", ".25", CVAR_FLOAT | CVAR_ARCHIVE, "Deadzone for steam joysticks.\n 0.0 = no deadzone 1.0 = dead\n" );
idCVar vr_padToButtonThreshold( "vr_padToButtonThreshold", ".7", CVAR_FLOAT | CVAR_ARCHIVE, "Threshold value for pad contact\n to register as button press\n .1 high sensitiveity thru\n .99 low sensitivity" );
idCVar vr_knockBack( "vr_knockBack", "0", CVAR_BOOL | CVAR_ARCHIVE | CVAR_GAME, "Enable damage knockback in VR. 0 = Disabled, 1 = Enabled" );
idCVar vr_jumpBounce( "vr_jumpBounce", "0", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "Enable view bounce after jumping. 0 = Disabled, 1 = Full", 0.0f, 1.0f ); // Carl
idCVar vr_stepSmooth( "vr_stepSmooth", "1", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "Enable smoothing when climbing stairs. 0 = Disabled, 1 = Full", 0.0f, 1.0f ); // Carl
idCVar vr_walkSpeedAdjust( "vr_walkSpeedAdjust", "-20", CVAR_FLOAT | CVAR_ARCHIVE, "Player walk speed adjustment in VR. (slow down default movement)" );

idCVar vr_wipPeriodMin( "vr_wipPeriodMin", "10.0", CVAR_FLOAT | CVAR_ARCHIVE, "" );
idCVar vr_wipPeriodMax( "vr_wipPeriodMax", "2000.0", CVAR_FLOAT | CVAR_ARCHIVE, "" );

idCVar vr_wipVelocityMin( "vr_wipVelocityMin", ".05", CVAR_FLOAT | CVAR_ARCHIVE, "" );
idCVar vr_wipVelocityMax( "vr_wipVelocityMax", "2.0", CVAR_FLOAT | CVAR_ARCHIVE, "" );

idCVar vr_headbbox( "vr_headbbox", "10.0", CVAR_FLOAT | CVAR_ARCHIVE, "" );

idCVar vr_pdaPosX( "vr_pdaPosX", "20", CVAR_FLOAT | CVAR_ARCHIVE, "" );
idCVar vr_pdaPosY( "vr_pdaPosY", "0", CVAR_FLOAT | CVAR_ARCHIVE, "" );
idCVar vr_pdaPosZ( "vr_pdaPosZ", "-11", CVAR_FLOAT | CVAR_ARCHIVE, "" );

idCVar vr_pdaPitch( "vr_pdaPitch", "30", CVAR_FLOAT | CVAR_ARCHIVE, "" );

idCVar vr_movePoint( "vr_movePoint", "4", CVAR_INTEGER | CVAR_ARCHIVE, "0: Standard Stick Move, 1: Off Hand = Forward, 2: Look = forward, 3: Weapon Hand = Forward, 4: Left Hand = Forward, 5: Right Hand = Forward", 0, 5 );
idCVar vr_moveClick( "vr_moveClick", "0", CVAR_INTEGER | CVAR_ARCHIVE, " 0 = Normal movement.\n 1 = Click and hold to walk, run button to run.\n 2 = Click to start walking, then touch only. Run btn to run.\n 3 = Click to start walking, hold click to run.\n 4 = Click to start walking, then click toggles run\n" );
idCVar vr_playerBodyMode( "vr_playerBodyMode", "1", CVAR_INTEGER | CVAR_GAME | CVAR_ARCHIVE, "Player body mode:\n0 = Display full body\n1 = Just Hands \n2 = Weapons only\n" );

idCVar vr_crouchMode( "vr_crouchMode", "0", CVAR_INTEGER | CVAR_GAME | CVAR_ARCHIVE, "Crouch Mode:\n 0 = Full motion crouch (In game matches real life)\n 1 = Crouch anim triggered by smaller movement." );
idCVar vr_crouchTriggerDist( "vr_crouchTriggerDist", "7", CVAR_FLOAT | CVAR_ARCHIVE, " Distance ( in real-world inches ) player must crouch in real life to toggle crouch\n" );
idCVar vr_crouchHideBody( "vr_crouchHideBody", "0", CVAR_FLOAT | CVAR_ARCHIVE, "Hide body ( if displayed )  when crouching. 0 = Dont hide, 1 = hide." );
idCVar vr_frameCheck( "vr_frameCheck", "0", CVAR_INTEGER | CVAR_ARCHIVE, "0 = bypass frame check" );

idCVar vr_forceOculusAudio( "vr_forceOculusAudio", "1", CVAR_BOOL | CVAR_ARCHIVE, "Request openAL to search for Rift headphones instead of default device\nFails to default device if rift not found." );
idCVar vr_stereoMirror( "vr_stereoMirror", "1", CVAR_BOOL | CVAR_ARCHIVE, "Render mirror window with stereo views. 0 = Mono , 1 = Stereo Warped" );

idCVar vr_teleportSkipHandrails( "vr_teleportSkipHandrails", "0", CVAR_INTEGER | CVAR_ARCHIVE, "Teleport aim ingnores handrails. 1 = true" );
idCVar vr_teleportShowAimAssist( "vr_teleportShowAimAssist", "0", CVAR_INTEGER | CVAR_ARCHIVE, "Move telepad target to reflect aim assist. 1 = true" );
idCVar vr_teleportButtonMode( "vr_teleportButtonMode", "0", CVAR_BOOL | CVAR_ARCHIVE, "0 = Press aim, release teleport.\n1 = 1st press aim, 2nd press teleport" );
idCVar vr_teleportHint( "vr_teleportHint", "0", CVAR_BOOL | CVAR_ARCHIVE, "" ); // Koz blech hack - used for now to keep track if the game has issued the player the hint about ducking when the teleport target is red.

idCVar vr_useHandPoses( "vr_useHandPoses", "0", CVAR_BOOL | CVAR_ARCHIVE, "If using oculus touch, enable finger poses when hands are empty or in guis" );
// Koz end
// Carl
idCVar vr_teleport( "vr_teleport", "2", CVAR_INTEGER | CVAR_ARCHIVE, "Player can teleport at will. 0 = disabled, 1 = gun sight, 2 = right hand, 3 = left hand, 4 = head", 0, 4 );
idCVar vr_teleportMode( "vr_teleportMode", "0", CVAR_INTEGER | CVAR_ARCHIVE, "Teleport Mode. 0 = Blink (default), 1 = Doom VFR style (slow time and warp speed), 2 = Doom VFR style + jet strafe)", 0, 2 );
idCVar vr_teleportMaxTravel( "vr_teleportMaxTravel", "950", CVAR_INTEGER | CVAR_ARCHIVE, "Maximum teleport path length/complexity/time. About 250 or 500 are good choices, but must be >= about 950 to use tightrope in MC Underground.", 150, 5000 );
idCVar vr_teleportThroughDoors( "vr_teleportThroughDoors", "0", CVAR_BOOL | CVAR_ARCHIVE, "Player can teleport somewhere visible even if the path to get there takes them through closed (but not locked) doors." );
idCVar vr_motionSickness( "vr_motionSickness", "1", CVAR_INTEGER | CVAR_ARCHIVE, "Motion sickness prevention aids. 0 = None, 1 = Chaperone, 2 = Reduce FOV, 3 = Black Screen, 4 = Black & Chaperone, 5 = Reduce FOV & Chaperone, 6 = Slow Mo, 7 = Slow Mo & Chaperone, 8 = Slow Mo & Reduce FOV, 9 = Slow Mo, Chaperone, Reduce FOV", 0, 9 );

idCVar vr_strobeTime( "vr_strobeTime", "500", CVAR_INTEGER | CVAR_ARCHIVE, "Time in ms between flashes when blacking screen. 0 = no strobe" );
idCVar vr_chaperone( "vr_chaperone", "2", CVAR_INTEGER | CVAR_ARCHIVE, "Chaperone/Guardian mode. 0 = when near, 1 = when throwing, 2 = when melee, 3 = when dodging, 4 = always", 0, 4 );
idCVar vr_chaperoneColor( "vr_chaperoneColor", "0", CVAR_INTEGER | CVAR_ARCHIVE, "Chaperone colour. 0 = default, 1 = black, 2 = grey, 3 = white, 4 = red, 5 = green, 6 = blue, 7 = yellow, 8 = cyan, 9 = magenta, 10 = purple", 0, 10 );

idCVar vr_handSwapsAnalogs( "vr_handSwapsAnalogs", "0", CVAR_BOOL | CVAR_ARCHIVE, "Should swapping the weapon hand affect analog controls (stick or touchpad) or just buttons/triggers? 0 = only swap buttons, 1 = swap all controls" );
idCVar vr_autoSwitchControllers( "vr_autoSwitchControllers", "1", CVAR_BOOL | CVAR_ARCHIVE, "Automatically switch to/from gamepad mode when using gamepad/motion controller. Should be true unless you're trying to use both together, or you get false detections. 0 = no, 1 = yes." );

idCVar vr_instantAccel( "vr_instantAccel", "1", CVAR_BOOL | CVAR_ARCHIVE, "Instant Movement Acceleration. 0 = Disabled 1 = Enabled" );
idCVar vr_shotgunChoke( "vr_shotgunChoke", "0", CVAR_FLOAT | CVAR_ARCHIVE, "% To choke shotgun. 0 = None, 100 = Full Choke\n" );
idCVar vr_headshotMultiplier( "vr_headshotMultiplier", "2.5", CVAR_FLOAT | CVAR_ARCHIVE, "Damage multiplier for headshots when using Fists,Pistol,Shotgun,Chaingun or Plasmagun.", 1, 5 );

//===================================================================



iVr vrCom;
iVr* vrSystem = &vrCom;

iVoice _voice; //avoid nameclash with timidity
iVoice* vrVoice = &_voice;


/*
==============
iVr::iVr()
==============
*/
iVr::iVr()
{
	isActive = false;

	VR_GAME_PAUSED = false;
	PDAforcetoggle = false;
	PDAforced = false;
	PDArising = false;
	gameSavingLoading = false;
	showingIntroVideo = true;
	forceLeftStick = true;	// start the PDA in the left menu.
	pdaToggleTime = Sys_Milliseconds();
	lastSaveTime = Sys_Milliseconds();
	wasSaved = false;
	wasLoaded = false;
	shouldRecenter = false;

	useFBO = false;
	VR_USE_MOTION_CONTROLS = 0;

	scanningPDA = false;

	vrIsBackgroundSaving = false;

	screenSeparation = 0.0f;

	officialIPD = 64.0f;
	officialHeight = 72.0f;

	manualIPD = 64.0f;
	manualHeight = 72.0f;

	hmdPositionTracked = false;

	vrFrameNumber = 0;
	lastPostFrame = 0;


	lastViewOrigin = vec3_zero;
	lastViewAxis = mat3_identity;

	lastCenterEyeOrigin = vec3_zero;
	lastCenterEyeAxis = mat3_identity;

	bodyYawOffset = 0.0f;
	lastHMDYaw = 0.0f;
	lastHMDPitch = 0.0f;
	lastHMDRoll = 0.0f;
	lastHMDViewOrigin = vec3_zero;
	lastHMDViewAxis = mat3_identity;
	headHeightDiff = 0;

	motionMoveDelta = vec3_zero;
	motionMoveVelocity = vec3_zero;
	leanOffset = vec3_zero;
	leanBlankOffset = vec3_zero;
	leanBlankOffsetLengthSqr = 0.0f;
	leanBlank = false;
	isLeaning = false;

	chestDefaultDefined = false;

	currentFlashlightPosition = FLASHLIGHT_BODY;

	handInGui = false;

	handRoll[0] = 0.0f;
	handRoll[1] = 0.0f;

	fingerPose[0] = 0;
	fingerPose[1] = 0;

	angles[3] = { 0.0f };

	swfRenderMode = RENDERING_NORMAL;

	forceRun = false;

	hmdBodyTranslation = vec3_zero;

	VR_AAmode = 0;

	independentWeaponYaw = 0;
	independentWeaponPitch = 0;

	playerDead = false;

	hmdWidth = 0;
	hmdHeight = 0;

	primaryFBOWidth = 0;
	primaryFBOHeight = 0;
	hmdHz = 90;

	hmdFovX = 0.0f;
	hmdFovY = 0.0f;

	hmdPixelScale = 1.0f;
	hmdAspect = 1.0f;

	hmdEyeImage[0] = 0;
	hmdEyeImage[1] = 0;
	hmdCurrentRender[0] = 0;
	hmdCurrentRender[1] = 0;

	// wip stuff
	// wip stuff
	wipNumSteps = 0;
	wipStepState = 0;
	wipLastPeriod = 0;
	wipCurrentDelta = 0.0f;
	wipCurrentVelocity = 0.0f;

	wipTotalDelta = 0.0f;
	wipLastAcces = 0.0f;
	wipAvgPeriod = 0.0f;
	wipTotalDeltaAvg = 0.0f;

	lastRead = 0;
	currentRead = 0;
	updateScreen = false;

	motionControlType = MOTION_NONE;

	bodyMoveAng = 0.0f;
	teleportButtonCount = 0;

	currentFlashMode = vr_flashlightMode.GetInteger();
	renderingSplash = true;

	currentBindingDisplay = "";

	cinematicStartViewYaw = 0.0f;
	cinematicStartPosition = vec3_zero;

	didTeleport = false;
	teleportDir = 0.0f;

	currentHandWorldPosition[0] = vec3_zero;
	currentHandWorldPosition[1] = vec3_zero;
}

idMat4 ConvertSteamVRMatrixToidMat4( const vr::HmdMatrix34_t& matPose )
{
	idMat4 matrixObj(
		matPose.m[0][0], matPose.m[1][0], matPose.m[2][0], matPose.m[3][0],
		matPose.m[0][1], matPose.m[1][1], matPose.m[2][1], matPose.m[3][1],
		matPose.m[0][2], matPose.m[1][2], matPose.m[2][2], matPose.m[3][2],
		matPose.m[0][3], matPose.m[1][3], matPose.m[2][3], 1.0f
	);

	return matrixObj;
}

//==============
// Purpose: Helper to get a string from a tracked device property and turn it
//			into an idStr
//==============
idStr GetTrackedDeviceString( vr::IVRSystem* pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError* peError = NULL )
{
	uint32_t unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty( unDevice, prop, NULL, 0, peError );
	if( unRequiredBufferLen == 0 )
	{
		return "";
	}

	char* pchBuffer = new char[unRequiredBufferLen];
	unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty( unDevice, prop, pchBuffer, unRequiredBufferLen, peError );
	idStr sResult = pchBuffer;
	delete[] pchBuffer;
	return sResult;
}

/*
==============
iVr::OpenVRInit
==============
*/
bool iVr::OpenVRInit()
{
	if( !vr::VR_IsHmdPresent() )
	{
		common->Printf( "No OpenVR HMD detected.\n" );
		return false;
	}

	vr::EVRInitError eError = vr::VRInitError_None;
	m_pHMD = vr::VR_Init( &eError, vr::VRApplication_Scene );

	if( eError != vr::VRInitError_None )
	{
		m_pHMD = NULL;
		common->Printf( "\n Unable to init SteamVR runtime.\n" );
		return false;
	}

	m_pRenderModels = ( vr::IVRRenderModels* )vr::VR_GetGenericInterface( vr::IVRRenderModels_Version, &eError );

	if( !m_pRenderModels )
	{
		m_pHMD = NULL;
		vr::VR_Shutdown();

		common->Printf( " Unable to get render model interface: %s\n", vr::VR_GetVRInitErrorAsEnglishDescription( eError ) );
		return false;
	}

	if( !vr::VRCompositor() )
	{
		common->Printf( "Compositor initialization failed. See log file for details\n" );
		return false;
	}

	vr::VRCompositor()->SetTrackingSpace( vr::TrackingUniverseStanding );

	motionControlType = MOTION_STEAMVR;


	/*
	vr::TrackedDeviceIndex_t deviceLeft,deviceRight;

	deviceLeft = vr::VRSystem()->GetTrackedDeviceIndexForControllerRole( vr::TrackedControllerRole_LeftHand );
	deviceRight = vr::VRSystem()->GetTrackedDeviceIndexForControllerRole( vr::TrackedControllerRole_RightHand );

	common->Printf( "Left Controller %d right Controller %d\n", deviceLeft, deviceRight );

	if ( deviceLeft != -1 || deviceRight != -1  )
	{
	common->Printf( "Tracked controllers detected. MOTION CONTROLS ENABLED\n" );
	VR_USE_MOTION_CONTROLS = true;
	}
	else
	{
	VR_USE_MOTION_CONTROLS = false;
	}
	*/

	VR_USE_MOTION_CONTROLS = true;

	common->Printf( "Getting driver info\n" );
	m_strDriver = "No Driver";
	m_strDisplay = "No Display";

	m_strDriver = GetTrackedDeviceString( m_pHMD, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String );
	m_strDisplay = GetTrackedDeviceString( m_pHMD, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String );

	// get this here so we have a resolution starting point for gl initialization.
	m_pHMD->GetRecommendedRenderTargetSize( &hmdWidth, &hmdHeight );

	hmdHz = ( int )( m_pHMD->GetFloatTrackedDeviceProperty( vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_DisplayFrequency_Float ) + 0.5f );

	officialIPD = m_pHMD->GetFloatTrackedDeviceProperty( vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_UserIpdMeters_Float ) * 100;

	// Leyland's code, used for Reduce FOV motion sickness fix
	float				openVRfovEye[2][4];
	m_pHMD->GetProjectionRaw( vr::Eye_Left,
							  &openVRfovEye[1][0], &openVRfovEye[1][1],
							  &openVRfovEye[1][2], &openVRfovEye[1][3] );

	m_pHMD->GetProjectionRaw( vr::Eye_Right,
							  &openVRfovEye[0][0], &openVRfovEye[0][1],
							  &openVRfovEye[0][2], &openVRfovEye[0][3] );

	screenSeparation =
		0.5f * ( openVRfovEye[1][1] + openVRfovEye[1][0] )
		/ ( openVRfovEye[1][1] - openVRfovEye[1][0] )
		- 0.5f * ( openVRfovEye[0][1] + openVRfovEye[0][0] )
		/ ( openVRfovEye[0][1] - openVRfovEye[0][0] );

	screenSeparation = fabs( screenSeparation ) / 2.0f ;
	com_engineHz.SetInteger( hmdHz );

	common->Printf( "Hmd Driver: %s .\n", m_strDriver.c_str() );
	common->Printf( "Hmd Display: %s .\n", m_strDisplay.c_str() );
	common->Printf( "Hmd HZ %d, width %d, height %d\n", hmdHz, hmdWidth, hmdHeight );
	common->Printf( "Hmd reported IPD in centimeters = %f \n", officialIPD );

	common->Printf( "HMD Left Eye leftTan %f\n", openVRfovEye[1][0] );
	common->Printf( "HMD Left Eye rightTan %f\n", openVRfovEye[1][1] );
	common->Printf( "HMD Left Eye upTan %f\n", openVRfovEye[1][2] );
	common->Printf( "HMD Left Eye downTan %f\n", openVRfovEye[1][3] );

	common->Printf( "HMD Right Eye leftTan %f\n", openVRfovEye[0][0] );
	common->Printf( "HMD Right Eye rightTan %f\n", openVRfovEye[0][1] );
	common->Printf( "HMD Right Eye upTan %f\n", openVRfovEye[0][2] );
	common->Printf( "HMD Right Eye downTan %f\n", openVRfovEye[0][3] );
	common->Printf( "OpenVR HMD Screen separation = %f\n", screenSeparation );

	return true;
}

/*
==============
iVr::HMDInit
==============
*/
void iVr::HMDInit()
{
	isActive = false;

	if( !vr_enable.GetBool() || !OpenVRInit() )
	{
		common->Printf( "No HMD detected.\n VR Disabled\n" );
		return;
	}
	common->Printf( "\n\n HMD Initialized\n" );

	isActive = true;

	common->Printf( "VR_USE_MOTION_CONTROLS Final = %d\n", VR_USE_MOTION_CONTROLS );
}


/*
==============
iVr::HMDShutdown
==============
*/
void iVr::HMDShutdown()
{
	vr::VR_Shutdown();
	m_pHMD = NULL;
}

idMat4 iVr::GetHMDMatrixProjectionEye( vr::Hmd_Eye nEye )
{
	if( !vrSystem->m_pHMD )
	{
		return mat4_default;
	}

	float m_fNearClip = 0.1f;
	float m_fFarClip = 30.0f;

	vr::HmdMatrix44_t mat = vrSystem->m_pHMD->GetProjectionMatrix( nEye, m_fNearClip, m_fFarClip ); // , vr::API_OpenGL );

	return idMat4(
			   mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
			   mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1],
			   mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2],
			   mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]
		   );
}

idMat4 iVr::GetHMDMatrixPoseEye( vr::Hmd_Eye nEye )
{
	if( !vrSystem->m_pHMD )
	{
		return  mat4_default;
	}

	vr::HmdMatrix34_t matEyeRight = vrSystem->m_pHMD->GetEyeToHeadTransform( nEye );
	idMat4 matrixObj(
		matEyeRight.m[0][0], matEyeRight.m[1][0], matEyeRight.m[2][0], 0.0,
		matEyeRight.m[0][1], matEyeRight.m[1][1], matEyeRight.m[2][1], 0.0,
		matEyeRight.m[0][2], matEyeRight.m[1][2], matEyeRight.m[2][2], 0.0,
		matEyeRight.m[0][3], matEyeRight.m[1][3], matEyeRight.m[2][3], 1.0f
	);

	return matrixObj.Inverse();
}

/*
==============
iVr::HMDInitializeDistortion
==============
*/
void iVr::HMDInitializeDistortion()
{
	if( !m_pHMD || !vr_enable.GetBool() )
	{
		isActive = false;
		return;
	}

	{
		m_mat4ProjectionLeft = GetHMDMatrixProjectionEye( vr::Eye_Left );
		m_mat4ProjectionRight = GetHMDMatrixProjectionEye( vr::Eye_Right );
		m_mat4eyePosLeft = GetHMDMatrixPoseEye( vr::Eye_Left );
		m_mat4eyePosRight = GetHMDMatrixPoseEye( vr::Eye_Right );

		m_pHMD->GetProjectionRaw( vr::Eye_Left, &hmdEye[0].projectionOpenVR.projLeft, &hmdEye[0].projectionOpenVR.projRight, &hmdEye[0].projectionOpenVR.projUp, &hmdEye[0].projectionOpenVR.projDown );
		m_pHMD->GetProjectionRaw( vr::Eye_Right, &hmdEye[1].projectionOpenVR.projLeft, &hmdEye[1].projectionOpenVR.projRight, &hmdEye[1].projectionOpenVR.projUp, &hmdEye[1].projectionOpenVR.projDown );

		m_pHMD->GetRecommendedRenderTargetSize( &hmdWidth, &hmdHeight );

		for( int eye = 0; eye < 2; eye++ )
		{
			hmdEye[eye].renderTargetRes.x = hmdWidth  * vr_pixelDensity.GetFloat();
			hmdEye[eye].renderTargetRes.y = hmdHeight * vr_pixelDensity.GetFloat();
		}

		vr::HmdMatrix34_t EyeToHeadTransform;
		EyeToHeadTransform = m_pHMD->GetEyeToHeadTransform( vr::Eye_Right );

		hmdForwardOffset = -EyeToHeadTransform.m[2][3];
		singleEyeIPD = EyeToHeadTransform.m[0][3];
	}

	isActive = true;
	common->Printf( "VR Mode ENABLED.\n" );

	if( !glConfig.framebufferObjectAvailable )
	{
		common->Error( "Framebuffer object not available.  Framebuffer support required for VR.\n" );
	}

	bool fboCreated = false;

	ovrSizei rendertarget;

	primaryFBOWidth = hmdEye[0].renderTargetRes.x;
	primaryFBOHeight = hmdEye[0].renderTargetRes.y;

	if( !fboCreated )
	{
		// create the FBOs
		common->Printf( "Generating FBOs.\n" );
		common->Printf( "Default recommended resolution = %i %i \n", hmdWidth, hmdHeight );
		common->Printf( "Requested pixel density = %f \n", vr_pixelDensity.GetFloat() );
		common->Printf( "\nWorking resolution ( default * pixelDensity ) = %i %i \n", primaryFBOWidth, primaryFBOHeight );

		VR_AAmode = r_multiSamples.GetInteger() == 0 ? VR_AA_NONE : VR_AA_MSAA;

		common->Printf( "vr_FBOAAmode %d r_multisamples %d\n", VR_AAmode, r_multiSamples.GetInteger() );

		/*
		if ( VR_AAmode == VR_AA_FXAA )
		{// enable FXAA

			VR_AAmode = VR_AA_NONE;

		}
		*/

		if( VR_AAmode == VR_AA_MSAA )
		{
			// enable MSAA
			GL_CheckErrors();

			common->Printf( "Creating %d x %d MSAA framebuffer\n", primaryFBOWidth, primaryFBOHeight );
			globalFramebuffers.primaryFBO = new Framebuffer( "_primaryFBO", primaryFBOWidth, primaryFBOHeight, true );

			common->Printf( "Adding Depth/Stencil attachments to MSAA framebuffer\n" );
			globalFramebuffers.primaryFBO->AddDepthStencilBuffer( GL_DEPTH24_STENCIL8 );

			common->Printf( "Adding color attachment to MSAA framebuffer\n" );
			globalFramebuffers.primaryFBO->AddColorBuffer( GL_RGBA, 0 );

			int status = globalFramebuffers.primaryFBO->Check();
			globalFramebuffers.primaryFBO->Error( status );

			common->Printf( "Creating resolve framebuffer\n" );
			globalFramebuffers.resolveFBO = new Framebuffer( "_resolveFBO", primaryFBOWidth, primaryFBOHeight, false ); // Koz
			common->Printf( "Adding Depth/Stencil attachments to framebuffer\n" );
			globalFramebuffers.resolveFBO->AddDepthStencilBuffer( GL_DEPTH24_STENCIL8 );
			common->Printf( "Adding color attachment to framebuffer\n" );
			globalFramebuffers.resolveFBO->AddColorBuffer( GL_RGBA, 0 );

			status = globalFramebuffers.resolveFBO->Check();
			globalFramebuffers.resolveFBO->Error( status );

			if( status = GL_FRAMEBUFFER_COMPLETE )
			{
				useFBO = true;
				fboCreated = true;
			}
			else
			{
				useFBO = false;
				fboCreated = false;
			}

		}

		if( !fboCreated )
		{
			// either AA disabled or AA buffer creation failed. Try creating unaliased FBOs.
			common->Printf( "Creating framebuffer\n" );
			globalFramebuffers.primaryFBO = new Framebuffer( "_primaryFBO", primaryFBOWidth, primaryFBOHeight, false ); // Koz

			common->Printf( "Adding Depth/Stencil attachments to framebuffer\n" );
			globalFramebuffers.primaryFBO->AddDepthStencilBuffer( GL_DEPTH24_STENCIL8 );

			common->Printf( "Adding color attachment to framebuffer\n" );
			globalFramebuffers.primaryFBO->AddColorBuffer( GL_RGBA8, 0 );

			int status = globalFramebuffers.primaryFBO->Check();
			globalFramebuffers.primaryFBO->Error( status );

			if( status = GL_FRAMEBUFFER_COMPLETE )
			{
				useFBO = true;
				fboCreated = true;
			}
			else
			{
				useFBO = false;
				fboCreated = false;
			}
		}

	}

	if( !useFBO )    // not using FBO's, will render to default framebuffer (screen)
	{
		primaryFBOWidth = renderSystem->GetNativeWidth() / 2;
		primaryFBOHeight = renderSystem->GetNativeHeight();
		rendertarget.w = primaryFBOWidth;
		rendertarget.h = primaryFBOHeight;

		for( int eye = 0; eye < 2; eye++ )
		{
			hmdEye[eye].renderTargetRes.x = primaryFBOWidth;
			hmdEye[eye].renderTargetRes.y = primaryFBOHeight;
		}

	}

	globalImages->hudImage->Resize( primaryFBOWidth, primaryFBOHeight );
	globalImages->pdaImage->Resize( primaryFBOWidth, primaryFBOHeight );
	//globalImages->skyBoxFront->Resize( primaryFBOWidth, primaryFBOHeight );
	//globalImages->skyBoxSides->Resize( primaryFBOWidth, primaryFBOHeight );
	globalImages->currentRenderImage->Resize( primaryFBOWidth, primaryFBOHeight );
	globalImages->currentDepthImage->Resize( primaryFBOWidth, primaryFBOHeight );

	common->Printf( "pdaImage size %d %d\n", globalImages->pdaImage->GetUploadWidth(), globalImages->pdaImage->GetUploadHeight() );
	common->Printf( "Hudimage size %d %d\n", globalImages->hudImage->GetUploadWidth(), globalImages->hudImage->GetUploadHeight() );

	{
		float combinedTanHalfFovHorizontal = std::max( std::max( hmdEye[0].projectionOpenVR.projLeft, hmdEye[0].projectionOpenVR.projRight ), std::max( hmdEye[1].projectionOpenVR.projLeft, hmdEye[1].projectionOpenVR.projRight ) );
		float combinedTanHalfFovVertical = std::max( std::max( hmdEye[0].projectionOpenVR.projUp, hmdEye[0].projectionOpenVR.projDown ), std::max( hmdEye[1].projectionOpenVR.projUp, hmdEye[1].projectionOpenVR.projDown ) );
		float horizontalFullFovInRadians = 2.0f * atanf( combinedTanHalfFovHorizontal );

		hmdFovX = RAD2DEG( horizontalFullFovInRadians );
		hmdFovY = RAD2DEG( 2.0 * atanf( combinedTanHalfFovVertical ) );
		hmdAspect = combinedTanHalfFovHorizontal / combinedTanHalfFovVertical;

		common->Printf( "Init Hmd FOV x,y = %f , %f. Aspect = %f\n", hmdFovX, hmdFovY, hmdAspect );
	}

	{
		// override the default steam skybox, initially just set to black.  UpdateScreen can copy static images to skyBoxFront during level loads/saves

		static vr::Texture_t* textures = new vr::Texture_t[6];
		for( int i = 0; i < 6; i++ )
		{
			textures[i].handle = ( unsigned int* )globalImages->skyBoxSides->texnum;
			textures[i].eType = vr::TextureType_OpenGL;
			textures[i].eColorSpace = vr::ColorSpace_Auto;
		}

		//textures[0].handle = (unsigned int*)globalImages->skyBoxFront->texnum;
		textures[0].handle = ( unsigned int* )globalImages->pdaImage->texnum;

		static vr::EVRCompositorError error = vr::VRCompositor()->SetSkyboxOverride( textures, 1 );

		common->Printf( "Compositor error = %d\n", error );
		if( ( int )error != vr::VRCompositorError_None )
		{
			gameLocal.Error( "Failed to set skybox override with error: %d\n", error );
		}

		common->Printf( "Finished setting skybox\n" );
	}

	globalFramebuffers.primaryFBO->Bind();


	{
		// make sure vsync is off.
		wglSwapIntervalEXT( 0 );
		r_swapInterval.SetInteger( 0 );
		r_swapInterval.SetModified();
	}

	GL_CheckErrors();

	{
		do
		{
			vr::VRCompositor()->WaitGetPoses( m_rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0 );
		}
		while( !m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid );

		//Seems to take a few frames before a vaild yaw is returned, so zero the current tracked player position by pulling multiple poses;
		for( int t = 0; t < 20; t++ )
		{
			HMDResetTrackingOriginOffset();
		}
	}
}

/*
==============`
iVr::HMDGetOrientation
==============
*/
void iVr::HMDGetOrientation( idAngles& hmdAngles, idVec3& headPositionDelta, idVec3& bodyPositionDelta, idVec3& absolutePosition, bool resetTrackingOffset )
{
	static int lastFrame = -1;
	static double time = 0.0;
	static int currentlyTracked;
	static int lastFrameReturned = -1;
	static uint lastIdFrame = -1;
	static float lastRoll = 0.0f;
	static float lastPitch = 0.0f;
	static float lastYaw = 0.0f;
	static idVec3 lastHmdPosition = vec3_zero;

	static idVec3 hmdPosition;
	static idVec3 lastHmdPos2 = vec3_zero;
	static idMat3 hmdAxis = mat3_identity;

	static bool	neckInitialized = false;
	static idVec3 initialNeckPosition = vec3_zero;
	static idVec3 currentNeckPosition = vec3_zero;
	static idVec3 lastNeckPosition = vec3_zero;

	static idVec3 lastHeadPositionDelta = vec3_zero;
	static idVec3 lastBodyPositionDelta = vec3_zero;
	static idVec3 lastAbsolutePosition = vec3_zero;

	static vr::TrackedDevicePose_t lastTrackedPoseOpenVR = { 0.0f };

	if( !m_pHMD )
	{
		hmdAngles.roll = 0.0f;
		hmdAngles.pitch = 0.0f;
		hmdAngles.yaw = 0.0f;
		headPositionDelta = vec3_zero;
		bodyPositionDelta = vec3_zero;
		absolutePosition = vec3_zero;
		return;
	}

	lastBodyYawOffset = bodyYawOffset;
	poseLastHmdAngles = poseHmdAngles;
	poseLastHmdHeadPositionDelta = poseHmdHeadPositionDelta;
	poseLastHmdBodyPositionDelta = poseHmdBodyPositionDelta;
	poseLastHmdAbsolutePosition = poseHmdAbsolutePosition;

	if( vr_frameCheck.GetInteger() == 1 && idLib::frameNumber == lastFrame ) //&& !renderingSplash )
	{
		//make sure to return the same values for this frame.
		hmdAngles.roll = lastRoll;
		hmdAngles.pitch = lastPitch;
		hmdAngles.yaw = lastYaw;
		headPositionDelta = lastHeadPositionDelta;
		bodyPositionDelta = lastBodyPositionDelta;

		if( resetTrackingOffset == true )
		{
			trackingOriginOffset = lastHmdPosition;
			trackingOriginHeight = trackingOriginOffset.z;
			if( vr_useFloorHeight.GetInteger() == 0 )
			{
				trackingOriginOffset.z += pm_normalviewheight.GetFloat() + 5 + CM_CLIP_EPSILON - vr_normalViewHeight.GetFloat() / vr_scale.GetFloat();
			}
			else if( vr_useFloorHeight.GetInteger() == 2 )
			{
				trackingOriginOffset.z += 5;
			}
			else if( vr_useFloorHeight.GetInteger() == 3 )
			{
				trackingOriginOffset.z = pm_normalviewheight.GetFloat() + 5 + CM_CLIP_EPSILON;
			}
			else if( vr_useFloorHeight.GetInteger() == 4 )
			{
				float oldScale = vr_scale.GetFloat();
				float h = trackingOriginHeight * oldScale;
				float newScale = h / 73.0f;
				trackingOriginHeight *= oldScale / newScale;
				trackingOriginOffset *= oldScale / newScale;
				vr_scale.SetFloat( newScale );
			}
			common->Printf( "Resetting tracking yaw offset.\n Yaw = %f old offset = %f ", hmdAngles.yaw, trackingOriginYawOffset );
			trackingOriginYawOffset = hmdAngles.yaw;
			common->Printf( "New Tracking yaw offset %f\n", hmdAngles.yaw, trackingOriginYawOffset );
			neckInitialized = false;

			cinematicStartViewYaw = trackingOriginYawOffset;

		}
		common->Printf( "HMDGetOrientation FramCheck Bail == idLib:: framenumber  lf %d  ilfn %d  rendersplash = %d\n", lastFrame, idLib::frameNumber, renderingSplash );
		return;
	}

	lastFrame = idLib::frameNumber;

	{
		if( !m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid )
		{
			common->Printf( "Pose invalid!!\n" );

			headPositionDelta = lastHeadPositionDelta;
			bodyPositionDelta = lastBodyPositionDelta;
			absolutePosition = lastAbsolutePosition;
			hmdAngles.roll = lastRoll;
			hmdAngles.pitch = lastPitch;
			hmdAngles.yaw = lastYaw;
			return;
		}
		//common->Printf( "Pose acquired %d\n", gameLocal.GetTime() );
	}

	{
		m_rmat4DevicePose[vr::k_unTrackedDeviceIndex_Hmd] = ConvertSteamVRMatrixToidMat4( m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking );

		// Koz convert position (in meters) to inch (1 id unit = 1 inch).
		hmdPosition.x = -m_rmat4DevicePose[vr::k_unTrackedDeviceIndex_Hmd][3][2] * ( 100.0f / 2.54f ) / vr_scale.GetFloat();
		hmdPosition.y = -m_rmat4DevicePose[vr::k_unTrackedDeviceIndex_Hmd][3][0] * ( 100.0f / 2.54f ) / vr_scale.GetFloat(); // meters to inches
		hmdPosition.z = m_rmat4DevicePose[vr::k_unTrackedDeviceIndex_Hmd][3][1] * ( 100.0f / 2.54f ) / vr_scale.GetFloat();
	}

	lastHmdPosition = hmdPosition;

	static idQuat poseRot;// = idQuat_zero;
	static idAngles poseAngles = ang_zero;

	{
		static idQuat orientationPose;
		orientationPose = m_rmat4DevicePose[vr::k_unTrackedDeviceIndex_Hmd].ToMat3().ToQuat();

		poseRot.x = orientationPose.z;
		poseRot.y = orientationPose.x;
		poseRot.z = -orientationPose.y;
		poseRot.w = orientationPose.w;
	}

	poseAngles = poseRot.ToAngles();

	hmdAngles.yaw = poseAngles.yaw;
	hmdAngles.roll = poseAngles.roll;
	hmdAngles.pitch = poseAngles.pitch;

	lastRoll = hmdAngles.roll;
	lastPitch = hmdAngles.pitch;
	lastYaw = hmdAngles.yaw;

	hmdPosition += hmdForwardOffset * poseAngles.ToMat3()[0];

	if( resetTrackingOffset == true )
	{
		trackingOriginOffset = lastHmdPosition;
		trackingOriginHeight = trackingOriginOffset.z;

		if( vr_useFloorHeight.GetInteger() == 0 )
		{
			trackingOriginOffset.z += pm_normalviewheight.GetFloat() + 5 + CM_CLIP_EPSILON - vr_normalViewHeight.GetFloat() / vr_scale.GetFloat();
		}
		else if( vr_useFloorHeight.GetInteger() == 2 )
		{
			trackingOriginOffset.z += 5;
		}
		else if( vr_useFloorHeight.GetInteger() == 3 )
		{
			trackingOriginOffset.z = pm_normalviewheight.GetFloat() + 5 + CM_CLIP_EPSILON;
		}
		else if( vr_useFloorHeight.GetInteger() == 4 )
		{
			float oldScale = vr_scale.GetFloat();
			float h = trackingOriginHeight * oldScale;
			float newScale = h / ( pm_normalviewheight.GetFloat() + 5 + CM_CLIP_EPSILON );
			trackingOriginHeight *= oldScale / newScale;
			trackingOriginOffset *= oldScale / newScale;
			vr_scale.SetFloat( newScale );
		}

		common->Printf( "Resetting tracking yaw offset.\n Yaw = %f old offset = %f ", hmdAngles.yaw, trackingOriginYawOffset );
		trackingOriginYawOffset = hmdAngles.yaw;
		common->Printf( "New Tracking yaw offset %f\n", hmdAngles.yaw, trackingOriginYawOffset );
		neckInitialized = false;
		cinematicStartViewYaw = trackingOriginYawOffset;

		return;
	}

	hmdPosition -= trackingOriginOffset;

	hmdPosition *= idAngles( 0.0f, -trackingOriginYawOffset, 0.0f ).ToMat3();

	absolutePosition = hmdPosition;

	hmdAngles.yaw -= trackingOriginYawOffset;
	hmdAngles.Normalize360();

	//	common->Printf( "Hmdangles yaw = %f pitch = %f roll = %f\n", poseAngles.yaw, poseAngles.pitch, poseAngles.roll );
	//	common->Printf( "Trans x = %f y = %f z = %f\n", hmdPosition.x, hmdPosition.y, hmdPosition.z );

	lastRoll = hmdAngles.roll;
	lastPitch = hmdAngles.pitch;
	lastYaw = hmdAngles.yaw;
	lastAbsolutePosition = absolutePosition;
	hmdPositionTracked = true;

	hmdBodyTranslation = absolutePosition;

	idAngles hmd2 = hmdAngles;
	hmd2.yaw -= bodyYawOffset;

	//hmdAxis = hmd2.ToMat3();
	hmdAxis = hmdAngles.ToMat3();

	currentNeckPosition = hmdPosition + hmdAxis[0] * vr_nodalX.GetFloat() / vr_scale.GetFloat() /*+ hmdAxis[1] * 0.0f */ + hmdAxis[2] * vr_nodalZ.GetFloat() / vr_scale.GetFloat();

	if( !neckInitialized )
	{
		lastNeckPosition = currentNeckPosition;
		initialNeckPosition = currentNeckPosition;
		if( vr_useFloorHeight.GetInteger() != 1 )
		{
			initialNeckPosition.z = vr_nodalZ.GetFloat() / vr_scale.GetFloat();
		}
		neckInitialized = true;
	}

	bodyPositionDelta = currentNeckPosition - lastNeckPosition; // use this to base movement on neck model
	bodyPositionDelta.z = currentNeckPosition.z - initialNeckPosition.z;

	//bodyPositionDelta = currentChestPosition - lastChestPosition;
	lastBodyPositionDelta = bodyPositionDelta;

	lastNeckPosition = currentNeckPosition;

	headPositionDelta = hmdPosition - currentNeckPosition; // use this to base movement on neck model
	//headPositionDelta = hmdPosition - currentChestPosition;
	headPositionDelta.z = hmdPosition.z;
	//bodyPositionDelta.z = 0;

	// how many game units the user has physically ducked in real life from their calibrated position
	userDuckingAmount = ( trackingOriginHeight - trackingOriginOffset.z ) - hmdPosition.z;

	lastBodyPositionDelta = bodyPositionDelta;
	lastHeadPositionDelta = headPositionDelta;
}

/*
==============
iVr::HMDResetTrackingOriginOffset
==============
*/
void iVr::HMDResetTrackingOriginOffset()
{
	static idVec3 body = vec3_zero;
	static idVec3 head = vec3_zero;
	static idVec3 absPos = vec3_zero;
	static idAngles rot = ang_zero;

	common->Printf( "HMDResetTrackingOriginOffset called\n " );

	HMDGetOrientation( rot, head, body, absPos, true );

	common->Printf( "New Yaw offset = %f\n", trackingOriginYawOffset );
}

/*
==============
iVr::MotionControlGetOpenVrController
==============
*/
void iVr::MotionControlGetOpenVrController( vr::TrackedDeviceIndex_t deviceNum, idVec3& motionPosition, idQuat& motionRotation )
{
	idMat4 m_rmat4DevicePose = ConvertSteamVRMatrixToidMat4( m_rTrackedDevicePose[( int )deviceNum].mDeviceToAbsoluteTracking );
	static idQuat orientationPose;
	static idQuat poseRot;
	static idAngles poseAngles = ang_zero;
	static idAngles angTemp = ang_zero;

	motionPosition.x = -m_rmat4DevicePose[3][2] * ( 100.0f / 2.54f ) / vr_scale.GetFloat();
	motionPosition.y = -m_rmat4DevicePose[3][0] * ( 100.0f / 2.54f ) / vr_scale.GetFloat(); // meters to inches
	motionPosition.z = m_rmat4DevicePose[3][1] * ( 100.0f / 2.54f ) / vr_scale.GetFloat();

	motionPosition -= trackingOriginOffset;
	motionPosition *= idAngles( 0.0f, ( -trackingOriginYawOffset ) , 0.0f ).ToMat3(); // .Inverse();

	orientationPose = m_rmat4DevicePose.ToMat3().ToQuat();

	poseRot.x = orientationPose.z;
	poseRot.y = orientationPose.x;
	poseRot.z = -orientationPose.y;
	poseRot.w = orientationPose.w;

	poseAngles = poseRot.ToAngles();

	angTemp.yaw = poseAngles.yaw;
	angTemp.roll = poseAngles.roll;
	angTemp.pitch = poseAngles.pitch;

	motionPosition -= hmdBodyTranslation;

	angTemp.yaw -= trackingOriginYawOffset;// + bodyYawOffset;
	angTemp.Normalize360();

	motionRotation = angTemp.ToQuat();
}

/*
==============
iVr::MotionControllGetHand;
==============
*/
void iVr::MotionControlGetHand( int hand, idVec3& motionPosition, idQuat& motionRotation )
{
	if( hand == HAND_LEFT )
	{
		MotionControlGetLeftHand( motionPosition, motionRotation );
	}
	else
	{
		MotionControlGetRightHand( motionPosition, motionRotation );
	}

	// apply weapon mount offsets

	if( hand == vr_weaponHand.GetInteger() && vr_mountedWeaponController.GetBool() )
	{
		idVec3 controlToHand = idVec3( vr_mountx.GetFloat() / vr_scale.GetFloat(), vr_mounty.GetFloat() / vr_scale.GetFloat(), vr_mountz.GetFloat()  / vr_scale.GetFloat() );
		idVec3 controlCenter = idVec3( vr_controllerOffsetX.GetFloat() / vr_scale.GetFloat(), vr_controllerOffsetY.GetFloat() / vr_scale.GetFloat(), vr_controllerOffsetZ.GetFloat()  / vr_scale.GetFloat() );

		motionPosition += ( controlToHand - controlCenter ) * motionRotation; // pivot around the new point
	}
	else
	{
		motionPosition += idVec3( vr_controllerOffsetX.GetFloat()  / vr_scale.GetFloat(), vr_controllerOffsetY.GetFloat() / vr_scale.GetFloat(), vr_controllerOffsetZ.GetFloat() / vr_scale.GetFloat() ) * motionRotation;
	}
}


/*
==============
iVr::MotionControllGetLeftHand;
==============
*/
void iVr::MotionControlGetLeftHand( idVec3& motionPosition, idQuat& motionRotation )
{
	switch( motionControlType )
	{
		case MOTION_STEAMVR:
		{
			//vr::TrackedDeviceIndex_t deviceNo = vr::VRSystem()->GetTrackedDeviceIndexForControllerRole( vr::TrackedControllerRole_LeftHand );
			//MotionControlGetOpenVrController( deviceNo, motionPosition, motionRotation );
			MotionControlGetOpenVrController( leftControllerDeviceNo, motionPosition, motionRotation );

			//motionPosition += idVec3( vr_controllerOffsetX.GetFloat(), vr_controllerOffsetY.GetFloat(), vr_controllerOffsetZ.GetFloat() ) * motionRotation;

			break;
		}

		default:
			break;
	}
}

/*
==============
iVr::MotionControllGetRightHand;
==============
*/
void iVr::MotionControlGetRightHand( idVec3& motionPosition, idQuat& motionRotation )
{
	switch( motionControlType )
	{
		case MOTION_STEAMVR:
		{
			//vr::TrackedDeviceIndex_t deviceNo = vr::VRSystem()->GetTrackedDeviceIndexForControllerRole( vr::TrackedControllerRole_RightHand );
			//MotionControlGetOpenVrController( deviceNo, motionPosition, motionRotation );
			MotionControlGetOpenVrController( rightControllerDeviceNo, motionPosition, motionRotation );

			//motionPosition += idVec3( vr_controllerOffsetX.GetFloat(), vr_controllerOffsetY.GetFloat(), vr_controllerOffsetZ.GetFloat() ) * motionRotation;
			break;
		}

		default:
			break;
	}
}

/*
==============
iVr::MotionControllSetHaptic
==============
*/
void iVr::MotionControllerSetHapticOpenVR( int hand, unsigned short value )
{
	vr::TrackedDeviceIndex_t deviceNo;

	if( hand == HAND_RIGHT )
	{
		deviceNo = vr::VRSystem()->GetTrackedDeviceIndexForControllerRole( vr::TrackedControllerRole_RightHand );
	}
	else
	{
		deviceNo = vr::VRSystem()->GetTrackedDeviceIndexForControllerRole( vr::TrackedControllerRole_LeftHand );
	}

	m_pHMD->TriggerHapticPulse( deviceNo, 0, value );
}

/*
==============
iVr::CalcAimMove
Pass the controller yaw & pitch changes.
Indepent weapon view angles will be updated,
and the correct yaw & pitch movement values will
be returned based on the current user aim mode.
==============
*/
void iVr::CalcAimMove( float& yawDelta, float& pitchDelta )
{
	if( VR_USE_MOTION_CONTROLS )
	{
		// no independent aim or joystick pitch when using motion controllers.
		pitchDelta = 0.0f;
		return;
	}

	float pitchDeadzone = vr_deadzonePitch.GetFloat();
	float yawDeadzone = vr_deadzoneYaw.GetFloat();

	independentWeaponPitch += pitchDelta;

	if( independentWeaponPitch >= pitchDeadzone )
	{
		independentWeaponPitch = pitchDeadzone;
	}
	if( independentWeaponPitch < -pitchDeadzone )
	{
		independentWeaponPitch = -pitchDeadzone;
	}
	pitchDelta = 0;

	independentWeaponYaw += yawDelta;

	if( independentWeaponYaw >= yawDeadzone )
	{
		yawDelta = independentWeaponYaw - yawDeadzone;
		independentWeaponYaw = yawDeadzone;
		return;
	}

	if( independentWeaponYaw < -yawDeadzone )
	{
		yawDelta = independentWeaponYaw + yawDeadzone;
		independentWeaponYaw = -yawDeadzone;
		return;
	}

	yawDelta = 0.0f;
}



/*
==============
iVr::StartFrame
==============
*/
void iVr::StartFrame()
{
	//common->Printf( "Framestart called from frame %d\n", idLib::frameNumber );

	static int lastFrame = -1;

	if( idLib::frameNumber == lastFrame && !renderingSplash )
	{
		return;
	}
	lastFrame = idLib::frameNumber;

	lastBodyYawOffset = bodyYawOffset;
	poseLastHmdAngles = poseHmdAngles;
	poseLastHmdHeadPositionDelta = poseHmdHeadPositionDelta;
	poseLastHmdBodyPositionDelta = poseHmdBodyPositionDelta;
	poseLastHmdAbsolutePosition = poseHmdAbsolutePosition;

	vr::VRCompositor()->WaitGetPoses( m_rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0 );

	leftControllerDeviceNo = vr::VRSystem()->GetTrackedDeviceIndexForControllerRole( vr::TrackedControllerRole_LeftHand );
	rightControllerDeviceNo = vr::VRSystem()->GetTrackedDeviceIndexForControllerRole( vr::TrackedControllerRole_RightHand );

	/*
	vr::VRControllerState_t& currentStateL = pControllerStateL;
	m_pHMD->GetControllerState( leftControllerDeviceNo, &currentStateL );

	vr::VRControllerState_t& currentStateR = pControllerStateR;
	m_pHMD->GetControllerState( rightControllerDeviceNo, &currentStateR );
	*/
	//Sys_PollJoystickInputEvents( 5 ); // 5 is the device num for the steam vr controller, pull once per frame for now :(


	HMDGetOrientation( poseHmdAngles, poseHmdHeadPositionDelta, poseHmdBodyPositionDelta, poseHmdAbsolutePosition, false );
	remainingMoveHmdBodyPositionDelta = poseHmdBodyPositionDelta;

	for( int i = 0; i < 2; i++ )
	{
		MotionControlGetHand( i, poseHandPos[i], poseHandRotationQuat[i] );
		poseHandRotationMat3[i] = poseHandRotationQuat[i].ToMat3();
		poseHandRotationAngles[i] = poseHandRotationQuat[i].ToAngles();
	}
}

/*
==============
iVr::GetCurrentFlashMode();
==============
*/

int iVr::GetCurrentFlashMode()
{
	//common->Printf( "Returning flashmode %d\n", currentFlashMode );
	return currentFlashMode;
}

/*
==============
iVr::GetCurrentFlashMode();
==============
*/
void iVr::NextFlashMode()
{
	currentFlashMode++;
	if( currentFlashMode >= FLASHLIGHT_MAX )
	{
		currentFlashMode = 0;
	}
}

void iVr::ForceChaperone( int which, bool force )
{
	static bool chaperones[2] = {};
	chaperones[which] = force;
	force = chaperones[0] || chaperones[1];

	if( m_pHMD )
	{
		vr::VRChaperone()->ForceBoundsVisible( force );
	}
}

void iVr::SwapBinding( int Old, int New )
{
	idStr s = idKeyInput::GetBinding( New );
	idKeyInput::SetBinding( New, idKeyInput::GetBinding( Old ) );
	idKeyInput::SetBinding( Old, s.c_str() );
}

void iVr::SwapWeaponHand()
{
	vr_weaponHand.SetInteger( 1 - vr_weaponHand.GetInteger() );

	// swap teleport hand
	if( vr_teleport.GetInteger() == 2 )
	{
		vr_teleport.SetInteger( 3 );
	}
	else if( vr_teleport.GetInteger() == 3 )
	{
		vr_teleport.SetInteger( 2 );
	}

	// swap motion controller bindings to other hand
	for( int k = K_JOY17; k <= K_JOY18; k++ )
	{
		SwapBinding( k, k + 7 );
	}

	// JOY19 is the Touch menu button, which only exists on the left hand
	for( int k = K_JOY20; k <= K_JOY23; k++ )
	{
		SwapBinding( k, k + 7 );
	}
	for( int k = K_JOY31; k <= K_JOY48; k++ )
	{
		SwapBinding( k, k + 18 );
	}

	SwapBinding( K_L_TOUCHTRIG, K_R_TOUCHTRIG );
	SwapBinding( K_L_STEAMVRTRIG, K_R_STEAMVRTRIG );

	if( vr_handSwapsAnalogs.GetBool() )
	{
		for( int k = K_TOUCH_LEFT_STICK_UP; k <= K_TOUCH_LEFT_STICK_RIGHT; k++ )
		{
			SwapBinding( k, k + 4 );
		}
		for( int k = K_STEAMVR_LEFT_PAD_UP; k <= K_STEAMVR_LEFT_PAD_RIGHT; k++ )
		{
			SwapBinding( k, k + 4 );
		}
		for( int k = K_STEAMVR_LEFT_JS_UP; k <= K_STEAMVR_LEFT_JS_RIGHT; k++ )
		{
			SwapBinding( k, k + 4 );
		}
	}
}