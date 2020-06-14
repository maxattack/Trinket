#include "common.fxh"

void main(in WireframeVSInput Input, out WireframePSInput Output) {
	Output.Pos = mul(g_ModelViewProjection, float4(Input.Pos,1.0));
	Output.Color = Input.Color;
}
