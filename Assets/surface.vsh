#include "common.fxh"

void main(in VSInput Input, out PSInput Ouput) {
    Ouput.Pos   = mul( g_ModelViewProjection, float4(Input.Pos,1.0) );

    float4 worldPos = mul( g_ModelTransform, float4(Input.Pos,1.0) );
    float4 ShadowMapPos = mul( g_WorldToShadowMapUVDepth, worldPos);
    Ouput.ShadowMapPos = ShadowMapPos.xyz / ShadowMapPos.w;

    float3 Normal = mul(g_NormalTransform, float4(Input.Normal, 0.0) ).xyz;
    Ouput.NdotL = saturate(dot(Normal, -g_LightDirection.xyz));

    Ouput.UV  = Input.UV;

    Ouput.Color = Input.Color;

    float viewPosZ = mul( g_ModelViewTransform, float4(Input.Pos,1.0) ).z;
    float fogStart = 12.5;
    float fogEnd = 25.0;
    Ouput.FogFactor = saturate((fogEnd - viewPosZ) / (fogEnd - fogStart));
}
