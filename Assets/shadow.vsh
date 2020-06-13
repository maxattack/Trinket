#include "common.fxh"

// For the shadow map, we only require the vertex position

float4 main(in VSInput Input) : SV_POSITION {
    return mul(g_ModelViewProjection, float4(Input.Pos,1.0));
}
