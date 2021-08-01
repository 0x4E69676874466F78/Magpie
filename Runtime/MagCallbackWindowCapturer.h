#pragma once
#include "pch.h"
#include "WindowCapturerBase.h"
#include "Utils.h"
#include "Env.h"


// 使用 Magnification API 抓取窗口
// 见 https://docs.microsoft.com/en-us/previous-versions/windows/desktop/magapi/magapi-intro
// 不使用，此API已过时，且速度很慢
class MagCallbackWindowCapturer : public WindowCapturerBase {
public:
	MagCallbackWindowCapturer() {
		if (_instance) {
			Debug::Assert(false, L"已存在 MagCallbackWindowCapturer 实例");
		}

		Debug::ThrowIfWin32Failed(
			MagInitialize(),
			L"MagInitialize 失败"
		);

		// 创建不可见的放大镜控件
		// 大小为目标窗口客户区
		SIZE srcClientSize = Utils::GetSize(Env::$instance->GetSrcClient());

		_hwndMag = CreateWindow(
			WC_MAGNIFIER,
			L"MagnifierWindow",
			WS_CHILD,
			0, 0,
			srcClientSize.cx, srcClientSize.cy,
			Env::$instance->GetHwndHost(),
			NULL,
			Env::$instance->GetHInstance(),
			NULL
		);
		Debug::ThrowIfWin32Failed(_hwndMag, L"创建放大镜控件失败");

		Debug::ThrowIfWin32Failed(
			MagSetImageScalingCallback(_hwndMag, &_ImageScalingCallback),
			L"设置放大镜回调失败"
		);

		_instance = this;
	}

	~MagCallbackWindowCapturer() {
		MagUninitialize();

		_instance = nullptr;
	}

	ComPtr<IUnknown> GetFrame() override {
		// MagSetWindowSource 是同步执行的
		if (!MagSetWindowSource(_hwndMag, Env::$instance->GetSrcClient())) {
			Debug::WriteErrorMessage(L"MagSetWindowSource 失败");
		}

		return _bmp;
	}

	CaptureredFrameType GetFrameType() override {
		return CaptureredFrameType::D2DImage;
	}
private:
	static BOOL CALLBACK _ImageScalingCallback(
		HWND hWnd,
		void* srcdata,
		MAGIMAGEHEADER srcheader,
		void* destdata,
		MAGIMAGEHEADER destheader,
		RECT unclipped,
		RECT clipped,
		HRGN dirty
	) {
		if (!_instance) {
			return FALSE;
		}

		if (srcheader.cbSize / srcheader.width / srcheader.height != 4) {
			Debug::WriteErrorMessage(L"srcdata 不是BGRA格式");
			return FALSE;
		}
		
		Env::$instance->GetD2DDC()->CreateBitmap(
			{ srcheader.width, srcheader.height },
			BitmapProperties(PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)),
			& _instance->_bmp
		);
		D2D1_RECT_U destRect = { 0, 0, srcheader.width, srcheader.height };
		_instance->_bmp->CopyFromMemory(&destRect, srcdata, srcheader.stride);

		return TRUE;
	}

	HWND _hwndMag = NULL;

	ComPtr<ID2D1Bitmap> _bmp = nullptr;

	static MagCallbackWindowCapturer* _instance;
};
