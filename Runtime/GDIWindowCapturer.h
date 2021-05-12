#pragma once
#include "pch.h"
#include "WindowCapturerBase.h"
#include "Utils.h"
#include <queue>
#include <chrono>

using namespace std::chrono;


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
		// �ڵ������߳��в���ʹ��GDI�ػ񴰿�
		// ���������Ⱦ�߳��л���ɿ���
		_getFrameThread.reset(new std::thread([&]() {
			while (!_closed) {
				ComPtr<IWICBitmapSource> frame = _GetFrameWithNoBitblt();

				_frameMutex.lock();
				_frame = frame;
				_frameMutex.unlock();

				std::this_thread::yield();
			}
		}));
	}

	~GDIWindowCapturer() {
		_closed = true;
		_getFrameThread->join();
	}

	ComPtr<IUnknown> GetFrame() override {
		std::lock_guard<std::mutex> guard(_frameMutex);

		ComPtr<IWICBitmapSource> frame = _frame;
		_frame = nullptr;
		return frame;
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

	ComPtr<IWICImagingFactory2> _wicImgFactory;
	const RECT& _srcRect;
	const HWND _hwndSrc;
	bool _useBitblt;

	D2DContext& _d2dContext;

	ComPtr<IWICBitmapSource> _frame;
	// ͬ���� _frame �ķ���
	std::mutex _frameMutex;
	std::atomic_bool _closed = false;

	std::unique_ptr<std::thread> _getFrameThread;
};
