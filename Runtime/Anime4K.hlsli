#include "common.hlsli"


// ����ֵ��-3~-3����Ϊ������ʧ�����ջ��������
#define Compress(value) compressLinear(value, -3, 3)
#define Uncompress(value) uncompressLinear(value, -3, 3)
