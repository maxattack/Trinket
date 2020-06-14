#include "common.fxh"

float4 main(in VSInput Input) : SV_POSITION {
    return mul(g_ModelViewProjection, float4(Input.Pos,1.0));
}
