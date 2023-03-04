/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

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

#ifndef __VOICE_H__
#define __VOICE_H__


typedef struct vr_voiceAction_s
{
	idStr string;
	int	action;
} vr_voiceAction_t;

class iVoice
{
public:

	iVoice();

	void		VoiceInit( void );
	bool		InitVoiceDictionary( void );
	static void		ListVoiceCmds_f( const idCmdArgs& args );
	void		VoiceShutdown( void );
	void		Speed( int talkingSpeed );

	void		Say( VERIFY_FORMAT_STRING const char* fmt, ... );

	void		HearWord( const char* w, int confidence );
	void		HearWord( const wchar_t* w, int confidence );
#ifdef _WIN32
	void		Event( WPARAM wParam, LPARAM lParam );
#endif
	bool		GetTalkButton();
	bool		GetSayButton( int j );

	void		AddFlicksyncLine( const char* line );
	void		AddFlicksyncLine( const wchar_t* line );

	float currentVolume; // 0 to 1
	float maxVolume; // max volume since sound started: 0 to 1
	//---------------------------
private:

	idStr available;
	idStr npc;
	idStr cmds1;
	idStr cmds2;
	idStr startListen;
	idStr stopListen;

	void		AddWord( const char* word );
	void		AddWord( const wchar_t* word );
};



#endif
