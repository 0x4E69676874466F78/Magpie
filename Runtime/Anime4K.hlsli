#include "common.hlsli"

// ������Զ���ֵ�и 0~1��Ϊ��ʹֵ����ɫ��֮�䴫����Ҫ����ѹ��
// ��������ֵ�� -2~2 ֮�䣬���ѧϰ���м�ֵһ���С�����Լ���û�нض���ʧ
// ��������ʧ���ɱ��⣬���ջ��������
#define Compress(value) (((value) + 2) / 4)	// (atan(value) / PI + 0.5);
#define Uncompress(value) ((value) * 4 - 2)	// tan((value - 0.5) * PI)

// ��ʱֵ�� -1~1 ֮�䣬ʹ�������ѹ���㷨���پ�����ʧ
#define Compress1(value) (((value) + 1) / 2)
#define Uncompress1(value) ((value) * 2 - 1)

// Anime4K ��ʹ�õ������ȷ���
#define GetLuma(rgb) (0.299 * (rgb).r + 0.587 * (rgb).g + 0.114 * (rgb).b)
