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

#include "precompiled.h"
#pragma hdrstop

#include "vr_hmd.h"
#include "Voice.h"
#include "../renderer/Framebuffer.h"

#include "../libs/openvr/headers/openvr.h"


#ifndef __VR_H__
#define __VR_H__

// hand == 0 == right, 1 == left
#define HAND_RIGHT 0
#define HAND_LEFT 1

#define POSE_FINGER 8
#define POSE_INDEX 1
#define POSE_THUMB 2
#define POSE_GRIP 4

typedef enum
{
	MOTION_NONE,
	MOTION_STEAMVR,
} vr_motionControl_t;

typedef enum
{
	VR_AA_NONE,
	VR_AA_MSAA,
	VR_AA_FXAA,
	NUM_VR_AA
} vr_aa_t;

typedef enum
{
	VR_HUD_NONE,
	VR_HUD_FULL,
	VR_HUD_LOOK_DOWN,
} vr_hud_t;

typedef enum
{
	RENDERING_NORMAL,
	RENDERING_PDA,
	RENDERING_HUD
} vr_swf_render_t;

typedef enum
{
	FLASHLIGHT_BODY,
	FLASHLIGHT_HEAD,
	FLASHLIGHT_GUN,
	FLASHLIGHT_HAND,
	FLASHLIGHT_MAX,
} vr_flashlight_mode_t;

// 0 = None, 1 = Chaperone, 2 = Reduce FOV, 3 = Black Screen, 4 = Black & Chaperone, 5 = Reduce FOV & Chaperone, 6 = Slow Mo, 7 = Slow Mo & Chaperone, 8 = Slow Mo & Reduce FOV, 9 = Slow Mo, Chaperone, Reduce FOV
enum vr_motionSicknessMode_t
{
	MOSICK_NONE,
	MOSICK_CHAPERONE,
	MOSICK_REDUCE_FOV,
	MOSICK_BLACK_SCREEN,
	MOSICK_BLACK_N_CHAPERONE,
	MOSICK_REDUCE_FOV_N_CHAPERONE,
	MOSICK_SLOWMO,
	MOSICK_SLOMO_N_CHAPERONE,
	MOSICK_SLOMO_N_REDUCE_FOV,
	MOSICK_SLOMO_N_REDUCE_FOV_N_CHAPERONE,
};

class idClipModel;

class iVr
{
public:
	iVr();

	bool				OpenVRInit();

	void				HMDInit();
	void				HMDShutdown();

	void				HMDInitializeDistortion();
	void				HMDGetOrientation( idAngles& hmdAngles, idVec3& headPositionDelta, idVec3& bodyPositionDelta, idVec3& absolutePosition, bool resetTrackingOffset );
	void				HMDRender( idImage* leftCurrent, idImage* rightCurrent );
	void				HMDTrackStatic( bool is3D );
	void				HUDRender( idImage* image0, idImage* image1 );
	void				HMDResetTrackingOriginOffset();

	void				StartFrame();

	void				MotionControlGetHand( int hand, idVec3& position, idQuat& rotation );
	void				MotionControlGetLeftHand( idVec3& position, idQuat& rotation );
	void				MotionControlGetRightHand( idVec3& position, idQuat& rotation );
	void				MotionControlGetOpenVrController( vr::TrackedDeviceIndex_t deviceNum, idVec3& position, idQuat& rotation );
	void				MotionControllerSetHapticOpenVR( int hand, unsigned short value );

	idVec2i				GetEyeResolution() const;

	// returns IPD in centimeters
	float				GetIPD() const;
	void				CalcAimMove( float& yawDelta, float& pitchDelta );

	int					GetCurrentFlashMode();
	void				NextFlashMode();

	void				ForceChaperone( int which, bool force );

	void				SwapWeaponHand();

	bool				IsActive() const
	{
		return isActive;
	}

	bool				HasHMD() const
	{
		return ( m_pHMD != nullptr );
	}

	int					GetHz() const
	{
		return hmdHz;
	}

	float				GetHMDFovX() const
	{
		return hmdFovX;
	}

	float				GetHMDFovY() const
	{
		return hmdFovY;
	}

	float				GetHMDAspect() const
	{
		return hmdAspect;
	}

	float				GetScreenSeparation() const
	{
		return screenSeparation;
	}

	float				GetUserDuckingAmount() const
	{
		return userDuckingAmount;
	}

	// input
	vr_motionControl_t	motionControlType;
	int					VR_USE_MOTION_CONTROLS;

	// rendering
	hmdEye_t			hmdEye[2];
	float				singleEyeIPD;

	float				bodyYawOffset;
	float				lastHMDYaw;
	float				lastHMDPitch;
	float				lastHMDRoll;
	idVec3				lastHMDViewOrigin;
	idMat3				lastHMDViewAxis;
	idVec3				uncrouchedHMDViewOrigin;
	float				headHeightDiff;

	// player movement
	bool				isLeaning;
	idVec3				leanOffset;
	idVec3				leanBlankOffset;
	float				leanBlankOffsetLengthSqr;
	bool				leanBlank;
	bool				forceRun;

	float				cinematicStartViewYaw;
	idVec3				cinematicStartPosition;

	idVec3				trackingOriginOffset;
	float				trackingOriginYawOffset;

	// teleporting
	bool				didTeleport;
	float				teleportDir;
	int					teleportButtonCount;
	idVec2				leftMapped;
	int					oldTeleportButtonState;

	// for 2D
	idAngles			poseHmdAngles;
	idVec3				poseHmdAbsolutePosition;

	idVec3				poseHmdHeadPositionDelta;
	idVec3				poseHmdBodyPositionDelta;

	idVec3				poseHandPos[2];
	idQuat				poseHandRotationQuat[2];
	idMat3				poseHandRotationMat3[2];
	idAngles			poseHandRotationAngles[2];

	idAngles			poseLastHmdAngles;
	idVec3				poseLastHmdHeadPositionDelta;
	idVec3				poseLastHmdBodyPositionDelta;
	idVec3				poseLastHmdAbsolutePosition;
	float				lastBodyYawOffset;

	// weapon aiming and interaction
	float				independentWeaponYaw;
	float				independentWeaponPitch;

	// TODO remove
	bool				renderingSplash;
	bool				showingIntroVideo;

	uint32_t			hmdWidth;
	uint32_t			hmdHeight;
	int					hmdHz;

	bool				useFBO;
	int					primaryFBOWidth;
	int					primaryFBOHeight;
	//---------------------------
	// client VR code shared with the game code

	// TODO delete as many vars as possible and separate blocks in vars that are accessed by the game thread and main thread

	int					currentFlashMode;

	bool				VR_GAME_PAUSED;

	bool				pdaForceToggle;
	bool				pdaForced;
	bool				pdaRising;
	bool				pdaForceLeftStick;				// navigate through PDA menus only with the left controller and motions
	bool				pdaScanning;

	bool				gameSavingLoading;


	int					swfRenderMode;
	int					pdaToggleTime;
	int					lastSaveTime;
	bool				wasSaved;
	bool				wasLoaded;

	int					currentFlashlightPosition;

	bool				handInGui;

	bool				shouldRecenter;

	int					vrFrameNumber;
	int					lastPostFrame;

	int					frameCount;

	int					fingerPose[2];

	idVec3				lastViewOrigin;
	idMat3				lastViewAxis;

	idVec3				lastCenterEyeOrigin;
	idMat3				lastCenterEyeAxis;

	float				handRoll[2];


	int					VR_AAmode;

	float				angles[3];

	vr::VRControllerState_t pControllerStateL;
	vr::VRControllerState_t pControllerStateR;

	vr::TrackedDeviceIndex_t leftControllerDeviceNo;
	vr::TrackedDeviceIndex_t rightControllerDeviceNo;

	float				hmdForwardOffset;

	float				manualHeight;

	idImage*			hmdEyeImage[2];			// TODO REMOVE duplicate of stereoImages in render backend
	idImage*			hmdCurrentRender[2];	// these point to idRenderBackend::stereoRenderImages

	float				trackingOriginHeight;
	bool				chestDefaultDefined;
	idVec3				hmdBodyTranslation;

	idVec3				motionMoveDelta;
	idVec3				motionMoveVelocity;

	idVec3				fixedPDAMoveDelta;

	bool				playerDead;

	bool				isLoading;

	int					lastRead;
	int					currentRead;
	bool				updateScreen;

	float				bodyMoveAng;

	bool				privateCamera;



	// wip stuff
	int					wipNumSteps;
	int					wipStepState;
	int					wipLastPeriod;
	float				wipCurrentDelta;
	float				wipCurrentVelocity;
	float				wipPeriodVel;
	float				wipTotalDelta;
	float				wipLastAcces;
	float				wipAvgPeriod;
	float				wipTotalDeltaAvg;

	idVec3				remainingMoveHmdBodyPositionDelta;
	idStr				currentBindingDisplay;
	idVec3				currentHandWorldPosition[2];

	// clip stuff
	idClipModel*		bodyClip;
	idClipModel*		headClip;


private:
	idMat4				GetHMDMatrixProjectionEye( vr::Hmd_Eye nEye );
	idMat4				GetHMDMatrixPoseEye( vr::Hmd_Eye nEye );

	void				SwapBinding( int Old, int New );

	bool				isActive;

	vr::IVRSystem*			m_pHMD = nullptr;
	vr::IVRCompositor*		m_pCompositor;
	vr::IVRChaperone*		m_pChaperone;
	vr::IVRRenderModels*		m_pRenderModels;
	vr::TrackedDevicePose_t	m_rTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
	vr::TrackedDevicePose_t	m1_rTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];

	idStr				m_strDriver;
	idStr				m_strDisplay;

	char				m_rDevClassChar[vr::k_unMaxTrackedDeviceCount];
	idMat4				m_rmat4DevicePose[vr::k_unMaxTrackedDeviceCount];
	bool				m_rbShowTrackedDevice[vr::k_unMaxTrackedDeviceCount];


	idMat4				m_mat4ProjectionLeft;
	idMat4				m_mat4ProjectionRight;
	idMat4				m_mat4eyePosLeft;
	idMat4				m_mat4eyePosRight;

	bool				hmdPositionTracked;

	float				hmdFovX;
	float				hmdFovY;
	float				hmdPixelScale;
	float				hmdAspect;

	float				screenSeparation;		// for Reduce FOV motion sickness fix
	float				userDuckingAmount;		// how many game units the user has physically ducked in real life from their calibrated position
};

extern idCVar vr_scale;
extern idCVar vr_normalViewHeight;
extern idCVar vr_manualHeight;
extern idCVar vr_useFloorHeight;

extern idCVar vr_wristStatMon;
extern idCVar vr_disableWeaponAnimation;
extern idCVar vr_headKick;

extern idCVar vr_weaponHand;

extern idCVar vr_flashlightMode;

extern idCVar vr_flashlightBodyPosX;
extern idCVar vr_flashlightBodyPosY;
extern idCVar vr_flashlightBodyPosZ;

extern idCVar vr_flashlightHelmetPosX;
extern idCVar vr_flashlightHelmetPosY;
extern idCVar vr_flashlightHelmetPosZ;

extern idCVar vr_forward_keyhole;

extern idCVar vr_PDAfixLocation;

extern idCVar vr_weaponPivotOffsetForward;
extern idCVar vr_weaponPivotOffsetHorizontal;
extern idCVar vr_weaponPivotOffsetVertical;
extern idCVar vr_weaponPivotForearmLength;


extern idCVar vr_guiScale;
extern idCVar vr_guiSeparation;

extern idCVar vr_guiMode;

extern idCVar vr_hudScale;
extern idCVar vr_hudPosHor;
extern idCVar vr_hudPosVer;
extern idCVar vr_hudPosDis;
extern idCVar vr_hudPosLock;
extern idCVar vr_hudType;
extern idCVar vr_hudPosAngle;
extern idCVar vr_hudRevealAngle;
extern idCVar vr_hudTransparency;
extern idCVar vr_hudOcclusion;

extern idCVar vr_hudHealth;
extern idCVar vr_hudAmmo;
extern idCVar vr_hudPickUps;
extern idCVar vr_hudTips;
extern idCVar vr_hudLocation;
extern idCVar vr_hudObjective;
extern idCVar vr_hudStamina;
extern idCVar vr_hudPills;
extern idCVar vr_hudComs;
extern idCVar vr_hudWeap;
extern idCVar vr_hudNewItems;
extern idCVar vr_hudFlashlight;
extern idCVar vr_hudLowHealth;

extern idCVar vr_tweakTalkCursor;
extern idCVar vr_talkWakeMonsters;
extern idCVar vr_talkWakeMonsterRadius;
extern idCVar vr_talkMode;
extern idCVar vr_voiceCommands;
extern idCVar vr_voicePushToTalk;
extern idCVar vr_voiceRepeat;
extern idCVar vr_voiceMinVolume;

extern idCVar vr_listMonitorName;

extern idCVar vr_enable;
extern idCVar vr_joystickMenuMapping;

extern idCVar vr_trackingPredictionUserDefined;

extern idCVar vr_minLoadScreenTime;

extern idCVar vr_deadzonePitch;
extern idCVar vr_deadzoneYaw;
extern idCVar vr_comfortDelta;
extern idCVar vr_comfortJetStrafeDelta;

extern idCVar vr_headingBeamMode;

extern idCVar vr_weaponSight;
extern idCVar vr_weaponSightToSurface;

extern idCVar vr_motionFlashPitchAdj;
extern idCVar vr_motionWeaponPitchAdj;

extern idCVar vr_3dgui;
extern idCVar vr_shakeAmplitude;
extern idCVar vr_controllerGamepad;

extern idCVar vr_offHandPosX;
extern idCVar vr_offHandPosY;
extern idCVar vr_offHandPosZ;

extern idCVar vr_padDeadzone;
extern idCVar vr_jsDeadzone;
extern idCVar vr_padToButtonThreshold;
extern idCVar vr_knockBack;
extern idCVar vr_jumpBounce;
extern idCVar vr_stepSmooth;

extern idCVar vr_mountedWeaponController;
extern idCVar vr_walkSpeedAdjust;

extern idCVar vr_crouchTriggerDist;
extern idCVar vr_crouchMode;

extern idCVar vr_wipPeriodMin;
extern idCVar vr_wipPeriodMax;

extern idCVar vr_wipVelocityMin;
extern idCVar vr_wipVelocityMax;

extern idCVar vr_headbbox;

extern idCVar vr_pdaPosX;
extern idCVar vr_pdaPosY;
extern idCVar vr_pdaPosZ;

extern idCVar vr_pdaPitch;

extern idCVar vr_movePoint;
extern idCVar vr_moveClick;
extern idCVar vr_playerBodyMode;

extern idCVar vr_stereoMirror;

extern idCVar vr_teleportSkipHandrails;
extern idCVar vr_teleportShowAimAssist;
extern idCVar vr_teleportButtonMode;
extern idCVar vr_teleportHint;

extern idCVar vr_teleport;
extern idCVar vr_teleportMode;
extern idCVar vr_teleportMaxTravel;
extern idCVar vr_teleportThroughDoors;
extern idCVar vr_motionSickness;
extern idCVar vr_strobeTime;
extern idCVar vr_chaperone;
extern idCVar vr_chaperoneColor;

extern idCVar vr_slotDebug;
extern idCVar vr_slotMag;
extern idCVar vr_slotDur;
extern idCVar vr_slotDisable;

extern idCVar vr_handSwapsAnalogs;
extern idCVar vr_autoSwitchControllers;

extern idCVar vr_useHandPoses;

extern idCVar vr_instantAccel;
extern idCVar vr_shotgunChoke;

extern idCVar vr_headshotMultiplier;

extern iVr* vrSystem;
extern iVoice* vrVoice;

#endif
