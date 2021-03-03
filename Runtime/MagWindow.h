#pragma once
#include "pch.h"
#include "Utils.h"
#include "EffectRenderer.h"
#include "MagCallbackWindowCapturer.h"


class MagWindow {
public:
    // ȷ��ֻ��ͬʱ����һ��ȫ������
    static std::unique_ptr<MagWindow> $instance;

    static void CreateInstance(
        HINSTANCE hInstance,
        HWND hwndSrc,
        UINT frameRate,
        const std::wstring_view& effectsJson,
        bool noDisturb = false
    ) {
        $instance.reset(new MagWindow(
            hInstance,
            hwndSrc,
            frameRate,
            effectsJson,
            noDisturb
        ));
    }

    // ���ɸ��ƣ������ƶ�
    MagWindow(const MagWindow&) = delete;
    MagWindow(MagWindow&&) = delete;
    
    ~MagWindow() {
        DestroyWindow(_hwndHost);
        UnregisterClass(_HOST_WINDOW_CLASS_NAME, _hInst);

        CoUninitialize();
    }

private:
    MagWindow(
        HINSTANCE hInstance,
        HWND hwndSrc,
        UINT frameRate,
        const std::wstring_view& effectsJson,
        bool noDisturb
    ) : _hInst(hInstance), _hwndSrc(hwndSrc), _frameRate(frameRate) {
        if ($instance) {
            Debug::Assert(false, L"�Ѵ���ȫ������");
        }

        assert(_frameRate == 0 || (_frameRate >= 30 && _frameRate <= 120));

        Debug::ThrowIfComFailed(
            CoInitialize(NULL),
            L"��ʼ�� COM ����"
        );

        Debug::Assert(
            IsWindow(_hwndSrc) && IsWindowVisible(_hwndSrc),
            L"hwndSrc ���Ϸ�"
        );

        Utils::GetClientScreenRect(_hwndSrc, _srcRect);

        _RegisterHostWndClass();
        _CreateHostWnd(noDisturb);

        Debug::ThrowIfComFailed(
            CoCreateInstance(
                CLSID_WICImagingFactory,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(&_wicImgFactory)
            ),
            L"���� WICImagingFactory ʧ��"
        );

        _windowCapturer.reset(new MagCallbackWindowCapturer(
            hInstance,
            _hwndHost,
            _srcRect,
            _wicImgFactory.Get()
        ));

        // ��ʼ�� EffectRenderer
        _effectRenderer.reset(
            new EffectRenderer(
                hInstance,
                _hwndHost,
                effectsJson,
                _srcRect,
                _wicImgFactory.Get(),
                noDisturb
            )
        );

        ShowWindow(_hwndHost, SW_NORMAL);

        if (_frameRate > 0) {
            SetTimer(_hwndHost, 1, 1000 / _frameRate, nullptr);
        } else {
            PostMessage(_hwndHost, _WM_MAXFRAMERATE, 0, 0);
        }

        _RegisterHookMsg();
    }

    // ��Ⱦһ֡�����׳��쳣
    static void _Render() {
        try {
            ComPtr<IWICBitmap> frame = $instance->_windowCapturer->GetFrame();
            $instance->_effectRenderer->Render(frame.Get());
        } catch (const magpie_exception& e) {
            Debug::WriteErrorMessage(L"��Ⱦʧ�ܣ�" + e.what());
        } catch (...) {
            Debug::WriteErrorMessage(L"��Ⱦ����δ֪����");
        }
    }

    static LRESULT CALLBACK _HostWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
        switch (message) {
        case WM_TIMER:
        {
            if (!$instance) {
                // timer��ʱ�ѱ�����
                break;
            }

            /* �Ѹ�Ϊ���� shell ��Ϣ
            if (GetForegroundWindow() != $instance->_hwndSrc) {
                DestroyMagWindow();
                break;
            }*/

            _Render();

            break;
        }
        case _WM_MAXFRAMERATE:
        {
            if (!$instance) {
                // ���������٣�ֱ���˳�
                break;
            }

            /* �Ѹ�Ϊ���� shell ��Ϣ
            if (GetForegroundWindow() != $instance->_hwndSrc) {
                DestroyMagWindow();
                break;
            }*/

            _Render();

            // ������Ⱦ��һ֡
            if (!PostMessage(hWnd, _WM_MAXFRAMERATE, 0, 0)) {
                Debug::WriteErrorMessage(L"PostMessage ʧ��");
            }
            break;
        }
        default:
        {
            if ($instance && $instance->_WM_SHELLHOOKMESSAGE == message) {
                // �����滷���仯ʱ�ر�ȫ������
                // �ĵ�û�ᵽ�����������ضϳ� byte�������޷�����
                switch ((BYTE)wParam) {
                case HSHELL_WINDOWACTIVATED:
                case HSHELL_WINDOWREPLACED:
                case HSHELL_WINDOWREPLACING:
                    $instance = nullptr;
                }
            } else {
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        }
        return 0;
    }

    // ע��ȫ��������
    void _RegisterHostWndClass() const {
        WNDCLASSEX wcex = {};
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.lpfnWndProc = _HostWndProc;
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
        Debug::ThrowIfWin32Failed(_hwndHost, L"����ȫ������ʧ��");

        // ���ô��ڲ�͸��
        Debug::ThrowIfWin32Failed(
            SetLayeredWindowAttributes(_hwndHost, 0, 255, LWA_ALPHA),
            L"SetLayeredWindowAttributes ʧ��"
        );
    }

    void _RegisterHookMsg() {
        _WM_SHELLHOOKMESSAGE = RegisterWindowMessage(L"SHELLHOOK");
        Debug::ThrowIfWin32Failed(
            _WM_SHELLHOOKMESSAGE,
            L"RegisterWindowMessage ʧ��"
        );
        Debug::ThrowIfWin32Failed(
            RegisterShellHookWindow(_hwndHost),
            L"RegisterShellHookWindow ʧ��"
        );
    }


    // ȫ����������
    static constexpr const wchar_t* _HOST_WINDOW_CLASS_NAME = L"Window_Magpie_967EB565-6F73-4E94-AE53-00CC42592A22";

    // ָ��Ϊ���֡��ʱʹ�õ���Ⱦ��Ϣ
    static constexpr const UINT _WM_MAXFRAMERATE = WM_USER;

    UINT _WM_SHELLHOOKMESSAGE = 0;

    HINSTANCE _hInst;
    HWND _hwndHost = NULL;
    HWND _hwndSrc;
    RECT _srcRect{};
    UINT _frameRate;

    ComPtr<IWICImagingFactory2> _wicImgFactory = nullptr;
    std::unique_ptr<EffectRenderer> _effectRenderer = nullptr;
    std::unique_ptr<MagCallbackWindowCapturer> _windowCapturer = nullptr;
};
