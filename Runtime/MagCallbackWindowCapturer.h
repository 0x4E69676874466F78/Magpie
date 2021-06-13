#pragma once
#include "pch.h"
#include "WindowCapturerBase.h"
#include "Utils.h"
#include "Env.h"


// ʹ�� Magnification API ץȡ����
// �� https://docs.microsoft.com/en-us/previous-versions/windows/desktop/magapi/magapi-intro
// ��ʹ�ã���API�ѹ�ʱ�����ٶȺ���
class MagCallbackWindowCapturer : public WindowCapturerBase {
public:
	MagCallbackWindowCapturer() {
        if (_instance) {
            Debug::Assert(false, L"�Ѵ��� MagCallbackWindowCapturer ʵ��");
        }

		Debug::ThrowIfWin32Failed(
			MagInitialize(),
			L"MagInitialize ʧ��"
		);

        // �������ɼ��ķŴ󾵿ؼ�
        // ��СΪĿ�괰�ڿͻ���
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

    ComPtr<IUnknown> GetFrame() override {
        // MagSetWindowSource ��ͬ��ִ�е�
        if (!MagSetWindowSource(_hwndMag, Env::$instance->GetSrcClient())) {
            Debug::WriteErrorMessage(L"MagSetWindowSource ʧ��");
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
            Debug::WriteErrorMessage(L"srcdata ����BGRA��ʽ");
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
