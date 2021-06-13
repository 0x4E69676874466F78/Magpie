#pragma once
#include "pch.h"
#include "Renderable.h"
#include "D2DImageEffectRenderer.h"
#include "WICBitmapEffectRenderer.h"
#include "D2DContext.h"
#include "CursorManager.h"
#include "FrameCatcher.h"
#include "WindowCapturerBase.h"
#include "MagCallbackWindowCapturer.h"
#include "GDIWindowCapturer.h"
#include "WinRTCapturer.h"
#include "Env.h"


// ����ȫ�����ڵ���Ⱦ
class RenderManager {
public:
	RenderManager() {
		// ��ʼ�� D2DContext
		_d2dContext.reset(new D2DContext());

		// ��ʼ�� WindowCapturer
		int captureMode = Env::$instance->GetCaptureMode();
		if (captureMode == 0) {
			_windowCapturer.reset(new WinRTCapturer());
		} else if (captureMode == 1) {
			_windowCapturer.reset(new GDIWindowCapturer());
		} else {
			throw new magpie_exception(L"�Ƿ���ץȡģʽ");
		}

		// ��ʼ�� EffectRenderer
		CaptureredFrameType frameType = _windowCapturer->GetFrameType();
		if (frameType == CaptureredFrameType::D2DImage) {
			_effectRenderer.reset(new D2DImageEffectRenderer());
		} else {
			_effectRenderer.reset(new WICBitmapEffectRenderer());
		}

		// ��ʼ�� CursorManager
		_cursorManager.reset(new CursorManager());

		if (Env::$instance->IsShowFPS()) {
			// ��ʼ�� FrameCatcher
			_frameCatcher.reset(new FrameCatcher());
		}
	}

	std::pair<bool, LRESULT> WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
		return _cursorManager->WndProc(hWnd, message, wParam, lParam);
	}

	void Render() {
		// ÿ����Ⱦ֮ǰ���Դ����״̬
		if (!_CheckSrcState()) {
			return;
		}
		const auto& frame = _windowCapturer->GetFrame();
		if (!frame) {
			return;
		}

		_d2dContext->Render([&](ID2D1DeviceContext* d2dDC) {
			d2dDC->Clear();

			ComPtr<ID2D1Image> img = _effectRenderer->Apply(frame.Get());
			img = _cursorManager->RenderEffect(img);
			
			const D2D_RECT_F& destRect = Env::$instance->GetDestRect();
			Env::$instance->GetD2DDC()->DrawImage(img.Get(), { destRect.left, destRect.top });

			if (_frameCatcher) {
				_frameCatcher->Render();
			}
			if (_cursorManager) {
				_cursorManager->Render();
			}
		});
	}

	
private:
	bool _CheckSrcState() {
		HWND hwndSrc = Env::$instance->GetHwndSrc();
		if (GetForegroundWindow() == hwndSrc) {
			// �ȼ��ǰ̨���ڣ������ڴ����ѹر�ʱGetClientScreenRect�����
			RECT rect;
			Utils::GetClientScreenRect(hwndSrc, rect);

			if (Env::$instance->GetSrcClient() == rect && Utils::GetWindowShowCmd(hwndSrc) == SW_NORMAL) {
				return true;
			}
		}

		// ״̬�ı�ʱ�ر�ȫ������
		DestroyWindow(Env::$instance->GetHwndHost());
		return false;
	}

	std::unique_ptr<D2DContext> _d2dContext = nullptr;
	std::unique_ptr<WindowCapturerBase> _windowCapturer = nullptr;
	std::unique_ptr<EffectRendererBase> _effectRenderer = nullptr;
	std::unique_ptr<CursorManager> _cursorManager = nullptr;
	std::unique_ptr<FrameCatcher> _frameCatcher = nullptr;
};
