#include "common.hlsli"

// ��Ϊ d2d �Ὣ��ɫ���䴫�ݵ�ֵ���Ƶ� [0, 1]��pass1 ���ݵ� pass2 ��ֵ��Ҫ����ѹ��
// ��������ֵ�� 0~32 ��
#define Compress(value) (value / 32)	// (atan(value) / PI + 0.5)
#define Uncompress(value) (value * 32)	// (tan((value - 0.5) * PI))
