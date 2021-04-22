#pragma once
#include "pch.h"
#include "WindowCapturerBase.h"
#include "Utils.h"


// ʹ�� Magnification API ץȡ����
// �� https://docs.microsoft.com/en-us/previous-versions/windows/desktop/magapi/magapi-intro
class MagCallbackWindowCapturer : public WindowCapturerBase {
public:
	MagCallbackWindowCapturer(
        D2DContext& d2dContext,
        HINSTANCE hInstance,
        HWND hwndHost,
        const RECT& srcRect
    ): WindowCapturerBase(d2dContext), _srcRect(srcRect) {
        if (_instance) {
            Debug::Assert(false, L"�Ѵ��� MagCallbackWindowCapturer ʵ��");
        }

		Debug::ThrowIfWin32Failed(
			MagInitialize(),
			L"MagInitialize ʧ��"
		);

        // �������ɼ��ķŴ󾵿ؼ�
        // ��СΪĿ�괰�ڿͻ���
        SIZE srcClientSize = Utils::GetSize(srcRect);

        _hwndMag = CreateWindow(WC_MAGNIFIER, L"MagnifierWindow",
            WS_CHILD,
            0, 0, srcClientSize.cx, srcClientSize.cy,
            hwndHost, NULL, hInstance, NULL);
        Debug::ThrowIfWin32Failed(_hwndMag, L"�����Ŵ󾵿ؼ�ʧ��");

        Debug::ThrowIfWin32Failed(
            MagSetImageScalingCallback(_hwndMag, &_ImageScalingCallback),
            L"���÷Ŵ󾵻ص�ʧ��"
        );

        _instance = this;
	}

	~MagCallbackWindowCapturer() {
		MagUninitialize();

        _instance = nullptr;
	}

    ComPtr<ID2D1Bitmap> GetFrame() override {
        // MagSetWindowSource ��ͬ��ִ�е�
        if (!MagSetWindowSource(_hwndMag, _srcRect)) {
            Debug::WriteErrorMessage(L"MagSetWindowSource ʧ��");
        }

        return _bmp;
	}

    bool IsAutoRender() override {
        return false;
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
            Debug::WriteErrorMessage(L"srcdata ����BGRA��ʽ");
            return FALSE;
        }
        
        _instance->_d2dContext.GetD2DDC()->CreateBitmap(
            { srcheader.width, srcheader.height },
            BitmapProperties(PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)),
            & _instance->_bmp
        );
        D2D1_RECT_U destRect = { 0, 0, srcheader.width, srcheader.height };
        _instance->_bmp->CopyFromMemory(&destRect, srcdata, srcheader.stride);

        return TRUE;
    }

    HWND _hwndMag = NULL;
    const RECT& _srcRect;

    ComPtr<ID2D1Bitmap> _bmp = nullptr;

    static MagCallbackWindowCapturer* _instance;
};
