#include "common.fxh"

cbuffer Constants {
    float4x4 g_ModelViewProjection;
    float4x4 g_ModelViewTransform;
    float4x4 g_ModelTransform;
    float4x4 g_NormalTransform;
    float4x4 g_WorldToShadowMapUVDepth;
    float4   g_LightDirection;    
};


void main(
    in SurfaceVSInput VSIn, 
    out SurfacePSInput PSIn
) {
    PSIn.Pos   = mul( g_ModelViewProjection, float4(VSIn.Pos,1.0) );

    float4 worldPos = mul( g_ModelTransform, float4(VSIn.Pos,1.0) );
    float4 ShadowMapPos = mul( g_WorldToShadowMapUVDepth, worldPos);
    PSIn.ShadowMapPos = ShadowMapPos.xyz / ShadowMapPos.w;

    float3 Normal = mul(g_NormalTransform, float4(VSIn.Normal, 0.0) ).xyz;
    PSIn.NdotL = saturate(dot(Normal, -g_LightDirection.xyz));

    PSIn.UV  = VSIn.UV;

    PSIn.Color = VSIn.Color;

    float viewPosZ = mul( g_ModelViewTransform, float4(VSIn.Pos,1.0) ).z;
    float fogStart = 12.5;
    float fogEnd = 25.0;
    PSIn.FogFactor = saturate((fogEnd - viewPosZ) / (fogEnd - fogStart));
}
