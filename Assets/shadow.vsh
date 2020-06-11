#include "common.fxh"

cbuffer Constants {
    float4x4 g_ModelViewProjection;
};

struct PSInput { 
    float4 Pos   : SV_POSITION; 
};

void main(in SurfaceVSInput VSIn, out PSInput PSIn) {
    PSIn.Pos = mul(g_ModelViewProjection, float4(VSIn.Pos,1.0));
}
