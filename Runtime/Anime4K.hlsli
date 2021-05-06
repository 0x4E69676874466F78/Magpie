#include "common.hlsli"

// ������Զ���ֵ�и 0~1��Ϊ��ʹֵ����ɫ��֮�䴫����Ҫ����ѹ��
// ��Ϊ������ʧ�����ջ��������
#define Compress(value) (((value) + 3) / 6)
#define Uncompress(value) ((value) * 6 - 3)

// ��ʱֵ�� -1~1 ֮�䣬ʹ�������ѹ���㷨���پ�����ʧ
#define Compress1(value) (((value) + 1) / 2)
#define Uncompress1(value) ((value) * 2 - 1)

// �޷�Χ���Ƶ�ѹ�������ٶȽ���
#define Compress2(value) (atan(value) / PI + 0.5)
#define Uncompress2(value) tan(((value) - 0.5) * PI)

#define Compress3(value) (((value) + 10) / 20)
#define Uncompress3(value) ((value) * 20 - 10)

// Anime4K ��ʹ�õ������ȷ���
#define GetLuma(rgb) (0.299 * (rgb).r + 0.587 * (rgb).g + 0.114 * (rgb).b)
