
struct SurfaceVSInput {
    float3 Pos    : ATTRIB0;
    float3 Normal : ATTRIB1;
    float4 Color  : ATTRIB2;
    float2 UV     : ATTRIB3;
};

struct SurfacePSInput { 
    float4 Pos          : SV_POSITION; 
    float3 ShadowMapPos : SHADOW_MAP_POS;
    float2 UV           : TEX_COORD;
    float4 Color        : COLOR0; 
    float NdotL         : N_DOT_L;
    float FogFactor     : FOG;
};

struct DebugPSInput
{
    float4 Pos : SV_POSITION;
    float2 UV  : TEX_COORD;
};
