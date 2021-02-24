#include "common.hlsli"

// ������Զ���ֵ�и 0~1��Ϊ��ʹֵ����ɫ��֮�䴫����Ҫ����ѹ��
// ��������ֵ�� -2~2 ֮�䣬���ѧϰ���м�ֵһ���С�����Լ���û����ʧ
// ʹ�� tan ����ѹ������Ϊ������ʧ��������
#define Compress(value) ((value + 2) / 4)	// (atan(value) / PI + 0.5);

#define Uncompress(value) (value * 4 - 2)	// tan((value - 0.5) * PI)

// Anime4K ��ʹ�õ������ȷ���
#define GetLuma(rgb) (0.299 * rgb.r + 0.587 * rgb.g + 0.114 * rgb.b)
