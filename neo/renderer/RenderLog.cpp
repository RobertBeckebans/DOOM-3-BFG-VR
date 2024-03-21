/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2020 Robert Beckebans

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
#include "RenderCommon.h"
#pragma hdrstop

/*
================================================================================================
Contains the RenderLog implementation.

TODO:	Emit statistics to the logfile at the end of views and frames.
================================================================================================
*/

idCVar r_logLevel( "r_logLevel", "0", CVAR_INTEGER, "1 = blocks only, 2 = everything", 1, 2 );

static const int LOG_LEVEL_BLOCKS_ONLY	= 1;
static const int LOG_LEVEL_EVERYTHING	= 2;

const char* renderLogMainBlockLabels[] =
{
	ASSERT_ENUM_STRING( MRB_GPU_TIME,						0 ),
	ASSERT_ENUM_STRING( MRB_BEGIN_DRAWING_VIEW,				1 ),
	ASSERT_ENUM_STRING( MRB_FILL_DEPTH_BUFFER,				2 ),
	ASSERT_ENUM_STRING( MRB_FILL_GEOMETRY_BUFFER,			3 ), // RB
	ASSERT_ENUM_STRING( MRB_SSAO_PASS,						4 ), // RB
	ASSERT_ENUM_STRING( MRB_AMBIENT_PASS,					5 ), // RB
	ASSERT_ENUM_STRING( MRB_DRAW_INTERACTIONS,				6 ),
	ASSERT_ENUM_STRING( MRB_DRAW_SHADER_PASSES,				7 ),
	ASSERT_ENUM_STRING( MRB_FOG_ALL_LIGHTS,					8 ),
	ASSERT_ENUM_STRING( MRB_BLOOM,							9 ),
	ASSERT_ENUM_STRING( MRB_DRAW_SHADER_PASSES_POST,		10 ),
	ASSERT_ENUM_STRING( MRB_DRAW_DEBUG_TOOLS,				11 ),
	ASSERT_ENUM_STRING( MRB_CAPTURE_COLORBUFFER,			12 ),
	ASSERT_ENUM_STRING( MRB_POSTPROCESS,					13 ),
	ASSERT_ENUM_STRING( MRB_DRAW_GUI,                       14 ),
	ASSERT_ENUM_STRING( MRB_TOTAL,							15 )
};

extern uint64 Sys_Microseconds();
/*
================================================================================================

PIX events on all platforms

================================================================================================
*/


/*
================================================
pixEvent_t
================================================
*/
struct pixEvent_t
{
	char		name[256];
	uint64		cpuTime;
	uint64		gpuTime;
};

idCVar r_pix( "r_pix", "0", CVAR_INTEGER, "print GPU/CPU event timing" );

#if !defined( USE_VULKAN )
	static const int	MAX_PIX_EVENTS = 256;
	// defer allocation of this until needed, so we don't waste lots of memory
	pixEvent_t* 		pixEvents;	// [MAX_PIX_EVENTS]
	int					numPixEvents;
	int					numPixLevels;
	static GLuint		timeQueryIds[MAX_PIX_EVENTS];
#endif

/*
========================
PC_BeginNamedEvent

FIXME: this is not thread safe on the PC
========================
*/
void PC_BeginNamedEvent( const char* szName, const idVec4& color )
{
	if( r_logLevel.GetInteger() <= 0 )
	{
		return;
	}

	// RB: colors are not supported in OpenGL

	// only do this if RBDOOM-3-BFG was started by RenderDoc or some similar tool
	if( glConfig.gremedyStringMarkerAvailable && glConfig.khronosDebugAvailable )
	{
		glPushDebugGroup( GL_DEBUG_SOURCE_APPLICATION_ARB, 0, GLsizei( strlen( szName ) ), szName );
	}

#if 0
	if( !r_pix.GetBool() )
	{
		return;
	}
	if( !pixEvents )
	{
		// lazy allocation to not waste memory
		pixEvents = ( pixEvent_t* )Mem_ClearedAlloc( sizeof( *pixEvents ) * MAX_PIX_EVENTS, TAG_CRAP );
	}
	if( numPixEvents >= MAX_PIX_EVENTS )
	{
		idLib::FatalError( "PC_BeginNamedEvent: event overflow" );
	}
	if( ++numPixLevels > 1 )
	{
		return;	// only get top level timing information
	}
	if( !glGetQueryObjectui64vEXT )
	{
		return;
	}

	GL_CheckErrors();
	if( timeQueryIds[0] == 0 )
	{
		glGenQueries( MAX_PIX_EVENTS, timeQueryIds );
	}
	glFinish();
	glBeginQuery( GL_TIME_ELAPSED_EXT, timeQueryIds[numPixEvents] );
	GL_CheckErrors();

	pixEvent_t* ev = &pixEvents[numPixEvents++];
	strncpy( ev->name, szName, sizeof( ev->name ) - 1 );
	ev->cpuTime = Sys_Microseconds();
#endif
}

/*
========================
PC_EndNamedEvent
========================
*/
void PC_EndNamedEvent()
{
	if( r_logLevel.GetInteger() <= 0 )
	{
		return;
	}

#if defined( USE_VULKAN )
	// SRS - Prefer VK_EXT_debug_utils over VK_EXT_debug_marker/VK_EXT_debug_report (deprecated by VK_EXT_debug_utils)
	if( vkcontext.debugUtilsSupportAvailable )
	{
		qvkCmdEndDebugUtilsLabelEXT( vkcontext.commandBuffer[ vkcontext.frameParity ] );
	}
	else if( vkcontext.debugMarkerSupportAvailable )
	{
		qvkCmdDebugMarkerEndEXT( vkcontext.commandBuffer[ vkcontext.frameParity ] );
	}
#else
	// only do this if RBDOOM-3-BFG was started by RenderDoc or some similar tool
	if( glConfig.gremedyStringMarkerAvailable && glConfig.khronosDebugAvailable )
	{
		glPopDebugGroup();
	}
#endif

#if 0
	if( !r_pix.GetBool() )
	{
		return;
	}
	if( numPixLevels <= 0 )
	{
		idLib::FatalError( "PC_EndNamedEvent: level underflow" );
	}
	if( --numPixLevels > 0 )
	{
		// only do timing on top level events
		return;
	}
	if( !glGetQueryObjectui64vEXT )
	{
		return;
	}

	pixEvent_t* ev = &pixEvents[numPixEvents - 1];
	ev->cpuTime = Sys_Microseconds() - ev->cpuTime;

	GL_CheckErrors();
	glEndQuery( GL_TIME_ELAPSED_EXT );
	GL_CheckErrors();
#endif
}

/*
========================
PC_EndFrame
========================
*/
void PC_EndFrame()
{
#if 0
	if( !r_pix.GetBool() )
	{
		return;
	}

	int64 totalGPU = 0;
	int64 totalCPU = 0;

	idLib::Printf( "----- GPU Events -----\n" );
	for( int i = 0 ; i < numPixEvents ; i++ )
	{
		pixEvent_t* ev = &pixEvents[i];

		int64 gpuTime = 0;
		glGetQueryObjectui64vEXT( timeQueryIds[i], GL_QUERY_RESULT, ( GLuint64EXT* )&gpuTime );
		ev->gpuTime = gpuTime;

		idLib::Printf( "%2d: %1.2f (GPU) %1.3f (CPU) = %s\n", i, ev->gpuTime / 1000000.0f, ev->cpuTime / 1000.0f, ev->name );
		totalGPU += ev->gpuTime;
		totalCPU += ev->cpuTime;
	}
	idLib::Printf( "%2d: %1.2f (GPU) %1.3f (CPU) = total\n", numPixEvents, totalGPU / 1000000.0f, totalCPU / 1000.0f );
	memset( pixEvents, 0, numPixLevels * sizeof( pixEvents[0] ) );

	numPixEvents = 0;
	numPixLevels = 0;
#endif
}


/*
================================================================================================

idRenderLog

================================================================================================
*/

idRenderLog	renderLog;



// RB begin
/*
========================
idRenderLog::idRenderLog
========================
*/
idRenderLog::idRenderLog()
{
}

#if 1

/*
========================
idRenderLog::OpenMainBlock
========================
*/
void idRenderLog::OpenMainBlock( renderLogMainBlock_t block )
{
	// SRS - Use glConfig.timerQueryAvailable flag to control timestamp capture for all platforms
	if( glConfig.timerQueryAvailable )
	{
		mainBlock = block;

		if( backEnd.glState.renderLogMainBlockTimeQueryIds[ backEnd.glState.frameParity ][ mainBlock * 2 ] == 0 )
		{
			glCreateQueries( GL_TIMESTAMP, 2, &backEnd.glState.renderLogMainBlockTimeQueryIds[ backEnd.glState.frameParity ][ mainBlock * 2 ] );
		}

		glQueryCounter( backEnd.glState.renderLogMainBlockTimeQueryIds[ backEnd.glState.frameParity ][ mainBlock * 2 + 0 ], GL_TIMESTAMP );
		backEnd.glState.renderLogMainBlockTimeQueryIssued[ backEnd.glState.frameParity ][ mainBlock * 2 + 0 ]++;
	}
}

/*
========================
idRenderLog::CloseMainBlock
========================
*/
void idRenderLog::CloseMainBlock()
{
	// SRS - Use glConfig.timerQueryAvailable flag to control timestamp capture for all platforms
	if( glConfig.timerQueryAvailable )
	{
		glQueryCounter( backEnd.glState.renderLogMainBlockTimeQueryIds[ backEnd.glState.frameParity ][ mainBlock * 2 + 1 ], GL_TIMESTAMP );
		backEnd.glState.renderLogMainBlockTimeQueryIssued[ backEnd.glState.frameParity ][ mainBlock * 2 + 1 ]++;
	}
}

#endif

/*
========================
idRenderLog::OpenBlock
========================
*/
void idRenderLog::OpenBlock( const char* label, const idVec4& color )
{
	PC_BeginNamedEvent( label, color );
}

/*
========================
idRenderLog::CloseBlock
========================
*/
void idRenderLog::CloseBlock()
{
	PC_EndNamedEvent();
}
// RB end