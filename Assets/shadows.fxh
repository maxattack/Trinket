Texture2D              g_ShadowMap;
SamplerComparisonState g_ShadowMap_sampler;

float ComputeShadowAmount(float3 shadowMapPos) {
	// Filter with PCF to get smooth shadows

	float cmp = max(shadowMapPos.z, 1e-7);
	//float LightAmount = g_ShadowMap.SampleCmp(g_ShadowMap_sampler, shadowMapPos.xy, cmp);

	float2 shadowMapSize = float2(2048, 2048);
	float2 uv = shadowMapPos.xy * shadowMapSize; // 1 unit - 1 texel
	float2 shadowMapSizeInv = 1.0 / shadowMapSize;
	float2 base_uv;
	base_uv.x = floor(uv.x + 0.5);
	base_uv.y = floor(uv.y + 0.5);
	float s = (uv.x + 0.5 - base_uv.x);
	float t = (uv.y + 0.5 - base_uv.y);
	base_uv -= float2(0.5, 0.5);
	base_uv *= shadowMapSizeInv;
	float sum = 0;

    // filter size = 3
	// float uw0 = (3 - 2 * s);
	// float uw1 = (1 + 2 * s);
	// float u0 = (2 - s) / uw0 - 1;
	// float u1 = s / uw1 + 1;
	// float vw0 = (3 - 2 * t);
	// float vw1 = (1 + 2 * t);
	// float v0 = (2 - t) / vw0 - 1;
	// float v1 = t / vw1 + 1;    
	// sum += uw0 * vw0 * g_ShadowMap.SampleCmp(g_ShadowMap_sampler, base_uv+float2(u0,v0)*shadowMapSizeInv, cmp);
	// sum += uw1 * vw0 * g_ShadowMap.SampleCmp(g_ShadowMap_sampler, base_uv+float2(u1,v0)*shadowMapSizeInv, cmp);
	// sum += uw0 * vw1 * g_ShadowMap.SampleCmp(g_ShadowMap_sampler, base_uv+float2(u0,v1)*shadowMapSizeInv, cmp);
	// sum += uw1 * vw1 * g_ShadowMap.SampleCmp(g_ShadowMap_sampler, base_uv+float2(u1,v1)*shadowMapSizeInv, cmp);
	// return sum * 1.0f / 16;

    // filter size = 5
	float uw0 = (4 - 3 * s);
	float uw1 = 7;
	float uw2 = (1 + 3 * s);
	float u0 = (3 - 2 * s) / uw0 - 2;
	float u1 = (3 + s) / uw1;
	float u2 = s / uw2 + 2;
	float vw0 = (4 - 3 * t);
	float vw1 = 7;
	float vw2 = (1 + 3 * t);
	float v0 = (3 - 2 * t) / vw0 - 2;
	float v1 = (3 + t) / vw1;
	float v2 = t / vw2 + 2;
	sum += uw0 * vw0 * g_ShadowMap.SampleCmp(g_ShadowMap_sampler, base_uv+float2(u0,v0)*shadowMapSizeInv, cmp);
	sum += uw1 * vw0 * g_ShadowMap.SampleCmp(g_ShadowMap_sampler, base_uv+float2(u1,v0)*shadowMapSizeInv, cmp);
	sum += uw2 * vw0 * g_ShadowMap.SampleCmp(g_ShadowMap_sampler, base_uv+float2(u2,v0)*shadowMapSizeInv, cmp);
	sum += uw0 * vw1 * g_ShadowMap.SampleCmp(g_ShadowMap_sampler, base_uv+float2(u0,v1)*shadowMapSizeInv, cmp);
	sum += uw1 * vw1 * g_ShadowMap.SampleCmp(g_ShadowMap_sampler, base_uv+float2(u1,v1)*shadowMapSizeInv, cmp);
	sum += uw2 * vw1 * g_ShadowMap.SampleCmp(g_ShadowMap_sampler, base_uv+float2(u2,v1)*shadowMapSizeInv, cmp);
	sum += uw0 * vw2 * g_ShadowMap.SampleCmp(g_ShadowMap_sampler, base_uv+float2(u0,v2)*shadowMapSizeInv, cmp);
	sum += uw1 * vw2 * g_ShadowMap.SampleCmp(g_ShadowMap_sampler, base_uv+float2(u1,v2)*shadowMapSizeInv, cmp);
	sum += uw2 * vw2 * g_ShadowMap.SampleCmp(g_ShadowMap_sampler, base_uv+float2(u2,v2)*shadowMapSizeInv, cmp);
	return sum * 1.0f / 144;

}