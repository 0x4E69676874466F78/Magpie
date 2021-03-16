#pragma once
#include "pch.h"
#include "WindowCapturerBase.h"
#include "Utils.h"


// ʹ�� GDI ץȡ����
class GDIWindowCapturer : public WindowCapturerBase {
public:
	GDIWindowCapturer(
		HWND hwndSrc,
		const RECT& srcRect,
		IWICImagingFactory2* wicImgFactory,
		bool useBitblt = false
	): _wicImgFactory(wicImgFactory), _srcRect(srcRect), _hwndSrc(hwndSrc), _useBitblt(useBitblt) {
	}

	ComPtr<IWICBitmapSource> GetFrame() override {
		if (_useBitblt) {
			return _GetFrameWithBitblt();
		} else {
			return _GetFrameWithNoBitblt();
		}
	}

private:
	ComPtr<IWICBitmapSource> _GetFrameWithNoBitblt() {
		SIZE srcSize = Utils::GetSize(_srcRect);
		RECT windowRect{};
		GetWindowRect(_hwndSrc, &windowRect);

		HDC hdcSrc = GetDC(_hwndSrc);
		// ֱ�ӻ�ȡҪ�Ŵ󴰿ڵ� DC ������ HBITMAP���������Լ�����һ��
		HBITMAP hBmpDest = (HBITMAP)GetCurrentObject(hdcSrc, OBJ_BITMAP);
		ReleaseDC(_hwndSrc, hdcSrc);

		ComPtr<IWICBitmap> wicBmp = nullptr;
		_wicImgFactory->CreateBitmapFromHBITMAP(
			hBmpDest,
			NULL,
			WICBitmapAlphaChannelOption::WICBitmapIgnoreAlpha,
			&wicBmp
		);

		// �ü����ͻ���
		ComPtr<IWICBitmapClipper> wicBmpClipper = nullptr;
		_wicImgFactory->CreateBitmapClipper(&wicBmpClipper);

		WICRect wicRect = {
			_srcRect.left - windowRect.left,
			_srcRect.top - windowRect.top,
			srcSize.cx,
			srcSize.cy
		};
		wicBmpClipper->Initialize(wicBmp.Get(), &wicRect);

		return wicBmpClipper;
	}

	ComPtr<IWICBitmapSource> _GetFrameWithBitblt() {
		SIZE srcSize = Utils::GetSize(_srcRect);

		HDC hdcSrc = GetDC(_hwndSrc);
		HDC hdcDest = CreateCompatibleDC(hdcSrc);
		HBITMAP hBmpDest = CreateCompatibleBitmap(hdcSrc, srcSize.cx, srcSize.cy);

		SelectObject(hdcDest, hBmpDest);
		BitBlt(hdcDest, 0, 0, srcSize.cx, srcSize.cy, hdcSrc, 0, 0, SRCCOPY);
		ReleaseDC(_hwndSrc, hdcSrc);

		ComPtr<IWICBitmap> wicBmp = nullptr;
		_wicImgFactory->CreateBitmapFromHBITMAP(
			hBmpDest,
			NULL,
			WICBitmapAlphaChannelOption::WICBitmapIgnoreAlpha,
			&wicBmp
		);

		DeleteBitmap(hBmpDest);
		DeleteDC(hdcDest);

		return wicBmp;
	}

	IWICImagingFactory2* _wicImgFactory;
	const RECT& _srcRect;
	HWND _hwndSrc;
	bool _useBitblt;
};
