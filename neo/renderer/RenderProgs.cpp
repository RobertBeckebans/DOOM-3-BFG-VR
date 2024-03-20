/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2014 Robert Beckebans

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

#include "RenderCommon.h"



idRenderProgManager renderProgManager;

/*
================================================================================================
idRenderProgManager::idRenderProgManager()
================================================================================================
*/
idRenderProgManager::idRenderProgManager()
{
}

/*
================================================================================================
idRenderProgManager::~idRenderProgManager()
================================================================================================
*/
idRenderProgManager::~idRenderProgManager()
{
}

/*
================================================================================================
R_ReloadShaders
================================================================================================
*/
static void R_ReloadShaders( const idCmdArgs& args )
{
	renderProgManager.KillAllShaders();
	renderProgManager.LoadAllShaders();
}

/*
================================================================================================
idRenderProgManager::Init()
================================================================================================
*/
void idRenderProgManager::Init()
{
	common->Printf( "----- Initializing Render Shaders -----\n" );


	for( int i = 0; i < MAX_BUILTINS; i++ )
	{
		builtinShaders[i] = -1;
	}

	// RB: added checks for GPU skinning
	struct builtinShaders_t
	{
		int					index;
		const char*			name;
		const char*			nameOutSuffix;
		uint32				shaderFeatures;
		bool				requireGPUSkinningSupport;
		rpStage_t			stages;
		vertexLayoutType_t	layout;
	} builtins[] =
	{
		{ BUILTIN_GUI, "builtin/gui", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_COLOR, "builtin/color", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		// RB begin
		{ BUILTIN_COLOR_SKINNED, "builtin/color", "_skinned", BIT( USE_GPU_SKINNING ), true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_VERTEX_COLOR, "builtin/vertex_color", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		// RB end
		{ BUILTIN_TEXTURED, "builtin/texture", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_TEXTURE_VERTEXCOLOR, "builtin/texture_color", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_TEXTURE_VERTEXCOLOR_SKINNED, "builtin/texture_color_skinned", "", 0, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_TEXTURE_TEXGEN_VERTEXCOLOR, "builtin/texture_color_texgen", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },

		// RB begin
		{ BUILTIN_INTERACTION, "builtin/lighting/interaction", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_INTERACTION_SKINNED, "builtin/lighting/interaction", "_skinned", BIT( USE_GPU_SKINNING ), true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },

		{ BUILTIN_INTERACTION_AMBIENT, "builtin/lighting/interactionAmbient", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_INTERACTION_AMBIENT_SKINNED, "builtin/lighting/interactionAmbient_skinned", "", 0, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },

		{ BUILTIN_INTERACTION_SHADOW_MAPPING_SPOT, "builtin/lighting/interactionSM", "_spot", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_INTERACTION_SHADOW_MAPPING_SPOT_SKINNED, "builtin/lighting/interactionSM", "_spot_skinned", BIT( USE_GPU_SKINNING ), true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },

		{ BUILTIN_INTERACTION_SHADOW_MAPPING_POINT, "builtin/lighting/interactionSM", "_point", BIT( LIGHT_POINT ), false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_INTERACTION_SHADOW_MAPPING_POINT_SKINNED, "builtin/lighting/interactionSM", "_point_skinned", BIT( USE_GPU_SKINNING ) | BIT( LIGHT_POINT ), true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },

		{ BUILTIN_INTERACTION_SHADOW_MAPPING_PARALLEL, "builtin/lighting/interactionSM", "_parallel", BIT( LIGHT_PARALLEL ), false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_INTERACTION_SHADOW_MAPPING_PARALLEL_SKINNED, "builtin/lighting/interactionSM", "_parallel_skinned", BIT( USE_GPU_SKINNING ) | BIT( LIGHT_PARALLEL ), true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		// RB end
		{ BUILTIN_ENVIRONMENT, "builtin/legacy/environment", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_ENVIRONMENT_SKINNED, "builtin/legacy/environment_skinned", "", 0, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_BUMPY_ENVIRONMENT, "builtin/legacy/bumpyenvironment", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_BUMPY_ENVIRONMENT_SKINNED, "builtin/legacy/bumpyenvironment_skinned", "", 0, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },

		{ BUILTIN_DEPTH, "builtin/depth", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_DEPTH_SKINNED, "builtin/depth_skinned", "", 0, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },

		{ BUILTIN_SHADOW, "builtin/lighting/shadow", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_SHADOW_VERT },
		{ BUILTIN_SHADOW_SKINNED, "builtin/lighting/shadow_skinned", "", 0, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_SHADOW_VERT_SKINNED },

		{ BUILTIN_SHADOW_DEBUG, "builtin/debug/shadowDebug", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_SHADOW_DEBUG_SKINNED, "builtin/debug/shadowDebug_skinned", "", 0, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },

		{ BUILTIN_BLENDLIGHT, "builtin/fog/blendlight", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_FOG, "builtin/fog/fog", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_FOG_SKINNED, "builtin/fog/fog_skinned", "", 0, true, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_SKYBOX, "builtin/legacy/skybox", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_WOBBLESKY, "builtin/legacy/wobblesky", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_POSTPROCESS, "builtin/post/postprocess", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_STEREO_DEGHOST, "builtin/VR/stereoDeGhost", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_STEREO_WARP, "builtin/VR/stereoWarp", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_BINK, "builtin/video/bink", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_BINK_GUI, "builtin/video/bink_gui", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_STEREO_INTERLACE, "builtin/VR/stereoInterlace", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		{ BUILTIN_MOTION_BLUR, "builtin/post/motionBlur", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },

		// RB begin
		{ BUILTIN_DEBUG_SHADOWMAP, "builtin/debug/debug_shadowmap", "", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		// RB end

		// Koz begin
		//{ BUILTIN_VRFXAA, "oculusVrFxaa.vfp", 0, false, SHADER_STAGE_DEFAULT, LAYOUT_DRAW_VERT },
		// Koz end
	};
	int numBuiltins = sizeof( builtins ) / sizeof( builtins[0] );
	vertexShaders.SetNum( numBuiltins );
	fragmentShaders.SetNum( numBuiltins );
	glslPrograms.SetNum( numBuiltins );

	for( int i = 0; i < numBuiltins; i++ )
	{
		vertexShaders[i].name = builtins[i].name;
		vertexShaders[i].nameOutSuffix = builtins[i].nameOutSuffix;
		vertexShaders[i].shaderFeatures = builtins[i].shaderFeatures;
		vertexShaders[i].builtin = true;

		fragmentShaders[i].name = builtins[i].name;
		fragmentShaders[i].nameOutSuffix = builtins[i].nameOutSuffix;
		fragmentShaders[i].shaderFeatures = builtins[i].shaderFeatures;
		fragmentShaders[i].builtin = true;

		builtinShaders[builtins[i].index] = i;

		if( builtins[i].requireGPUSkinningSupport && !glConfig.gpuSkinningAvailable )
		{
			// RB: don't try to load shaders that would break the GLSL compiler in the OpenGL driver
			continue;
		}

		LoadVertexShader( i );
		LoadFragmentShader( i );
		LoadGLSLProgram( i, i, i );
	}

	glslUniforms.SetNum( RENDERPARM_USER + MAX_GLSL_USER_PARMS, vec4_zero );

	if( glConfig.gpuSkinningAvailable )
	{
		vertexShaders[builtinShaders[BUILTIN_TEXTURE_VERTEXCOLOR_SKINNED]].usesJoints = true;
		vertexShaders[builtinShaders[BUILTIN_INTERACTION_SKINNED]].usesJoints = true;
		vertexShaders[builtinShaders[BUILTIN_INTERACTION_AMBIENT_SKINNED]].usesJoints = true;
		vertexShaders[builtinShaders[BUILTIN_ENVIRONMENT_SKINNED]].usesJoints = true;
		vertexShaders[builtinShaders[BUILTIN_BUMPY_ENVIRONMENT_SKINNED]].usesJoints = true;
		vertexShaders[builtinShaders[BUILTIN_DEPTH_SKINNED]].usesJoints = true;
		vertexShaders[builtinShaders[BUILTIN_SHADOW_SKINNED]].usesJoints = true;
		vertexShaders[builtinShaders[BUILTIN_SHADOW_DEBUG_SKINNED]].usesJoints = true;
		vertexShaders[builtinShaders[BUILTIN_FOG_SKINNED]].usesJoints = true;
		// RB begin
		vertexShaders[builtinShaders[BUILTIN_INTERACTION_SHADOW_MAPPING_SPOT_SKINNED]].usesJoints = true;
		vertexShaders[builtinShaders[BUILTIN_INTERACTION_SHADOW_MAPPING_POINT_SKINNED]].usesJoints = true;
		vertexShaders[builtinShaders[BUILTIN_INTERACTION_SHADOW_MAPPING_PARALLEL_SKINNED]].usesJoints = true;
		// RB end
	}

	cmdSystem->AddCommand( "reloadShaders", R_ReloadShaders, CMD_FL_RENDERER, "reloads shaders" );
}

/*
================================================================================================
idRenderProgManager::LoadAllShaders()
================================================================================================
*/
void idRenderProgManager::LoadAllShaders()
{
	for( int i = 0; i < vertexShaders.Num(); i++ )
	{
		LoadVertexShader( i );
	}
	for( int i = 0; i < fragmentShaders.Num(); i++ )
	{
		LoadFragmentShader( i );
	}

	for( int i = 0; i < glslPrograms.Num(); ++i )
	{
		if( glslPrograms[i].vertexShaderIndex == -1 || glslPrograms[i].fragmentShaderIndex == -1 )
		{
			// RB: skip reloading because we didn't load it initially
			continue;
		}

		LoadGLSLProgram( i, glslPrograms[i].vertexShaderIndex, glslPrograms[i].fragmentShaderIndex );
	}
}

/*
================================================================================================
idRenderProgManager::KillAllShaders()
================================================================================================
*/
void idRenderProgManager::KillAllShaders()
{
	Unbind();
	for( int i = 0; i < vertexShaders.Num(); i++ )
	{
		if( vertexShaders[i].progId != INVALID_PROGID )
		{
			glDeleteShader( vertexShaders[i].progId );
			vertexShaders[i].progId = INVALID_PROGID;
		}
	}
	for( int i = 0; i < fragmentShaders.Num(); i++ )
	{
		if( fragmentShaders[i].progId != INVALID_PROGID )
		{
			glDeleteShader( fragmentShaders[i].progId );
			fragmentShaders[i].progId = INVALID_PROGID;
		}
	}
	for( int i = 0; i < glslPrograms.Num(); ++i )
	{
		if( glslPrograms[i].progId != INVALID_PROGID )
		{
			glDeleteProgram( glslPrograms[i].progId );
			glslPrograms[i].progId = INVALID_PROGID;
		}
	}
}

/*
================================================================================================
idRenderProgManager::Shutdown()
================================================================================================
*/
void idRenderProgManager::Shutdown()
{
	KillAllShaders();
}

/*
================================================================================================
idRenderProgManager::FindVertexShader
================================================================================================
*/
int idRenderProgManager::FindVertexShader( const char* name )
{
	for( int i = 0; i < vertexShaders.Num(); i++ )
	{
		if( vertexShaders[i].name.Icmp( name ) == 0 )
		{
			LoadVertexShader( i );
			return i;
		}
	}
	vertexShader_t shader;
	shader.name = name;
	int index = vertexShaders.Append( shader );
	LoadVertexShader( index );
	currentVertexShader = index;

	// RB: removed idStr::Icmp( name, "heatHaze.vfp" ) == 0  hack
	// this requires r_useUniformArrays 1
	for( int i = 0; i < vertexShaders[index].uniforms.Num(); i++ )
	{
		if( vertexShaders[index].uniforms[i] == RENDERPARM_ENABLE_SKINNING )
		{
			vertexShaders[index].usesJoints = true;
			vertexShaders[index].optionalSkinning = true;
		}
	}
	// RB end

	return index;
}

/*
================================================================================================
idRenderProgManager::FindFragmentShader
================================================================================================
*/
int idRenderProgManager::FindFragmentShader( const char* name )
{
	for( int i = 0; i < fragmentShaders.Num(); i++ )
	{
		if( fragmentShaders[i].name.Icmp( name ) == 0 )
		{
			LoadFragmentShader( i );
			return i;
		}
	}
	fragmentShader_t shader;
	shader.name = name;
	int index = fragmentShaders.Append( shader );
	LoadFragmentShader( index );
	currentFragmentShader = index;
	return index;
}




/*
================================================================================================
idRenderProgManager::LoadVertexShader
================================================================================================
*/
void idRenderProgManager::LoadVertexShader( int index )
{
	if( vertexShaders[index].progId != INVALID_PROGID )
	{
		return; // Already loaded
	}

	vertexShader_t& vs = vertexShaders[index];
	vertexShaders[index].progId = ( GLuint ) LoadGLSLShader( GL_VERTEX_SHADER, vs.name, vs.nameOutSuffix, vs.shaderFeatures, vs.builtin, vs.uniforms );
}

/*
================================================================================================
idRenderProgManager::LoadFragmentShader
================================================================================================
*/
void idRenderProgManager::LoadFragmentShader( int index )
{
	if( fragmentShaders[index].progId != INVALID_PROGID )
	{
		return; // Already loaded
	}

	fragmentShader_t& fs = fragmentShaders[index];
	fragmentShaders[index].progId = ( GLuint ) LoadGLSLShader( GL_FRAGMENT_SHADER, fs.name, fs.nameOutSuffix, fs.shaderFeatures, fs.builtin, fs.uniforms );
}

/*
================================================================================================
idRenderProgManager::BindShader
================================================================================================
*/
// RB begin
void idRenderProgManager::BindShader( int progIndex, int vIndex, int fIndex, bool builtin )
{
	if( currentVertexShader == vIndex && currentFragmentShader == fIndex )
	{
		return;
	}

	if( builtin )
	{
		currentVertexShader = vIndex;
		currentFragmentShader = fIndex;

		// vIndex denotes the GLSL program
		if( vIndex >= 0 && vIndex < glslPrograms.Num() )
		{
			currentRenderProgram = vIndex;
			RENDERLOG_PRINTF( "Binding GLSL Program %s\n", glslPrograms[vIndex].name.c_str() );
			glUseProgram( glslPrograms[vIndex].progId );
		}
	}
	else
	{
		if( progIndex == -1 )
		{
			// RB: FIXME linear search
			for( int i = 0; i < glslPrograms.Num(); ++i )
			{
				if( ( glslPrograms[i].vertexShaderIndex == vIndex ) && ( glslPrograms[i].fragmentShaderIndex == fIndex ) )
				{
					progIndex = i;
					break;
				}
			}
		}

		currentVertexShader = vIndex;
		currentFragmentShader = fIndex;

		if( progIndex >= 0 && progIndex < glslPrograms.Num() )
		{
			currentRenderProgram = progIndex;
			RENDERLOG_PRINTF( "Binding GLSL Program %s\n", glslPrograms[progIndex].name.c_str() );
			glUseProgram( glslPrograms[progIndex].progId );
		}
	}
}
// RB end

/*
================================================================================================
idRenderProgManager::Unbind
================================================================================================
*/
void idRenderProgManager::Unbind()
{
	currentVertexShader = -1;
	currentFragmentShader = -1;

	glUseProgram( 0 );
}

// RB begin
bool idRenderProgManager::IsShaderBound() const
{
	return ( currentVertexShader != -1 );
}
// RB end

/*
================================================================================================
idRenderProgManager::SetRenderParms
================================================================================================
*/
void idRenderProgManager::SetRenderParms( renderParm_t rp, const float* value, int num )
{
	for( int i = 0; i < num; i++ )
	{
		SetRenderParm( ( renderParm_t )( rp + i ), value + ( i * 4 ) );
	}
}

/*
================================================================================================
idRenderProgManager::SetRenderParm
================================================================================================
*/
void idRenderProgManager::SetRenderParm( renderParm_t rp, const float* value )
{
	SetUniformValue( rp, value );
}


/*
========================
RpPrintState
========================
*/
#if 0
void RpPrintState( uint64 stateBits )
{

	// culling
	idLib::Printf( "Culling: " );
	switch( stateBits & GLS_CULL_BITS )
	{
		case GLS_CULL_FRONTSIDED:
			idLib::Printf( "FRONTSIDED -> BACK" );
			break;
		case GLS_CULL_BACKSIDED:
			idLib::Printf( "BACKSIDED -> FRONT" );
			break;
		case GLS_CULL_TWOSIDED:
			idLib::Printf( "TWOSIDED" );
			break;
		default:
			idLib::Printf( "NA" );
			break;
	}
	idLib::Printf( "\n" );

	// polygon mode
	idLib::Printf( "PolygonMode: %s\n", ( stateBits & GLS_POLYMODE_LINE ) ? "LINE" : "FILL" );

	// color mask
	idLib::Printf( "ColorMask: " );
	idLib::Printf( ( stateBits & GLS_REDMASK ) ? "_" : "R" );
	idLib::Printf( ( stateBits & GLS_GREENMASK ) ? "_" : "G" );
	idLib::Printf( ( stateBits & GLS_BLUEMASK ) ? "_" : "B" );
	idLib::Printf( ( stateBits & GLS_ALPHAMASK ) ? "_" : "A" );
	idLib::Printf( "\n" );

	// blend
	idLib::Printf( "Blend: src=" );
	switch( stateBits & GLS_SRCBLEND_BITS )
	{
		case GLS_SRCBLEND_ZERO:
			idLib::Printf( "ZERO" );
			break;
		case GLS_SRCBLEND_ONE:
			idLib::Printf( "ONE" );
			break;
		case GLS_SRCBLEND_DST_COLOR:
			idLib::Printf( "DST_COLOR" );
			break;
		case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
			idLib::Printf( "ONE_MINUS_DST_COLOR" );
			break;
		case GLS_SRCBLEND_SRC_ALPHA:
			idLib::Printf( "SRC_ALPHA" );
			break;
		case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
			idLib::Printf( "ONE_MINUS_SRC_ALPHA" );
			break;
		case GLS_SRCBLEND_DST_ALPHA:
			idLib::Printf( "DST_ALPHA" );
			break;
		case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
			idLib::Printf( "ONE_MINUS_DST_ALPHA" );
			break;
		default:
			idLib::Printf( "NA" );
			break;
	}
	idLib::Printf( ", dst=" );
	switch( stateBits & GLS_DSTBLEND_BITS )
	{
		case GLS_DSTBLEND_ZERO:
			idLib::Printf( "ZERO" );
			break;
		case GLS_DSTBLEND_ONE:
			idLib::Printf( "ONE" );
			break;
		case GLS_DSTBLEND_SRC_COLOR:
			idLib::Printf( "SRC_COLOR" );
			break;
		case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
			idLib::Printf( "ONE_MINUS_SRC_COLOR" );
			break;
		case GLS_DSTBLEND_SRC_ALPHA:
			idLib::Printf( "SRC_ALPHA" );
			break;
		case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
			idLib::Printf( "ONE_MINUS_SRC_ALPHA" );
			break;
		case GLS_DSTBLEND_DST_ALPHA:
			idLib::Printf( "DST_ALPHA" );
			break;
		case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
			idLib::Printf( "ONE_MINUS_DST_ALPHA" );
			break;
		default:
			idLib::Printf( "NA" );
	}
	idLib::Printf( "\n" );

	// depth func
	idLib::Printf( "DepthFunc: " );
	switch( stateBits & GLS_DEPTHFUNC_BITS )
	{
		case GLS_DEPTHFUNC_EQUAL:
			idLib::Printf( "EQUAL" );
			break;
		case GLS_DEPTHFUNC_ALWAYS:
			idLib::Printf( "ALWAYS" );
			break;
		case GLS_DEPTHFUNC_LESS:
			idLib::Printf( "LEQUAL" );
			break;
		case GLS_DEPTHFUNC_GREATER:
			idLib::Printf( "GEQUAL" );
			break;
		default:
			idLib::Printf( "NA" );
			break;
	}
	idLib::Printf( "\n" );

	// depth mask
	idLib::Printf( "DepthWrite: %s\n", ( stateBits & GLS_DEPTHMASK ) ? "FALSE" : "TRUE" );

	// depth bounds
	idLib::Printf( "DepthBounds: %s\n", ( stateBits & GLS_DEPTH_TEST_MASK ) ? "TRUE" : "FALSE" );

	// depth bias
	idLib::Printf( "DepthBias: %s\n", ( stateBits & GLS_POLYGON_OFFSET ) ? "TRUE" : "FALSE" );

	// stencil
	auto printStencil = [&]( stencilFace_t face, uint64 bits, uint64 mask, uint64 ref )
	{
		idLib::Printf( "Stencil: %s, ", ( bits & ( GLS_STENCIL_FUNC_BITS | GLS_STENCIL_OP_BITS ) ) ? "ON" : "OFF" );
		idLib::Printf( "Face=" );
		switch( face )
		{
			case STENCIL_FACE_FRONT:
				idLib::Printf( "FRONT" );
				break;
			case STENCIL_FACE_BACK:
				idLib::Printf( "BACK" );
				break;
			default:
				idLib::Printf( "BOTH" );
				break;
		}
		idLib::Printf( ", Func=" );
		switch( bits & GLS_STENCIL_FUNC_BITS )
		{
			case GLS_STENCIL_FUNC_NEVER:
				idLib::Printf( "NEVER" );
				break;
			case GLS_STENCIL_FUNC_LESS:
				idLib::Printf( "LESS" );
				break;
			case GLS_STENCIL_FUNC_EQUAL:
				idLib::Printf( "EQUAL" );
				break;
			case GLS_STENCIL_FUNC_LEQUAL:
				idLib::Printf( "LEQUAL" );
				break;
			case GLS_STENCIL_FUNC_GREATER:
				idLib::Printf( "GREATER" );
				break;
			case GLS_STENCIL_FUNC_NOTEQUAL:
				idLib::Printf( "NOTEQUAL" );
				break;
			case GLS_STENCIL_FUNC_GEQUAL:
				idLib::Printf( "GEQUAL" );
				break;
			case GLS_STENCIL_FUNC_ALWAYS:
				idLib::Printf( "ALWAYS" );
				break;
			default:
				idLib::Printf( "NA" );
				break;
		}
		idLib::Printf( ", OpFail=" );
		switch( bits & GLS_STENCIL_OP_FAIL_BITS )
		{
			case GLS_STENCIL_OP_FAIL_KEEP:
				idLib::Printf( "KEEP" );
				break;
			case GLS_STENCIL_OP_FAIL_ZERO:
				idLib::Printf( "ZERO" );
				break;
			case GLS_STENCIL_OP_FAIL_REPLACE:
				idLib::Printf( "REPLACE" );
				break;
			case GLS_STENCIL_OP_FAIL_INCR:
				idLib::Printf( "INCR" );
				break;
			case GLS_STENCIL_OP_FAIL_DECR:
				idLib::Printf( "DECR" );
				break;
			case GLS_STENCIL_OP_FAIL_INVERT:
				idLib::Printf( "INVERT" );
				break;
			case GLS_STENCIL_OP_FAIL_INCR_WRAP:
				idLib::Printf( "INCR_WRAP" );
				break;
			case GLS_STENCIL_OP_FAIL_DECR_WRAP:
				idLib::Printf( "DECR_WRAP" );
				break;
			default:
				idLib::Printf( "NA" );
				break;
		}
		idLib::Printf( ", ZFail=" );
		switch( bits & GLS_STENCIL_OP_ZFAIL_BITS )
		{
			case GLS_STENCIL_OP_ZFAIL_KEEP:
				idLib::Printf( "KEEP" );
				break;
			case GLS_STENCIL_OP_ZFAIL_ZERO:
				idLib::Printf( "ZERO" );
				break;
			case GLS_STENCIL_OP_ZFAIL_REPLACE:
				idLib::Printf( "REPLACE" );
				break;
			case GLS_STENCIL_OP_ZFAIL_INCR:
				idLib::Printf( "INCR" );
				break;
			case GLS_STENCIL_OP_ZFAIL_DECR:
				idLib::Printf( "DECR" );
				break;
			case GLS_STENCIL_OP_ZFAIL_INVERT:
				idLib::Printf( "INVERT" );
				break;
			case GLS_STENCIL_OP_ZFAIL_INCR_WRAP:
				idLib::Printf( "INCR_WRAP" );
				break;
			case GLS_STENCIL_OP_ZFAIL_DECR_WRAP:
				idLib::Printf( "DECR_WRAP" );
				break;
			default:
				idLib::Printf( "NA" );
				break;
		}
		idLib::Printf( ", OpPass=" );
		switch( bits & GLS_STENCIL_OP_PASS_BITS )
		{
			case GLS_STENCIL_OP_PASS_KEEP:
				idLib::Printf( "KEEP" );
				break;
			case GLS_STENCIL_OP_PASS_ZERO:
				idLib::Printf( "ZERO" );
				break;
			case GLS_STENCIL_OP_PASS_REPLACE:
				idLib::Printf( "REPLACE" );
				break;
			case GLS_STENCIL_OP_PASS_INCR:
				idLib::Printf( "INCR" );
				break;
			case GLS_STENCIL_OP_PASS_DECR:
				idLib::Printf( "DECR" );
				break;
			case GLS_STENCIL_OP_PASS_INVERT:
				idLib::Printf( "INVERT" );
				break;
			case GLS_STENCIL_OP_PASS_INCR_WRAP:
				idLib::Printf( "INCR_WRAP" );
				break;
			case GLS_STENCIL_OP_PASS_DECR_WRAP:
				idLib::Printf( "DECR_WRAP" );
				break;
			default:
				idLib::Printf( "NA" );
				break;
		}
		idLib::Printf( ", mask=%llu, ref=%llu\n", mask, ref );
	};

	uint32 mask = uint32( ( stateBits & GLS_STENCIL_FUNC_MASK_BITS ) >> GLS_STENCIL_FUNC_MASK_SHIFT );
	uint32 ref = uint32( ( stateBits & GLS_STENCIL_FUNC_REF_BITS ) >> GLS_STENCIL_FUNC_REF_SHIFT );
	if( stateBits & GLS_SEPARATE_STENCIL )
	{
		printStencil( STENCIL_FACE_FRONT, ( stateBits & GLS_STENCIL_FRONT_OPS ), mask, ref );
		printStencil( STENCIL_FACE_BACK, ( ( stateBits & GLS_STENCIL_BACK_OPS ) >> 12 ), mask, ref );
	}
	else
	{
		printStencil( STENCIL_FACE_NUM, stateBits, mask, ref );
	}
}
#endif