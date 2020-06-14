
struct VSInput {
    float3 Pos    : ATTRIB0;
    float3 Normal : ATTRIB1;
    float2 UV     : ATTRIB2;
    float4 Color  : ATTRIB3;
};

struct PSInput { 
    float4 Pos          : SV_POSITION; 
    float3 ShadowMapPos : SHADOW_MAP_POS;
    float2 UV           : TEX_COORD;
    float4 Color        : COLOR0; 
    float NdotL         : N_DOT_L;
    float FogFactor     : FOG;
};

struct PSOutput {
    float4 Color : SV_TARGET;
};

struct DebugPSInput {
    float4 Pos : SV_POSITION;
    float2 UV  : TEX_COORD;
};

cbuffer Constants {
    float4x4 g_ModelViewProjection;
    float4x4 g_ModelViewTransform;
    float4x4 g_ModelTransform;
    float4x4 g_NormalTransform;
    float4x4 g_WorldToShadowMapUVDepth;
    float4   g_LightDirection;    
};

struct WireframeVSInput {
    float3 Pos : ATTRIB0;
    float4 Color : ATTRIB1;
};

struct WireframePSInput {
    float4 Pos : SV_POSITION;
    float4 Color : COLOR0;
};

