#pragma once
#include "pch.h"
#include "WindowCapturerBase.h"
#include "Utils.h"


// ʹ�� GDI ץȡ����
class GDIWindowCapturer : public WindowCapturerBase {
public:
	GDIWindowCapturer(
		D2DContext& d2dContext,
		HWND hwndSrc,
		const RECT& srcRect,
		ComPtr<IWICImagingFactory2> wicImgFactory,
		bool useBitblt = false
	): _d2dContext(d2dContext), _wicImgFactory(wicImgFactory), _srcRect(srcRect), _hwndSrc(hwndSrc), _useBitblt(useBitblt) {
	}

	ComPtr<IUnknown> GetFrame() override {
		return _useBitblt ? _GetFrameWithBitblt() : _GetFrameWithNoBitblt();
	}

	CaptureredFrameType GetFrameType() override {
		return CaptureredFrameType::WICBitmap;
	}
private:
	ComPtr<IWICBitmapSource> _GetFrameWithNoBitblt() {
		SIZE srcSize = Utils::GetSize(_srcRect);
		RECT windowRect{};
		Debug::ThrowIfWin32Failed(
			GetWindowRect(_hwndSrc, &windowRect),
			L"GetWindowRectʧ��"
		);

		HDC hdcSrc = GetWindowDC(_hwndSrc);
		Debug::ThrowIfWin32Failed(
			hdcSrc,
			L"GetDCʧ��"
		);
		// ֱ�ӻ�ȡҪ�Ŵ󴰿ڵ� DC ������ HBITMAP���������Լ�����һ��
		HBITMAP hBmpDest = (HBITMAP)GetCurrentObject(hdcSrc, OBJ_BITMAP);
		Debug::ThrowIfWin32Failed(
			hBmpDest,
			L"GetCurrentObjectʧ��"
		);
		Debug::ThrowIfWin32Failed(
			ReleaseDC(_hwndSrc, hdcSrc),
			L"ReleaseDCʧ��"
		);

		ComPtr<IWICBitmap> wicBmp = nullptr;
		Debug::ThrowIfComFailed(
			_wicImgFactory->CreateBitmapFromHBITMAP(
				hBmpDest,
				NULL,
				WICBitmapAlphaChannelOption::WICBitmapIgnoreAlpha,
				&wicBmp
			),
			L"CreateBitmapFromHBITMAPʧ��"
		);

		// �ü����ͻ���
		ComPtr<IWICBitmapClipper> wicBmpClipper = nullptr;
		Debug::ThrowIfComFailed(
			_wicImgFactory->CreateBitmapClipper(&wicBmpClipper),
			L"CreateBitmapClipperʧ��"
		);

		WICRect wicRect = {
			_srcRect.left - windowRect.left,
			_srcRect.top - windowRect.top,
			srcSize.cx,
			srcSize.cy
		};
		Debug::ThrowIfComFailed(
			wicBmpClipper->Initialize(wicBmp.Get(), &wicRect),
			L"wicBmpClipper��ʼ��ʧ��"
		);
		
		return wicBmpClipper;
	}

	ComPtr<IWICBitmapSource> _GetFrameWithBitblt() {
		SIZE srcSize = Utils::GetSize(_srcRect);

		HDC hdcSrc = GetDC(_hwndSrc);
		Debug::ThrowIfWin32Failed(
			hdcSrc,
			L"GetDCʧ��"
		);
		HDC hdcDest = CreateCompatibleDC(hdcSrc);
		Debug::ThrowIfWin32Failed(
			hdcDest,
			L"CreateCompatibleDCʧ��"
		);
		HBITMAP hBmpDest = CreateCompatibleBitmap(hdcSrc, srcSize.cx, srcSize.cy);
		Debug::ThrowIfWin32Failed(
			hBmpDest,
			L"CreateCompatibleBitmapʧ��"
		);

		Debug::ThrowIfWin32Failed(
			SelectObject(hdcDest, hBmpDest),
			L"SelectObjectʧ��"
		);
		Debug::ThrowIfWin32Failed(
			BitBlt(hdcDest, 0, 0, srcSize.cx, srcSize.cy, hdcSrc, 0, 0, SRCCOPY),
			L"BitBltʧ��"
		);
		
		Debug::ThrowIfWin32Failed(
			ReleaseDC(_hwndSrc, hdcSrc),
			L"ReleaseDCʧ��"
		);

		ComPtr<IWICBitmap> wicBmp = nullptr;
		Debug::ThrowIfComFailed(
			_wicImgFactory->CreateBitmapFromHBITMAP(
				hBmpDest,
				NULL,
				WICBitmapAlphaChannelOption::WICBitmapIgnoreAlpha,
				&wicBmp
			),
			L"CreateBitmapFromHBITMAPʧ��"
		);

		Debug::ThrowIfWin32Failed(
			DeleteBitmap(hBmpDest),
			L"DeleteBitmapʧ��"
		);
		Debug::ThrowIfWin32Failed(
			DeleteDC(hdcDest),
			L"DeleteDCʧ��"
		);

		return wicBmp;
	}

	ComPtr<IWICImagingFactory2> _wicImgFactory;
	const RECT& _srcRect;
	HWND _hwndSrc;
	bool _useBitblt;

	D2DContext& _d2dContext;
};
