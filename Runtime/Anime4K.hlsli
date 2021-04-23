#include "common.hlsli"

// ������Զ���ֵ�и 0~1��Ϊ��ʹֵ����ɫ��֮�䴫����Ҫ����ѹ��
// ��������ֵ�� -1.5~1.5 ֮�䣬���ѧϰ���м�ֵһ���С�����Լ���û�нض���ʧ
// ��������ʧ���ɱ��⣬���ջ��������
#define Compress(value) ((value + 1.5) / 3)	// (atan(value) / PI + 0.5);

#define Uncompress(value) (value * 3 - 1.5)	// tan((value - 0.5) * PI)

// Anime4K ��ʹ�õ������ȷ���
#define GetLuma(rgb) (0.299 * rgb.r + 0.587 * rgb.g + 0.114 * rgb.b)
