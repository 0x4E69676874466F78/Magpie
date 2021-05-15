#pragma once
#include "pch.h"
#include "WindowCapturerBase.h"
#include "Utils.h"
#include <queue>
#include <chrono>
#include "Env.h"

using namespace std::chrono;


// ʹ�� GDI ץȡ����
class GDIWindowCapturer : public WindowCapturerBase {
public:
	GDIWindowCapturer() {
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
		HWND hwndSrc = Env::$instance->GetHwndSrc();
		
		RECT windowRect{};
		Debug::ThrowIfWin32Failed(
			GetWindowRect(hwndSrc, &windowRect),
			L"GetWindowRectʧ��"
		);
		
		HDC hdcSrc = GetWindowDC(hwndSrc);
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
			ReleaseDC(hwndSrc, hdcSrc),
			L"ReleaseDCʧ��"
		);

		IWICImagingFactory2* wicImgFactory = Env::$instance->GetWICImageFactory();
		ComPtr<IWICBitmap> wicBmp = nullptr;
		Debug::ThrowIfComFailed(
			wicImgFactory->CreateBitmapFromHBITMAP(
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
			wicImgFactory->CreateBitmapClipper(&wicBmpClipper),
			L"CreateBitmapClipperʧ��"
		);

		const RECT& srcClient = Env::$instance->GetSrcClient();
		WICRect wicRect = {
			srcClient.left - windowRect.left,
			srcClient.top - windowRect.top,
			srcClient.right - srcClient.left,
			srcClient.bottom - srcClient.top
		};
		Debug::ThrowIfComFailed(
			wicBmpClipper->Initialize(wicBmp.Get(), &wicRect),
			L"wicBmpClipper��ʼ��ʧ��"
		);

		return wicBmpClipper;
	}


	ComPtr<IWICBitmapSource> _frame;
	// ͬ���� _frame �ķ���
	std::mutex _frameMutex;
	std::atomic_bool _closed = false;

	std::unique_ptr<std::thread> _getFrameThread;
};
