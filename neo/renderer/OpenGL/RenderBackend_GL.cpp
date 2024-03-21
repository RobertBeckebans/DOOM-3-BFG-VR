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

// SRS - Include SDL headers to enable vsync changes without restart for UNIX-like OSs
#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)
	// SRS - Don't seem to need these #undefs (at least on macOS), are they needed for Linux, etc?
	// DG: SDL.h somehow needs the following functions, so #undef those silly
	//     "don't use" #defines from Str.h
	//#undef strncmp
	//#undef strcasecmp
	//#undef vsnprintf
	// DG end
	#include <SDL.h>
#endif
// SRS end

#if defined(_WIN32)
	// Vista OpenGL wrapper check
	#include "../sys/win32/win_local.h"
#endif

#include "../RenderCommon.h"
#include "../RenderBackend.h"
#include "../../framework/Common_local.h"
#include "d3xp/Game_local.h"


#include"vr/Vr.h" // Koz
#include"renderer/Framebuffer.h"


idCVar r_drawFlickerBox( "r_drawFlickerBox", "0", CVAR_RENDERER | CVAR_BOOL, "visual test for dropping frames" );

idCVar r_showSwapBuffers( "r_showSwapBuffers", "0", CVAR_BOOL, "Show timings from GL_BlockingSwapBuffers" );
idCVar r_syncEveryFrame( "r_syncEveryFrame", "1", CVAR_BOOL, "Don't let the GPU buffer execution past swapbuffers" );

static int		swapIndex;		// 0 or 1 into renderSync
static GLsync	renderSync[2];

void GLimp_SwapBuffers();
void RB_SetMVP( const idRenderMatrix& mvp );

/*
==================
GL_CheckErrors
==================
*/
// RB: added filename, line parms
bool GL_CheckErrors_( const char* filename, int line )
{
	int		err;
	char	s[64];
	int		i;

	if( r_ignoreGLErrors.GetBool() )
	{
		return false;
	}

	// check for up to 10 errors pending
	bool error = false;
	for( i = 0 ; i < 10 ; i++ )
	{
		err = glGetError();
		if( err == GL_NO_ERROR )
		{
			break;
		}

		error = true;
		switch( err )
		{
			case GL_INVALID_ENUM:
				strcpy( s, "GL_INVALID_ENUM" );
				break;
			case GL_INVALID_VALUE:
				strcpy( s, "GL_INVALID_VALUE" );
				break;
			case GL_INVALID_OPERATION:
				strcpy( s, "GL_INVALID_OPERATION" );
				break;
#if !defined(USE_GLES2) && !defined(USE_GLES3)
			case GL_STACK_OVERFLOW:
				strcpy( s, "GL_STACK_OVERFLOW" );
				break;
			case GL_STACK_UNDERFLOW:
				strcpy( s, "GL_STACK_UNDERFLOW" );
				break;
#endif
			case GL_OUT_OF_MEMORY:
				strcpy( s, "GL_OUT_OF_MEMORY" );
				break;
			default:
				idStr::snPrintf( s, sizeof( s ), "%i", err );
				break;
		}

		common->Printf( "caught OpenGL error: %s in file %s line %i\n", s, filename, line );
	}

	return error;
}






/*
========================
DebugCallback

For ARB_debug_output
========================
*/
// RB: added const to userParam
static void CALLBACK DebugCallback( unsigned int source, unsigned int type,
									unsigned int id, unsigned int severity, int length, const char* message, const void* userParam )
{
	// it probably isn't safe to do an idLib::Printf at this point

	// RB: printf should be thread safe on Linux
#if defined(_WIN32)
	OutputDebugString( message );
	OutputDebugString( "\n" );
#else
	printf( "%s\n", message );
#endif
	// RB end
}

/*
=================
R_CheckExtension
=================
*/
bool R_CheckExtension( char* name )
{
	if( !strstr( glConfig.extensions_string, name ) )
	{
		common->Printf( "X..%s not found\n", name );
		return false;
	}

	common->Printf( "...using %s\n", name );
	return true;
}

/*
==================
R_CheckPortableExtensions
==================
*/
// RB: replaced QGL with GLEW
static void R_CheckPortableExtensions()
{
	glConfig.glVersion = atof( glConfig.version_string );
	const char* badVideoCard = idLocalization::GetString( "#str_06780" );
	if( glConfig.glVersion < 2.0f )
	{
		idLib::FatalError( "%s", badVideoCard );
	}

	if( idStr::Icmpn( glConfig.renderer_string, "ATI ", 4 ) == 0 || idStr::Icmpn( glConfig.renderer_string, "AMD ", 4 ) == 0 )
	{
		glConfig.vendor = VENDOR_AMD;
	}
	else if( idStr::Icmpn( glConfig.renderer_string, "NVIDIA", 6 ) == 0 )
	{
		glConfig.vendor = VENDOR_NVIDIA;
	}
	else if( idStr::Icmpn( glConfig.renderer_string, "Intel", 5 ) == 0 )
	{
		glConfig.vendor = VENDOR_INTEL;
	}

	// RB: Mesa support
	if( idStr::Icmpn( glConfig.renderer_string, "Mesa", 4 ) == 0 || idStr::Icmpn( glConfig.renderer_string, "X.org", 5 ) == 0 || idStr::Icmpn( glConfig.renderer_string, "Gallium", 7 ) == 0 )
	{
		glConfig.driverType = GLDRV_OPENGL_MESA;
	}
	// RB end

	// GL_ARB_multitexture
	if( glConfig.driverType == GLDRV_OPENGL32_COMPATIBILITY_PROFILE || glConfig.driverType == GLDRV_OPENGL32_CORE_PROFILE || glConfig.driverType == GLDRV_OPENGL_MESA )
	{
		glConfig.multitextureAvailable = true;
	}
	else
	{
		glConfig.multitextureAvailable = GLEW_ARB_multitexture != 0;
	}

	// GL_EXT_direct_state_access
	glConfig.directStateAccess = GLEW_EXT_direct_state_access != 0;


	// GL_ARB_texture_compression + GL_S3_s3tc
	// DRI drivers may have GL_ARB_texture_compression but no GL_EXT_texture_compression_s3tc
	glConfig.textureCompressionAvailable = GLEW_ARB_texture_compression != 0 && GLEW_EXT_texture_compression_s3tc != 0;

	// GL_EXT_texture_filter_anisotropic
	glConfig.anisotropicFilterAvailable = GLEW_EXT_texture_filter_anisotropic != 0;
	if( glConfig.anisotropicFilterAvailable )
	{
		glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glConfig.maxTextureAnisotropy );
		common->Printf( "   maxTextureAnisotropy: %f\n", glConfig.maxTextureAnisotropy );
	}
	else
	{
		glConfig.maxTextureAnisotropy = 1;
	}

	// GL_EXT_texture_lod_bias
	// The actual extension is broken as specificed, storing the state in the texture unit instead
	// of the texture object.  The behavior in GL 1.4 is the behavior we use.
	glConfig.textureLODBiasAvailable = ( glConfig.glVersion >= 1.4 || GLEW_EXT_texture_lod_bias != 0 );
	if( glConfig.textureLODBiasAvailable )
	{
		common->Printf( "...using %s\n", "GL_EXT_texture_lod_bias" );
	}
	else
	{
		common->Printf( "X..%s not found\n", "GL_EXT_texture_lod_bias" );
	}

	// GL_ARB_seamless_cube_map
	glConfig.seamlessCubeMapAvailable = GLEW_ARB_seamless_cube_map != 0;
	r_useSeamlessCubeMap.SetModified();		// the CheckCvars() next frame will enable / disable it

	// GL_ARB_vertex_buffer_object
	glConfig.vertexBufferObjectAvailable = GLEW_ARB_vertex_buffer_object != 0;

	// GL_ARB_map_buffer_range, map a section of a buffer object's data store
	glConfig.mapBufferRangeAvailable = GLEW_ARB_map_buffer_range != 0;

	// GL_ARB_vertex_array_object
	glConfig.vertexArrayObjectAvailable = GLEW_ARB_vertex_array_object != 0;

	// GL_ARB_draw_elements_base_vertex
	glConfig.drawElementsBaseVertexAvailable = GLEW_ARB_draw_elements_base_vertex != 0;

	// GL_ARB_vertex_program / GL_ARB_fragment_program
	glConfig.fragmentProgramAvailable = GLEW_ARB_fragment_program != 0;
	//if( glConfig.fragmentProgramAvailable )
	{
		glGetIntegerv( GL_MAX_TEXTURE_COORDS, ( GLint* )&glConfig.maxTextureCoords );
		glGetIntegerv( GL_MAX_TEXTURE_IMAGE_UNITS, ( GLint* )&glConfig.maxTextureImageUnits );
	}

	// GLSL, core in OpenGL > 2.0
	glConfig.glslAvailable = ( glConfig.glVersion >= 2.0f );

	// GL_ARB_uniform_buffer_object
	glConfig.uniformBufferAvailable = GLEW_ARB_uniform_buffer_object != 0;
	if( glConfig.uniformBufferAvailable )
	{
		glGetIntegerv( GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, ( GLint* )&glConfig.uniformBufferOffsetAlignment );
		if( glConfig.uniformBufferOffsetAlignment < 256 )
		{
			glConfig.uniformBufferOffsetAlignment = 256;
		}
	}
	// RB: make GPU skinning optional for weak OpenGL drivers
	glConfig.gpuSkinningAvailable = glConfig.uniformBufferAvailable && ( glConfig.driverType == GLDRV_OPENGL3X || glConfig.driverType == GLDRV_OPENGL32_CORE_PROFILE || glConfig.driverType == GLDRV_OPENGL32_COMPATIBILITY_PROFILE );

	// ATI_separate_stencil / OpenGL 2.0 separate stencil
	glConfig.twoSidedStencilAvailable = ( glConfig.glVersion >= 2.0f ) || GLEW_ATI_separate_stencil != 0;

	// GL_EXT_depth_bounds_test
	glConfig.depthBoundsTestAvailable = GLEW_EXT_depth_bounds_test != 0;

	// GL_ARB_sync
	glConfig.syncAvailable = GLEW_ARB_sync &&
							 // as of 5/24/2012 (driver version 15.26.12.64.2761) sync objects
							 // do not appear to work for the Intel HD 4000 graphics
							 ( glConfig.vendor != VENDOR_INTEL || r_skipIntelWorkarounds.GetBool() );

	// GL_ARB_occlusion_query
	glConfig.occlusionQueryAvailable = GLEW_ARB_occlusion_query != 0;

	// GL_ARB_timer_query
	glConfig.timerQueryAvailable = ( GLEW_ARB_timer_query != 0 || GLEW_EXT_timer_query != 0 ) && ( glConfig.vendor != VENDOR_INTEL || r_skipIntelWorkarounds.GetBool() ) && glConfig.driverType != GLDRV_OPENGL_MESA;

	// GREMEDY_string_marker
	glConfig.gremedyStringMarkerAvailable = GLEW_GREMEDY_string_marker != 0;
	if( glConfig.gremedyStringMarkerAvailable )
	{
		common->Printf( "...using %s\n", "GL_GREMEDY_string_marker" );
	}
	else
	{
		common->Printf( "X..%s not found\n", "GL_GREMEDY_string_marker" );
	}

	// KHR_debug
	glConfig.khronosDebugAvailable = GLEW_KHR_debug != 0;
	if( glConfig.khronosDebugAvailable )
	{
		common->Printf( "...using %s\n", "GLEW_KHR_debug" );
	}
	else
	{
		common->Printf( "X..%s not found\n", "GLEW_KHR_debug" );
	}

	// GL_ARB_framebuffer_object
	glConfig.framebufferObjectAvailable = GLEW_ARB_framebuffer_object != 0;
	if( glConfig.framebufferObjectAvailable )
	{
		glGetIntegerv( GL_MAX_RENDERBUFFER_SIZE, &glConfig.maxRenderbufferSize );
		glGetIntegerv( GL_MAX_COLOR_ATTACHMENTS, &glConfig.maxColorAttachments );

		common->Printf( "...using %s\n", "GL_ARB_framebuffer_object" );
	}
	else
	{
		common->Printf( "X..%s not found\n", "GL_ARB_framebuffer_object" );
	}

	// GL_EXT_framebuffer_blit
	glConfig.framebufferBlitAvailable = GLEW_EXT_framebuffer_blit != 0;
	if( glConfig.framebufferBlitAvailable )
	{
		common->Printf( "...using %s\n", "GL_EXT_framebuffer_blit" );
	}
	else
	{
		common->Printf( "X..%s not found\n", "GL_EXT_framebuffer_blit" );
	}

	// GL_ARB_debug_output
	glConfig.debugOutputAvailable = GLEW_ARB_debug_output != 0;
	if( glConfig.debugOutputAvailable )
	{
		if( r_debugContext.GetInteger() >= 1 )
		{
			glDebugMessageCallbackARB( ( GLDEBUGPROCARB ) DebugCallback, NULL );
		}
		if( r_debugContext.GetInteger() >= 2 )
		{
			// force everything to happen in the main thread instead of in a separate driver thread
			glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB );
		}
		if( r_debugContext.GetInteger() >= 3 )
		{
			// enable all the low priority messages
			glDebugMessageControlARB( GL_DONT_CARE,
									  GL_DONT_CARE,
									  GL_DEBUG_SEVERITY_LOW_ARB,
									  0, NULL, true );
		}
	}

	// GL_ARB_multitexture
	if( !glConfig.multitextureAvailable )
	{
		idLib::Error( "GL_ARB_multitexture not available" );
	}
	// GL_ARB_texture_compression + GL_EXT_texture_compression_s3tc
	if( !glConfig.textureCompressionAvailable )
	{
		idLib::Error( "GL_ARB_texture_compression or GL_EXT_texture_compression_s3tc not available" );
	}
	// GL_ARB_vertex_buffer_object
	if( !glConfig.vertexBufferObjectAvailable )
	{
		idLib::Error( "GL_ARB_vertex_buffer_object not available" );
	}
	// GL_ARB_map_buffer_range
	if( !glConfig.mapBufferRangeAvailable )
	{
		idLib::Error( "GL_ARB_map_buffer_range not available" );
	}
	// GL_ARB_vertex_array_object
	if( !glConfig.vertexArrayObjectAvailable )
	{
		idLib::Error( "GL_ARB_vertex_array_object not available" );
	}
	// GL_ARB_draw_elements_base_vertex
	if( !glConfig.drawElementsBaseVertexAvailable )
	{
		idLib::Error( "GL_ARB_draw_elements_base_vertex not available" );
	}
	// GL_ARB_vertex_program / GL_ARB_fragment_program
	//if( !glConfig.fragmentProgramAvailable )
	//{
	//	idLib::Warning( "GL_ARB_fragment_program not available" );
	//}
	// GLSL
	if( !glConfig.glslAvailable )
	{
		idLib::Error( "GLSL not available" );
	}
	// GL_ARB_uniform_buffer_object
	if( !glConfig.uniformBufferAvailable )
	{
		idLib::Error( "GL_ARB_uniform_buffer_object not available" );
	}
	// GL_EXT_stencil_two_side
	if( !glConfig.twoSidedStencilAvailable )
	{
		idLib::Error( "GL_ATI_separate_stencil not available" );
	}

	// generate one global Vertex Array Object (VAO)
	glGenVertexArrays( 1, &glConfig.global_vao );
	glBindVertexArray( glConfig.global_vao );
}
// RB end


idStr extensions_string;

/*
==================
R_InitOpenGL

This function is responsible for initializing a valid OpenGL subsystem
for rendering.  This is done by calling the system specific GLimp_Init,
which gives us a working OGL subsystem, then setting all necessary openGL
state, including images, vertex programs, and display lists.

Changes to the vertex cache size or smp state require a vid_restart.

If R_IsInitialized() is false, no rendering can take place, but
all renderSystem functions will still operate properly, notably the material
and model information functions.
==================
*/
void R_InitOpenGL()
{
	common->Printf( "----- R_InitOpenGL -----\n" );

	if( tr.IsInitialized() )
	{
		common->FatalError( "R_InitOpenGL called while active" );
	}

	// DG: make sure SDL has setup video so getting supported modes in R_SetNewMode() works
	GLimp_PreInit();
	// DG end

	R_SetNewMode( true );


	// input and sound systems need to be tied to the new window
	Sys_InitInput();

	// get our config strings
	glConfig.vendor_string = ( const char* )glGetString( GL_VENDOR );
	glConfig.renderer_string = ( const char* )glGetString( GL_RENDERER );
	glConfig.version_string = ( const char* )glGetString( GL_VERSION );
	glConfig.shading_language_string = ( const char* )glGetString( GL_SHADING_LANGUAGE_VERSION );
	glConfig.extensions_string = ( const char* )glGetString( GL_EXTENSIONS );

	if( glConfig.extensions_string == NULL )
	{
		// As of OpenGL 3.2, glGetStringi is required to obtain the available extensions
		//glGetStringi = ( PFNGLGETSTRINGIPROC )GLimp_ExtensionPointer( "glGetStringi" );

		// Build the extensions string
		GLint numExtensions;
		glGetIntegerv( GL_NUM_EXTENSIONS, &numExtensions );
		extensions_string.Clear();
		for( int i = 0; i < numExtensions; i++ )
		{
			extensions_string.Append( ( const char* )glGetStringi( GL_EXTENSIONS, i ) );
			// the now deprecated glGetString method usaed to create a single string with each extension separated by a space
			if( i < numExtensions - 1 )
			{
				extensions_string.Append( ' ' );
			}
		}
		glConfig.extensions_string = extensions_string.c_str();
	}


	float glVersion = atof( glConfig.version_string );
	float glslVersion = atof( glConfig.shading_language_string );
	idLib::Printf( "OpenGL Version  : %3.1f\n", glVersion );
	idLib::Printf( "OpenGL Vendor   : %s\n", glConfig.vendor_string );
	idLib::Printf( "OpenGL Renderer : %s\n", glConfig.renderer_string );
	idLib::Printf( "OpenGL GLSL     : %3.1f\n", glslVersion );

	// OpenGL driver constants
	GLint temp;
	glGetIntegerv( GL_MAX_TEXTURE_SIZE, &temp );
	glConfig.maxTextureSize = temp;

	// stubbed or broken drivers may have reported 0...
	if( glConfig.maxTextureSize <= 0 )
	{
		glConfig.maxTextureSize = 256;
	}

	tr.SetInitialized();

	// recheck all the extensions (FIXME: this might be dangerous)
	R_CheckPortableExtensions();

	renderProgManager.Init();

	tr.SetInitialized();

	// allocate the vertex array range or vertex objects
	vertexCache.Init();

	// allocate the frame data, which may be more if smp is enabled
	R_InitFrameData();

	// Reset our gamma
	R_SetColorMappings();

	// RB begin
#if defined(_WIN32)
	static bool glCheck = false;
	if( !glCheck && win32.osversion.dwMajorVersion == 6 )
	{
		glCheck = true;
		if( !idStr::Icmp( glConfig.vendor_string, "Microsoft" ) && idStr::FindText( glConfig.renderer_string, "OpenGL-D3D" ) != -1 )
		{
			if( cvarSystem->GetCVarBool( "r_fullscreen" ) )
			{
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "vid_restart partial windowed\n" );
				Sys_GrabMouseCursor( false );
			}
			int ret = MessageBox( NULL, "Please install OpenGL drivers from your graphics hardware vendor to run " GAME_NAME ".\nYour OpenGL functionality is limited.",
								  "Insufficient OpenGL capabilities", MB_OKCANCEL | MB_ICONWARNING | MB_TASKMODAL );
			if( ret == IDCANCEL )
			{
				cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "quit\n" );
				cmdSystem->ExecuteCommandBuffer();
			}
			if( cvarSystem->GetCVarBool( "r_fullscreen" ) )
			{
				cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "vid_restart\n" );
			}
		}
	}
#endif
	// RB end
}



/*
=============
RB_DrawFlickerBox
=============
*/
static void RB_DrawFlickerBox()
{
	if( !r_drawFlickerBox.GetBool() )
	{
		return;
	}
	if( tr.frameCount & 1 )
	{
		glClearColor( 1, 0, 0, 1 );
	}
	else
	{
		glClearColor( 0, 1, 0, 1 );
	}
	glScissor( 0, 0, 256, 256 );
	glClear( GL_COLOR_BUFFER_BIT );
}

/*
=============
RB_SetBuffer
=============
*/
static void	RB_SetBuffer( const void* data )
{
	// see which draw buffer we want to render the frame to

	const setBufferCommand_t* cmd = ( const setBufferCommand_t* )data;

	RENDERLOG_PRINTF( "---------- RB_SetBuffer ---------- to buffer # %d\n", cmd->buffer );

	GL_Scissor( 0, 0, tr.GetWidth(), tr.GetHeight() );

	// clear screen for debugging
	// automatically enable this with several other debug tools
	// that might leave unrendered portions of the screen
	if( r_clear.GetFloat() || idStr::Length( r_clear.GetString() ) != 1 || r_singleArea.GetBool() || r_showOverDraw.GetBool() )
	{
		float c[3];
		if( sscanf( r_clear.GetString(), "%f %f %f", &c[0], &c[1], &c[2] ) == 3 )
		{
			GL_Clear( true, false, false, 0, c[0], c[1], c[2], 1.0f );
		}
		else if( r_clear.GetInteger() == 2 )
		{
			GL_Clear( true, false, false, 0, 0.0f, 0.0f,  0.0f, 1.0f );
		}
		else if( r_showOverDraw.GetBool() )
		{
			GL_Clear( true, false, false, 0, 1.0f, 1.0f, 1.0f, 1.0f );
		}
		else
		{
			GL_Clear( true, false, false, 0, 0.4f, 0.0f, 0.25f, 1.0f );
		}
	}
}

/*
=============
GL_BlockingSwapBuffers

We want to exit this with the GPU idle, right at vsync
=============
*/
const void GL_BlockingSwapBuffers()
{
	RENDERLOG_PRINTF( "***************** GL_BlockingSwapBuffers *****************\n\n\n" );

	const int beforeFinish = Sys_Milliseconds();

	if( !glConfig.syncAvailable )
	{
		glFinish();
	}

	const int beforeSwap = Sys_Milliseconds();
	if( r_showSwapBuffers.GetBool() && beforeSwap - beforeFinish > 1 )
	{
		common->Printf( "%i msec to glFinish\n", beforeSwap - beforeFinish );
	}


	GLimp_SwapBuffers();

	if( game->isVR && !commonVr->hasOculusRift )
	{
		commonVr->FrameStart();
	}

	const int beforeFence = Sys_Milliseconds();
	if( r_showSwapBuffers.GetBool() && beforeFence - beforeSwap > 1 )
	{
		common->Printf( "%i msec to swapBuffers\n", beforeFence - beforeSwap );
	}

	if( glConfig.syncAvailable )
	{
		swapIndex ^= 1;

		if( glIsSync( renderSync[swapIndex] ) )
		{
			glDeleteSync( renderSync[swapIndex] );
		}
		// draw something tiny to ensure the sync is after the swap
		const int start = Sys_Milliseconds();
		glScissor( 0, 0, 1, 1 );
		glEnable( GL_SCISSOR_TEST );
		glClear( GL_COLOR_BUFFER_BIT );
		renderSync[swapIndex] = glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
		const int end = Sys_Milliseconds();
		if( r_showSwapBuffers.GetBool() && end - start > 1 )
		{
			common->Printf( "%i msec to start fence\n", end - start );
		}

		GLsync	syncToWaitOn;
		if( r_syncEveryFrame.GetBool() )
		{
			syncToWaitOn = renderSync[swapIndex];
		}
		else
		{
			syncToWaitOn = renderSync[!swapIndex];
		}

		if( glIsSync( syncToWaitOn ) )
		{
			for( GLenum r = GL_TIMEOUT_EXPIRED; r == GL_TIMEOUT_EXPIRED; )
			{
				r = glClientWaitSync( syncToWaitOn, GL_SYNC_FLUSH_COMMANDS_BIT, 1000 * 1000 );
			}
		}
	}

	const int afterFence = Sys_Milliseconds();
	if( r_showSwapBuffers.GetBool() && afterFence - beforeFence > 1 )
	{
		common->Printf( "%i msec to wait on fence\n", afterFence - beforeFence );
	}

	const int64 exitBlockTime = Sys_Microseconds();

	static int64 prevBlockTime;
	if( r_showSwapBuffers.GetBool() && prevBlockTime )
	{
		const int delta = ( int )( exitBlockTime - prevBlockTime );
		common->Printf( "blockToBlock: %i\n", delta );
	}
	prevBlockTime = exitBlockTime;

}

/*
========================
GL_SetDefaultState

This should initialize all GL state that any part of the entire program
may touch, including the editor.
========================
*/
void GL_SetDefaultState()
{
	RENDERLOG_PRINTF( "--- GL_SetDefaultState ---\n" );

	glClearDepth( 1.0f );

	// make sure our GL state vector is set correctly
	memset( &backEnd.glState, 0, sizeof( backEnd.glState ) );
	GL_State( 0, true );

	// RB begin
	Framebuffer::BindDefault();
	// RB end

	// These are changed by GL_Cull
	glCullFace( GL_FRONT_AND_BACK );
	glEnable( GL_CULL_FACE );

	// These are changed by GL_State
	glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
	glBlendFunc( GL_ONE, GL_ZERO );
	glDepthMask( GL_TRUE );
	glDepthFunc( GL_LESS );
	glDisable( GL_STENCIL_TEST );
	glDisable( GL_POLYGON_OFFSET_FILL );
	glDisable( GL_POLYGON_OFFSET_LINE );
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

	// These should never be changed
	// DG: deprecated in opengl 3.2 and not needed because we don't do fixed function pipeline
	// glShadeModel( GL_SMOOTH );
	// DG end
	glEnable( GL_DEPTH_TEST );
	glEnable( GL_BLEND );
	glEnable( GL_SCISSOR_TEST );
	//glDrawBuffer( GL_BACK );
	//glReadBuffer( GL_BACK );

	glEnable( GL_MULTISAMPLE );

	if( r_useScissor.GetBool() )
	{
		glScissor( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight() );
	}

//	if ( useFBO ) {
//		globalFramebuffers.primaryFBO->Bind();
//	}
	//renderProgManager.Unbind();
}

/*
====================
R_MakeStereoRenderImage
====================
*/
static void R_MakeStereoRenderImage( idImage* image )
{
	idImageOpts	opts;
	opts.width = renderSystem->GetWidth();
	opts.height = renderSystem->GetHeight();
	opts.numLevels = 1;
	opts.format = FMT_RGBA8;
	image->AllocImage( opts, TF_LINEAR, TR_CLAMP );
}

/*
====================
RB_StereoRenderExecuteBackEndCommands

Renders the draw list twice, with slight modifications for left eye / right eye
====================
*/
void RB_StereoRenderExecuteBackEndCommands( const emptyCommand_t* const allCmds )
{
	uint64 backEndStartTime = Sys_Microseconds();

	// If we are in a monoscopic context, this draws to the only buffer, and is
	// the same as GL_BACK.  In a quad-buffer stereo context, this is necessary
	// to prevent GL from forcing the rendering to go to both BACK_LEFT and
	// BACK_RIGHT at a performance penalty.
	// To allow stereo deghost processing, the views have to be copied to separate
	// textures anyway, so there isn't any benefit to rendering to BACK_RIGHT for
	// that eye.

	// Koz begin
	if( commonVr->useFBO )
	{
		globalFramebuffers.primaryFBO->Bind();
	}
	else
	{
		glDrawBuffer( GL_BACK_LEFT );
	}
	// Koz end


	// create the stereoRenderImage if we haven't already
	static idImage* stereoRenderImages[2];
	for( int i = 0; i < 2; i++ )
	{
		if( stereoRenderImages[i] == NULL )
		{
			stereoRenderImages[i] = globalImages->ImageFromFunction( va( "_stereoRender%i", i ), R_MakeStereoRenderImage );
		}

		// resize the stereo render image if the main window has changed size
		if( stereoRenderImages[i]->GetUploadWidth() != renderSystem->GetWidth() ||
				stereoRenderImages[i]->GetUploadHeight() != renderSystem->GetHeight() )
		{
			stereoRenderImages[i]->Resize( renderSystem->GetWidth(), renderSystem->GetHeight() );
		}
	}

	// In stereoRender mode, the front end has generated two RC_DRAW_VIEW commands
	// with slightly different origins for each eye.

	// TODO: only do the copy after the final view has been rendered, not mirror subviews?

	// Render the 3D draw views from the screen origin so all the screen relative
	// texture mapping works properly, then copy the portion we are going to use
	// off to a texture.
	bool foundEye[2] = { false, false };

	for( int stereoEye = 1; stereoEye >= -1; stereoEye -= 2 )
	{
		// set up the target texture we will draw to
		int targetEye = ( stereoEye == 1 ) ? 1 : 0;

		// Set the back end into a known default state to fix any stale render state issues
		GL_SetDefaultState();
		renderProgManager.Unbind();
		renderProgManager.ZeroUniforms();
		for( const emptyCommand_t* cmds = allCmds; cmds != NULL; cmds = ( const emptyCommand_t* )cmds->next )
		{
			switch( cmds->commandId )
			{
				case RC_NOP:
					break;
				case RC_DRAW_VIEW_GUI:
				case RC_DRAW_VIEW_3D:
				{
					const drawSurfsCommand_t* const dsc = ( const drawSurfsCommand_t* )cmds;
					const viewDef_t&			eyeViewDef = *dsc->viewDef;

					if( eyeViewDef.renderView.viewEyeBuffer && eyeViewDef.renderView.viewEyeBuffer != stereoEye )
					{
						// this is the render view for the other eye
						continue;
					}

					foundEye[ targetEye ] = true;
					RB_DrawView( dsc, stereoEye );
					if( cmds->commandId == RC_DRAW_VIEW_GUI )
					{
					}
				}
				break;

				case RC_SET_BUFFER:
					RB_SetBuffer( cmds );
					break;

				case RC_COPY_RENDER:
					RB_CopyRender( cmds );
					break;

				case RC_POST_PROCESS:
				{
					postProcessCommand_t* cmd = ( postProcessCommand_t* )cmds;
					if( cmd->viewDef->renderView.viewEyeBuffer != stereoEye )
					{
						break;
					}
					RB_PostProcess( cmds );
				}
				break;

				default:
					common->Error( "RB_ExecuteBackEndCommands: bad commandId" );
					break;
			}
		}

		// copy to the target
		stereoRenderImages[ targetEye ]->CopyFramebuffer( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight() );
		commonVr->hmdCurrentRender[targetEye] = stereoRenderImages[targetEye];
	}

	// perform the final compositing / warping / deghosting to the actual framebuffer(s)
	assert( foundEye[0] && foundEye[1] );

	GL_SetDefaultState();

	RB_SetMVP( renderMatrix_identity );

	// If we are in quad-buffer pixel format but testing another 3D mode,
	// make sure we draw to both eyes.  This is likely to be sub-optimal
	// performance on most cards and drivers, but it is better than getting
	// a confusing, half-ghosted view.
	if( renderSystem->GetStereo3DMode() != STEREO3D_QUAD_BUFFER )
	{
		if( !commonVr->useFBO )
		{
			glDrawBuffer( GL_BACK );    // Koz fixme
		}

	}

	GL_State( GLS_DEPTHFUNC_ALWAYS );
	GL_Cull( CT_TWO_SIDED );

	// We just want to do a quad pass - so make sure we disable any texgen and
	// set the texture matrix to the identity so we don't get anomalies from
	// any stale uniform data being present from a previous draw call
	const float texS[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
	const float texT[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
	renderProgManager.SetRenderParm( RENDERPARM_TEXTUREMATRIX_S, texS );
	renderProgManager.SetRenderParm( RENDERPARM_TEXTUREMATRIX_T, texT );

	// disable any texgen
	const float texGenEnabled[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_0_ENABLED, texGenEnabled );

	renderProgManager.BindShader_Texture();
	GL_Color( 1, 1, 1, 1 );


	//common->Printf( "SREBEC Rendering frame %d\n", idLib::frameNumber );
	switch( renderSystem->GetStereo3DMode() )
	{
		case STEREO3D_QUAD_BUFFER:
			glDrawBuffer( GL_BACK_RIGHT );
			GL_SelectTexture( 0 );
			stereoRenderImages[1]->Bind();
			GL_SelectTexture( 1 );
			stereoRenderImages[0]->Bind();
			RB_DrawElementsWithCounters( &backEnd.unitSquareSurface );

			glDrawBuffer( GL_BACK_LEFT );
			GL_SelectTexture( 1 );
			stereoRenderImages[1]->Bind();
			GL_SelectTexture( 0 );
			stereoRenderImages[0]->Bind();
			RB_DrawElementsWithCounters( &backEnd.unitSquareSurface );

			break;
		case STEREO3D_HDMI_720:
			// HDMI 720P 3D
			GL_SelectTexture( 0 );
			stereoRenderImages[1]->Bind();
			GL_SelectTexture( 1 );
			stereoRenderImages[0]->Bind();
			GL_ViewportAndScissor( 0, 0, 1280, 720 );
			RB_DrawElementsWithCounters( &backEnd.unitSquareSurface );

			GL_SelectTexture( 0 );
			stereoRenderImages[0]->Bind();
			GL_SelectTexture( 1 );
			stereoRenderImages[1]->Bind();
			GL_ViewportAndScissor( 0, 750, 1280, 720 );
			RB_DrawElementsWithCounters( &backEnd.unitSquareSurface );

			// force the HDMI 720P 3D guard band to a constant color
			glScissor( 0, 720, 1280, 30 );
			glClear( GL_COLOR_BUFFER_BIT );
			break;
		default:

		case STEREO3D_HMD:

			// Koz begin
			// This is the rift.

			if( game->isVR )
			{


				if( commonVr->playerDead || ( game->Shell_IsActive() && !commonVr->PDAforced && !commonVr->PDAforcetoggle ) || ( !commonVr->PDAforced && common->Dialog().IsDialogActive() )
						|| commonVr->isLoading || commonVr->showingIntroVideo || session->GetState() == idSession::LOADING || ( gameLocal.inCinematic && vr_cinematics.GetInteger() == 2 && vr_flicksyncCharacter.GetInteger() == 0 ) )
				{
					commonVr->HMDTrackStatic( !commonVr->isLoading && !commonVr->showingIntroVideo && session->GetState() != idSession::LOADING );//  && (gameLocal.inCinematic && vr_cinematics.GetInteger() == 0) );

				}
				else
				{
					commonVr->HMDRender( stereoRenderImages[0], stereoRenderImages[1] );
				}

				// Koz GL_CheckErrors();

			}
			break;
		// Koz end


		case STEREO3D_SIDE_BY_SIDE:

		// a non-warped side-by-side-uncompressed (dual input cable) is rendered
		// just like STEREO3D_SIDE_BY_SIDE_COMPRESSED, so fall through.
		case STEREO3D_SIDE_BY_SIDE_COMPRESSED:
			GL_SelectTexture( 0 );
			stereoRenderImages[0]->Bind();
			GL_SelectTexture( 1 );
			stereoRenderImages[1]->Bind();
			GL_ViewportAndScissor( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight() );
			RB_DrawElementsWithCounters( &backEnd.unitSquareSurface );

			GL_SelectTexture( 0 );
			stereoRenderImages[1]->Bind();
			GL_SelectTexture( 1 );
			stereoRenderImages[0]->Bind();
			GL_ViewportAndScissor( renderSystem->GetWidth(), 0, renderSystem->GetWidth(), renderSystem->GetHeight() );
			RB_DrawElementsWithCounters( &backEnd.unitSquareSurface );
			break;

		case STEREO3D_TOP_AND_BOTTOM_COMPRESSED:
			GL_SelectTexture( 1 );
			stereoRenderImages[0]->Bind();
			GL_SelectTexture( 0 );
			stereoRenderImages[1]->Bind();
			GL_ViewportAndScissor( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight() );
			RB_DrawElementsWithCounters( &backEnd.unitSquareSurface );

			GL_SelectTexture( 1 );
			stereoRenderImages[1]->Bind();
			GL_SelectTexture( 0 );
			stereoRenderImages[0]->Bind();
			GL_ViewportAndScissor( 0, renderSystem->GetHeight(), renderSystem->GetWidth(), renderSystem->GetHeight() );
			RB_DrawElementsWithCounters( &backEnd.unitSquareSurface );
			break;

		case STEREO3D_INTERLACED:
			// every other scanline
			GL_SelectTexture( 0 );
			stereoRenderImages[0]->Bind();
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

			GL_SelectTexture( 1 );
			stereoRenderImages[1]->Bind();
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

			GL_ViewportAndScissor( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight() * 2 );
			renderProgManager.BindShader_StereoInterlace();
			RB_DrawElementsWithCounters( &backEnd.unitSquareSurface );

			GL_SelectTexture( 0 );
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

			GL_SelectTexture( 1 );
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

			break;
	}

	// debug tool
	RB_DrawFlickerBox();

	// make sure the drawing is actually started
	glFlush();

	// we may choose to sync to the swapbuffers before the next frame

	// stop rendering on this thread
	uint64 backEndFinishTime = Sys_Microseconds();
	backEnd.pc.cpuTotalMicroSec = backEndFinishTime - backEndStartTime;
}

/*
====================
RB_ExecuteBackEndCommands

This function will be called syncronously if running without
smp extensions, or asyncronously by another thread.
====================
*/
void RB_ExecuteBackEndCommands( const emptyCommand_t* cmds )
{
	// r_debugRenderToTexture
	int c_draw3d = 0;
	int c_draw2d = 0;
	int c_setBuffers = 0;
	int c_copyRenders = 0;

	resolutionScale.SetCurrentGPUFrameTime( commonLocal.GetRendererGPUMicroseconds() );

	renderLog.StartFrame();

	if( cmds->commandId == RC_NOP && !cmds->next )
	{
		return;
	}

	if( renderSystem->GetStereo3DMode() != STEREO3D_OFF )
	{
		RB_StereoRenderExecuteBackEndCommands( cmds );
		renderLog.EndFrame();
		return;
	}

	uint64 backEndStartTime = Sys_Microseconds();

	// needed for editor rendering
	GL_SetDefaultState();

	// If we have a stereo pixel format, this will draw to both
	// the back left and back right buffers, which will have a
	// performance penalty.
	if( !commonVr->useFBO )
	{
		glDrawBuffer( GL_BACK );    // Koz
	}


	for( ; cmds != NULL; cmds = ( const emptyCommand_t* )cmds->next )
	{
		switch( cmds->commandId )
		{
			case RC_NOP:
				break;
			case RC_DRAW_VIEW_3D:
			case RC_DRAW_VIEW_GUI:
				RB_DrawView( cmds, 0 );
				if( ( ( const drawSurfsCommand_t* )cmds )->viewDef->viewEntitys )
				{
					c_draw3d++;
				}
				else
				{
					c_draw2d++;
				}
				break;
			case RC_SET_BUFFER:
				c_setBuffers++;
				break;
			case RC_COPY_RENDER:
				RB_CopyRender( cmds );
				c_copyRenders++;
				break;
			case RC_POST_PROCESS:
				RB_PostProcess( cmds );
				break;
			default:
				common->Error( "RB_ExecuteBackEndCommands: bad commandId" );
				break;
		}
	}

	RB_DrawFlickerBox();

	// Fix for the steam overlay not showing up while in game without Shell/Debug/Console/Menu also rendering
	glColorMask( 1, 1, 1, 1 );

	glFlush();

	// stop rendering on this thread
	uint64 backEndFinishTime = Sys_Microseconds();
	backEnd.pc.cpuTotalMicroSec = backEndFinishTime - backEndStartTime;

	if( r_debugRenderToTexture.GetInteger() == 1 )
	{
		common->Printf( "3d: %i, 2d: %i, SetBuf: %i, CpyRenders: %i, CpyFrameBuf: %i\n", c_draw3d, c_draw2d, c_setBuffers, c_copyRenders, backEnd.pc.c_copyFrameBuffer );
		backEnd.pc.c_copyFrameBuffer = 0;
	}
	renderLog.EndFrame();
}
