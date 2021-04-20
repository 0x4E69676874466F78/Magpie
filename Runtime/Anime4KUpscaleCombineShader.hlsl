// ��ͨ�� Denoise �汾�� combine ��ɫ��
// ��ֲ�� https://github.com/bloc97/Anime4K/blob/master/glsl/Upscale/Anime4K_Upscale_CNN_M_x2.glsl


cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0);
};


#define D2D_INPUT_COUNT 2
#define D2D_INPUT0_COMPLEX
#define D2D_INPUT1_COMPLEX
#define MAGPIE_USE_YUV
#include "Anime4K.hlsli"


D2D_PS_ENTRY(main) {
	float4 coord0 = D2DGetInputCoordinate(0);
	coord0.xy /= float2(2,2);	// �� dest ����ӳ��Ϊ src ����
	float4 coord1 = D2DGetInputCoordinate(1);
	coord1.xy /= float2(2, 2);	// �� dest ����ӳ��Ϊ src ����

	float2 f = frac(coord1.xy / coord1.zw);
	// ��ȡ�������֣����ʹ�� round ���� bug����Ϊ����������� f ���ܵ��� 0.75
	int2 i = int2(f * 2);
	float l = Uncompress(D2DSampleInput(1, coord1.xy + (float2(0.5, 0.5) - f) * coord1.zw))[i.y * 2 + i.x];

	float3 yuv = RGB2YUV(D2DSampleInput(0, coord0.xy).xyz);
	return float4(YUV2RGB(yuv.x+l, yuv.y, yuv.z), 1);
}
