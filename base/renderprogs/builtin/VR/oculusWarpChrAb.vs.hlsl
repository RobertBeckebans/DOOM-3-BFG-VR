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

#include "renderprogs/global.inc"

struct VS_IN {
	float2 pos : POS;
	float2 texr : TEXR;
	float2 texg : TEXG;
	float2 texb : TEXB;
	float4 dcolor : DCOLOR;
};

struct VS_OUT {
	float4 color : COLOR0;
	float4 position : POSITION;
	float4 texcoord0 : TEXCOORD0;
	float4 texcoord1 : TEXCOORD1;
	float4 texcoord2 : TEXCOORD2;
};

void main( VS_IN vertex, out VS_OUT result ) {
	
	result.position.x = vertex.pos.x;
	result.position.y = vertex.pos.y;
	result.position.z = 0.5;
	result.position.w = 1.0;
	
	// Vertex inputs are in TanEyeAngle space for the R,G,B channels (i.e. after chromatic aberration and distortion).
	// Scale them into the correct [0-1],[0-1] UV lookup space (depending on eye)

	float2 tresult = vertex.texr * rpEyeToSourceUVScale.xy + rpEyeToSourceUVOffset.xy;
	result.texcoord0 = float4 ( tresult.x, tresult.y, 0.0, 0.0 );
	result.texcoord0.y = 1.0 - result.texcoord0.y;

	tresult = vertex.texg * rpEyeToSourceUVScale.xy + rpEyeToSourceUVOffset.xy;
	result.texcoord1 = float4 ( tresult.x, tresult.y, 0.0, 0.0 );
	result.texcoord1.y = 1.0 - result.texcoord1.y;

	tresult = vertex.texb * rpEyeToSourceUVScale.xy + rpEyeToSourceUVOffset.xy;
	result.texcoord2 = float4 ( tresult.x, tresult.y, 0.0, 0.0 );
	result.texcoord2.y = 1.0 - result.texcoord2.y;

	result.color = vertex.dcolor;
}