#pragma once
#include "pch.h"
#include "WindowCapturerBase.h"
#include "Utils.h"


// ʹ�� Magnification API ץȡ����
// �� https://docs.microsoft.com/en-us/previous-versions/windows/desktop/magapi/magapi-intro
class MagCallbackWindowCapturer : public WindowCapturerBase {
public:
	MagCallbackWindowCapturer(
        HINSTANCE hInstance,
        HWND hwndHost,
        const RECT& srcRect,
        IWICImagingFactory2* wicImgFactory
    ): _srcRect(srcRect), _wicImgFactory(wicImgFactory) {
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

	ComPtr<IWICBitmapSource> GetFrame() override {
        // MagSetWindowSource ��ͬ��ִ�е�
        if (!MagSetWindowSource(_hwndMag, _srcRect)) {
            Debug::WriteErrorMessage(L"MagSetWindowSource ʧ��");
        }

        return _wicBmp;
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

        Debug::ThrowIfComFailed(
            _instance->_wicImgFactory->CreateBitmapFromMemory(
                srcheader.width,
                srcheader.height,
                GUID_WICPixelFormat32bppPBGRA,
                srcheader.stride,
                (UINT)srcheader.cbSize,
                (BYTE*)srcdata,
                &_instance->_wicBmp
            ),
            L"���ڴ洴�� WICBitmap ʧ��"
        );

        return TRUE;
    }

    HWND _hwndMag = NULL;
    const RECT& _srcRect;
    IWICImagingFactory2* _wicImgFactory;

    ComPtr<IWICBitmap> _wicBmp = nullptr;

    static MagCallbackWindowCapturer* _instance;
};
