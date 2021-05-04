#pragma once
#include "pch.h"
#include "D2DContext.h"


enum class CaptureredFrameType {
	D2DImage, WICBitmap
};

enum class CaptureStyle {
	Normal, Event
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
	
	// CaptureStyle Ϊ Event ʱʹ�ã�ע��ص���֪ͨһ֡����
	virtual void On(std::function<void()> cb) {}

	// �����֡������
	virtual CaptureredFrameType GetFrameType() = 0;

	// ������Normal ��ʾ��������Event ��ʾ�ȴ�һ֡����
	virtual CaptureStyle GetCaptureStyle() = 0;
};
