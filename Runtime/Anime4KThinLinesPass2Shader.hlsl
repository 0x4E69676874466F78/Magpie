// Anime4K-v3.1-ThinLines-Kernel(Y)
// ��ֲ�� https://github.com/bloc97/Anime4K/blob/master/glsl/Experimental-Effects/Anime4K_ThinLines_HQ.glsl



cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0);
};


#define MAGPIE_INPUT_COUNT 1
#include "Anime4K.hlsli"


D2D_PS_ENTRY(main) {
	InitMagpieSampleInput();

	float2 t1 = SampleInputOffChecked(0, float2(0, -1)).xy * 5 - 1;
	float2 t2 = SampleInputOffChecked(0, float2(0, 1)).xy * 5 - 1;

	//[tl  t tr]
	//[ l cc  r]
	//[bl  b br]
	float tx = t1.x;
	float cx = SampleInputCur(0).x * 5 - 1;
	float bx = t2.x;


	float ty = t1.y;
	//float cy = LUMAD_tex(HOOKED_pos).y;
	float by = t2.y;


	//Horizontal Gradient
	//[-1  0  1]
	//[-2  0  2]
	//[-1  0  1]
	float xgrad = (tx + cx + cx + bx) / 8.0;

	//Vertical Gradient
	//[-1 -2 -1]
	//[ 0  0  0]
	//[ 1  2  1]
	float ygrad = (-ty + by) / 8.0;

	//Computes the luminance's gradient
	float norm = sqrt(xgrad * xgrad + ygrad * ygrad);
	float a = pow(norm, 0.7);

	return float4(pow(norm, 0.7) / 2, 0, 0, 1);
}
