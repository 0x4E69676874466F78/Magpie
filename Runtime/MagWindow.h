#pragma once
#include "pch.h"
#include "Utils.h"
#include "EffectRenderer.h"


class MagWindow {
public:
	MagWindow(
        HINSTANCE hInstance, 
        HWND hwndSrc, 
        UINT frameRate,
        const std::wstring_view& effectsJson,
        bool noDisturb = false
    ) : _hInst(hInstance), _hwndSrc(hwndSrc), _frameRate(frameRate)
    {
        if (_instance) {
            Debug::ThrowIfFalse(false, L"�Ѵ���ȫ������");
        }

        assert(_frameRate == 0 || (_frameRate >= 30 && _frameRate <= 120));

        Debug::ThrowIfFalse(
            IsWindow(_hwndSrc) && IsWindowVisible(_hwndSrc),
            L"hwndSrc ���Ϸ�"
        );

        Debug::ThrowIfFalse(
            Utils::GetClientScreenRect(_hwndSrc, _srcRect),
            L"�޷���ȡԴ���ڿͻ����ߴ�"
        );

        _RegisterHostWndClass();

        Debug::ThrowIfFalse(
            MagInitialize(),
            L"MagInitialize ʧ��"
        );
        _CreateHostWnd(noDisturb);

        _InitWICImgFactory();

        // ��ʼ�� EffectRenderer
        _effectRenderer.reset(
            new EffectRenderer(
                hInstance,
                _hwndHost,
                effectsJson,
                _srcRect,
                _wicImgFactory,
                noDisturb
            )
        );

        ShowWindow(_hwndHost, SW_NORMAL);

        if (_frameRate > 0) {
            SetTimer(_hwndHost, 1, 1000 / _frameRate, nullptr);
        } else {
            PostMessage(_hwndHost, WM_MAXFRAMERATE, 0, 0);
        }

        // ��ʼ����ɺ����� _instance
        _instance = this;
	}

    // ���ɸ��ƣ������ƶ�
    MagWindow(const MagWindow&) = delete;
    MagWindow(MagWindow&&) = delete;
    
    ~MagWindow() {
        MagUninitialize();

        DestroyWindow(_hwndHost);
        _instance = nullptr;

        UnregisterClass(_HOST_WINDOW_CLASS_NAME, _hInst);
    }

    static LRESULT CALLBACK HostWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
        switch (message) {
        case WM_TIMER:
        {
            if (!_instance) {
                // timer��ʱ�ѱ�����
                break;
            }

            // ��Ⱦһ֡
            if (!MagSetWindowSource(_instance->_hwndMag, _instance->_srcRect)) {
                Debug::writeLine(L"MagSetWindowSource ʧ��");
            }
            break;
        }
        case WM_MAXFRAMERATE:
        {
            if (!_instance) {
                // ���������٣�ֱ���˳�
                break;
            }

            // ��Ⱦһ֡
            if (!MagSetWindowSource(_instance->_hwndMag, _instance->_srcRect)) {
                Debug::writeLine(L"MagSetWindowSource ʧ��");
            }

            // ������Ⱦ��һ֡
            if (!PostMessage(hWnd, WM_MAXFRAMERATE, 0, 0)) {
                Debug::writeLine(L"PostMessage ʧ��");
            }
            break;
        }
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        return 0;
    }


private:
    // ע��ȫ��������
    void _RegisterHostWndClass() const {
        WNDCLASSEX wcex = {};
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.lpfnWndProc = HostWndProc;
        wcex.hInstance = _hInst;
        wcex.lpszClassName = _HOST_WINDOW_CLASS_NAME;

        // �����ظ�ע����ɵĴ���
        RegisterClassEx(&wcex);
    }

    void _CreateHostWnd(bool noDisturb) {
        // ����ȫ������
        SIZE screenSize = Utils::GetScreenSize(_hwndSrc);
        _hwndHost = CreateWindowEx(
            (noDisturb ? 0 : WS_EX_TOPMOST) | WS_EX_NOACTIVATE | WS_EX_LAYERED | WS_EX_TRANSPARENT,
            _HOST_WINDOW_CLASS_NAME, NULL, WS_CLIPCHILDREN | WS_POPUP | WS_VISIBLE,
            0, 0, screenSize.cx, screenSize.cy,
            NULL, NULL, _hInst, NULL);
        Debug::ThrowIfFalse(_hwndHost, L"����ȫ������ʧ��");

        // ���ô��ڲ�͸��
        Debug::ThrowIfFalse(
            SetLayeredWindowAttributes(_hwndHost, 0, 255, LWA_ALPHA),
            L"SetLayeredWindowAttributes ʧ��"
        );

        // �������ɼ��ķŴ󾵿ؼ�
        // ��СΪĿ�괰�ڿͻ���
        SIZE srcClientSize = Utils::GetSize(_srcRect);

        _hwndMag = CreateWindow(WC_MAGNIFIER, L"MagnifierWindow",
            WS_CHILD,
            0, 0, srcClientSize.cx, srcClientSize.cy,
            _hwndHost, NULL, _hInst, NULL);
        Debug::ThrowIfFalse(_hwndMag, L"�����Ŵ󾵿ؼ�ʧ��");

        Debug::ThrowIfFalse(
            MagSetImageScalingCallback(_hwndMag, &_ImageScalingCallback),
            L"���÷Ŵ󾵻ص�ʧ��"
        );
    }

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
            Debug::writeLine(L"srcdata ����BGRA��ʽ");
            return FALSE;
        }

        ComPtr<IWICBitmap> wicBmpSource = nullptr;

        try {
            Debug::ThrowIfFailed(
                _instance->_wicImgFactory->CreateBitmapFromMemory(
                    srcheader.width,
                    srcheader.height,
                    GUID_WICPixelFormat32bppPBGRA,
                    srcheader.stride,
                    (UINT)srcheader.cbSize,
                    (BYTE*)srcdata,
                    &wicBmpSource
                ),
                L"���ڴ洴�� WICBitmap ʧ��"
            );

            _instance->_effectRenderer->Render(wicBmpSource);
        } catch (...) {
            Debug::writeLine(L"��Ⱦ����");
            return FALSE;
        }
        
        return TRUE;
    }


    void _InitWICImgFactory() {
        Debug::ThrowIfFailed(
            CoInitialize(NULL), 
            L"��ʼ�� COM ����"
        );

        Debug::ThrowIfFailed(
            CoCreateInstance(
                CLSID_WICImagingFactory,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(&_wicImgFactory)
            ),
            L"���� WICImagingFactory ʧ��"
        );
    }

    // ȫ����������
    static constexpr const wchar_t* _HOST_WINDOW_CLASS_NAME = L"Window_Magpie_967EB565-6F73-4E94-AE53-00CC42592A22";

    // ָ��Ϊ���֡��ʱʹ�õ���Ⱦ��Ϣ
    static constexpr const UINT WM_MAXFRAMERATE = WM_USER;

    HINSTANCE _hInst;
    HWND _hwndHost = NULL;
    HWND _hwndMag = NULL;
    HWND _hwndSrc;
    RECT _srcRect{};
    UINT _frameRate;

    ComPtr<IWICImagingFactory2> _wicImgFactory = nullptr;
    std::unique_ptr<EffectRenderer> _effectRenderer = nullptr;

    // ȷ��ֻ��ͬʱ����һ��ȫ������
    static MagWindow *_instance;
};
