#pragma once
#include "pch.h"
#include "D2DContext.h"


enum class CaptureredFrameType {
	D2DImage, WICBitmap
};


// �������͵� WindowCapturer �Ļ���
class WindowCapturerBase {
public:
	WindowCapturerBase() {}

	virtual ~WindowCapturerBase() {}

	// ���ɸ��ƣ������ƶ�
	WindowCapturerBase(const WindowCapturerBase&) = delete;
	WindowCapturerBase(WindowCapturerBase&&) = delete;

	// ����һ֡
	virtual ComPtr<IUnknown> GetFrame() = 0;

	// �е�ʵ���޷���֪ʲôʱ������µ�֡��������ǻ�����֡�����ʱ���Զ�������Ⱦ��Ϣ
	virtual bool IsAutoRender() = 0;

	// �����֡������
	virtual CaptureredFrameType GetFrameType() = 0;
};
