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

// *INDENT-OFF*
uniform sampler2D samp0 : register(s0); 
uniform sampler2D samp1 : register(s1); 

struct PS_IN 
{
//	float4 position : POSITION;
	float4 color : COLOR0;
	float4 texcoord0 : TEXCOORD0;
};

struct PS_OUT 
{	
	float4 color : COLOR;
};
// *INDENT-ON*

void main( PS_IN fragment, out PS_OUT result ) {
	float4 outColor = tex2Dlod( samp0, fragment.texcoord0 ) * fragment.color;
	outColor.a = 1.0;

	if ( rpOverdriveScales.x > 0.0 )
	{
		float2 frag = fragment.position.xy * 0.5 + 0.5;
		float4 frag2 = float4(frag, 0.0, 0.0);
		float3 oldColor = tex2Dlod( samp1, frag2 ).rgb;
		float3 newColor = outColor.rgb;
		float3 adjustedScales;

		adjustedScales.x = newColor.x > oldColor.x ? rpOverdriveScales.x : rpOverdriveScales.y;
		adjustedScales.y = newColor.y > oldColor.y ? rpOverdriveScales.x : rpOverdriveScales.y;
		adjustedScales.z = newColor.z > oldColor.z ? rpOverdriveScales.x : rpOverdriveScales.y;
		float3 overdriveColor = clamp(newColor + (newColor - oldColor) * adjustedScales, 0.0, 1.0);
		outColor = vec4(overdriveColor, 1.0);
	}
	result.color = outColor;

}