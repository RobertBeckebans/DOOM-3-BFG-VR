/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013 Robert Beckebans

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

#ifndef __RENDERER_BACKEND_H__
#define __RENDERER_BACKEND_H__

#include "RenderLog.h"

bool			GL_CheckErrors_( const char* filename, int line );
#if 1 // !defined(RETAIL)
	#define         GL_CheckErrors()	GL_CheckErrors_(__FILE__, __LINE__)
#else
	#define         GL_CheckErrors()	false
#endif
// RB end

struct tmu_t
{
	unsigned int	current2DMap;
	unsigned int	current2DArray;
	unsigned int	currentCubeMap;
};

const int MAX_MULTITEXTURE_UNITS =	8;

struct glState_t
{
	tmu_t				tmu[ MAX_MULTITEXTURE_UNITS ];
	int					currenttmu;

	// RB: 64 bit fixes, changed unsigned int to uintptr_t
	uintptr_t			currentVertexBuffer;
	uintptr_t			currentIndexBuffer;

	Framebuffer*		currentFramebuffer;		// RB: for offscreen rendering
	// RB end

	int					faceCulling;

	vertexLayoutType_t	vertexLayout;

	float				polyOfsScale;
	float				polyOfsBias;

	uint64				frameCounter;
	uint32				frameParity;

	// for GL_TIME_ELAPSED_EXT queries
	GLuint				renderLogMainBlockTimeQueryIds[ NUM_FRAME_DATA ][ MRB_TOTAL_QUERIES ];
	uint32				renderLogMainBlockTimeQueryIssued[ NUM_FRAME_DATA ][ MRB_TOTAL_QUERIES ];
};

/*
===========================================================================

idRenderBackend

all state modified by the back end is separated from the front end state

===========================================================================
*/

class idRenderBackend
{
	friend class Framebuffer;
	friend class idImage;

public:
	idRenderBackend();
	~idRenderBackend();

	void				Init();
	void				Shutdown();

	void				ExecuteBackEndCommands( const emptyCommand_t* cmds );
	void				StereoRenderExecuteBackEndCommands( const emptyCommand_t* const allCmds );
	void				GL_BlockingSwapBuffers();

	void				Print();
	void				CheckCVars();

	void				DrawElementsWithCounters( const drawSurf_t* surf );
private:
	void				DrawFlickerBox();

	void				DrawStencilShadowPass( const drawSurf_t* drawSurf, const bool renderZPass );
	void				DrawViewInternal( const viewDef_t* viewDef, const int stereoEye );
	void				DrawView( const void* data, const int stereoEye );
	void				CopyRender( const void* data );

	void				BindVariableStageImage( const textureStage_t* texture, const float* shaderRegisters );
	void				PrepareStageTexturing( const shaderStage_t* pStage, const drawSurf_t* surf );
	void				FinishStageTexturing( const shaderStage_t* pStage, const drawSurf_t* surf );

	void				ResetViewportAndScissorToDefaultCamera( const viewDef_t* _viewDef );

	void				FillDepthBufferGeneric( const drawSurf_t* const* drawSurfs, int numDrawSurfs );
	void				FillDepthBufferFast( drawSurf_t** drawSurfs, int numDrawSurfs );

	void				T_BlendLight( const drawSurf_t* drawSurfs, const viewLight_t* vLight );
	void				BlendLight( const drawSurf_t* drawSurfs, const drawSurf_t* drawSurfs2, const viewLight_t* vLight );
	void				T_BasicFog( const drawSurf_t* drawSurfs, const idPlane fogPlanes[ 4 ], const idRenderMatrix* inverseBaseLightProject );
	void				FogPass( const drawSurf_t* drawSurfs,  const drawSurf_t* drawSurfs2, const viewLight_t* vLight );
	void				FogAllLights();

	void				SetupInteractionStage( const shaderStage_t* surfaceStage, const float* surfaceRegs, const float lightColor[4],
			idVec4 matrix[2], float color[4] );

	void				DrawInteractions( const viewDef_t* _viewDef );
	void				DrawSingleInteraction( drawInteraction_t* din, bool useFastPath, bool useIBL, bool setInteractionShader );
	int					DrawShaderPasses( const drawSurf_t* const* const drawSurfs, const int numDrawSurfs,
										  const float guiStereoScreenOffset, const int stereoEye );

	void				RenderInteractions( const drawSurf_t* surfList, const viewLight_t* vLight, int depthFunc, bool performStencilTest, bool useLightDepthBounds );

	// RB
	void				ShadowMapPass( const drawSurf_t* drawSurfs, const viewLight_t* vLight, int side );
	void				StencilShadowPass( const drawSurf_t* drawSurfs, const viewLight_t* vLight );
	void				StencilSelectLight( const viewLight_t* vLight );
	// Experimental feature
	void				MotionBlur();
	void				PostProcess( const void* data );
private:
	void				GL_StartFrame();
	void				GL_EndFrame();

public:
	uint64				GL_GetCurrentState() const;
private:
	uint64				GL_GetCurrentStateMinusStencil() const;
	void				GL_SetDefaultState();

	void				GL_State( uint64 stateBits, bool forceGlState = false );

	void				GL_Cull( int cullType );
	void				GL_DepthBoundsTest( const float zmin, const float zmax );
	void				GL_PolygonOffset( float scale, float bias );

public:
	void				GL_SelectTexture( int unit );

	void				GL_Scissor( int x /* left*/, int y /* bottom */, int w, int h );
	void				GL_Viewport( int x /* left */, int y /* bottom */, int w, int h );

	ID_INLINE void		GL_Scissor( const idScreenRect& rect )
	{
		GL_Scissor( rect.x1, rect.y1, rect.x2 - rect.x1 + 1, rect.y2 - rect.y1 + 1 );
	}
	ID_INLINE void		GL_Viewport( const idScreenRect& rect )
	{
		GL_Viewport( rect.x1, rect.y1, rect.x2 - rect.x1 + 1, rect.y2 - rect.y1 + 1 );
	}

	ID_INLINE void	GL_ViewportAndScissor( int x, int y, int w, int h )
	{
		GL_Viewport( x, y, w, h );
		GL_Scissor( x, y, w, h );
	}

	ID_INLINE void	GL_ViewportAndScissor( const idScreenRect& rect )
	{
		GL_Viewport( rect );
		GL_Scissor( rect );
	}

	void				GL_Clear( bool color, bool depth, bool stencil, byte stencilValue, float r, float g, float b, float a );

	void				GL_Color( float r, float g, float b, float a );
	ID_INLINE void		GL_Color( float r, float g, float b )
	{
		GL_Color( r, g, b, 1.0f );
	}

	ID_INLINE void		GL_Color( const idVec3& color )
	{
		GL_Color( color[0], color[1], color[2], 1.0f );
	}

	ID_INLINE void		GL_Color( const idVec4& color )
	{
		GL_Color( color[0], color[1], color[2], color[3] );
	}

	void				SetBuffer( const void* data );
private:

public:
	backEndCounters_t	pc;

	// surfaces used for code-based drawing
	drawSurf_t			unitSquareSurface;
	drawSurf_t			zeroOneCubeSurface;
	drawSurf_t			zeroOneSphereSurface; // RB
	drawSurf_t			testImageSurface;
	drawSurf_t			hudSurface; // Koz hud mesh

private:
	const viewDef_t*	viewDef;

	const viewEntity_t*	currentSpace;			// for detecting when a matrix must change
	idScreenRect		currentScissor;			// for scissor clipping, local inside renderView viewport

	bool				currentRenderCopied;	// true if any material has already referenced _currentRender

	idRenderMatrix		prevMVP[2];				// world MVP from previous frame for motion blur, per-eye

	// RB begin
	idRenderMatrix		shadowV[6];				// shadow depth view matrix
	idRenderMatrix		shadowP[6];				// shadow depth projection matrix
	// RB end

private:
	uint64				glStateBits;			// for all render APIs

public:
	glState_t			glState;				// for OpenGL state deltas

	int					GetCurrentTextureUnit() const
	{
		return glState.currenttmu;
	}
};


class idImage;
//class idTriangles;
class idRenderModelSurface;
class idDeclRenderProg;
class idRenderTexture;










#endif
