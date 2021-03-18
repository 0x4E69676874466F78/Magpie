#pragma once
#include "pch.h"
#include "Utils.h"
#include "EffectRenderer.h"
#include "MagCallbackWindowCapturer.h"
#include "GDIWindowCapturer.h"
#include "CursorManager.h"
#include "FrameCatcher.h"


class MagWindow {
public:
    // ȷ��ֻ��ͬʱ����һ��ȫ������
    static std::unique_ptr<MagWindow> $instance;

    static void CreateInstance(
        HINSTANCE hInstance,
        HWND hwndSrc,
        const std::wstring_view& effectsJson,
        int captureMode = 0,
        bool showFPS = false,
        bool lowLatencyMode = false,
        bool noVSync = false,
        bool noDisturb = false
    ) {
        $instance.reset(new MagWindow(
            hInstance,
            hwndSrc,
            effectsJson,
            captureMode,
            showFPS,
            lowLatencyMode,
            noVSync,
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

    HWND GetSrcWnd() {
        return _hwndSrc;
    }

    HWND GetHostWnd() {
        return _hwndHost;
    }

private:
    MagWindow(
        HINSTANCE hInstance,
        HWND hwndSrc,
        const std::wstring_view& effectsJson,
        int captureMode,
        bool showFPS,
        bool lowLatencyMode,
        bool noVSync,
        bool noDisturb
    ) : _hInst(hInstance), _hwndSrc(hwndSrc) {
        if ($instance) {
            Debug::Assert(false, L"�Ѵ���ȫ������");
        }

        Debug::ThrowIfComFailed(
            CoInitialize(NULL),
            L"��ʼ�� COM ����"
        );
        
        Debug::Assert(
            IsWindow(_hwndSrc) && IsWindowVisible(_hwndSrc),
            L"hwndSrc ���Ϸ�"
        );

        Debug::Assert(
            captureMode >= 0 && captureMode <= 1,
            L"�Ƿ���ץȡģʽ"
        );

        Utils::GetClientScreenRect(_hwndSrc, _srcClient);

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

        Debug::ThrowIfComFailed(
            DWriteCreateFactory(
                DWRITE_FACTORY_TYPE_SHARED,
                __uuidof(IDWriteFactory),
                &_dwFactory
            ),
            L"���� IDWriteFactory ʧ��"
        );

        if (captureMode == 0) {
            _windowCapturer.reset(new GDIWindowCapturer(
                _hwndSrc,
                _srcClient,
                _wicImgFactory.Get()
            ));
        } else if (captureMode == 1) {
            _windowCapturer.reset(new MagCallbackWindowCapturer(
                hInstance,
                _hwndHost,
                _srcClient,
                _wicImgFactory.Get()
            ));
        }
        

        // ��ʼ�� EffectRenderer
        _effectRenderer.reset(new EffectRenderer(
            hInstance,
            _hwndHost,
            effectsJson,
            _srcClient,
            _wicImgFactory.Get(),
            lowLatencyMode,
            noVSync,
            noDisturb
        ));

        // ��ʼ�� CursorManager
        _cursorManager.reset(new CursorManager(
            _effectRenderer->GetD2DDC(),
            hInstance,
            _wicImgFactory.Get(),
            D2D1::RectF(
                (float)_srcClient.left,
                (float)_srcClient.top,
                (float)_srcClient.right,
                (float)_srcClient.bottom
            ),
            _effectRenderer->GetDestRect(),
            noDisturb
        ));

        if (showFPS) {
            // ��ʼ�� FrameCatcher
            _frameCatcher.reset(new FrameCatcher(
                _effectRenderer->GetD2DDC(),
                _dwFactory.Get(),
                _effectRenderer->GetDestRect()
            ));
        }

        ShowWindow(_hwndHost, SW_NORMAL);

        PostMessage(_hwndHost, _WM_RENDER, 0, 0);

        // ���� Shell ��Ϣ��ʱ���ɿ�
        // _RegisterHookMsg();
    }


    // ��Ⱦһ֡�����׳��쳣
    void _Render() {
        try {
            ComPtr<IWICBitmapSource> frame = _windowCapturer->GetFrame();
            _effectRenderer->Render(frame.Get(), {
                _cursorManager.get(),
                _frameCatcher.get()
            });
        } catch (const magpie_exception& e) {
            Debug::WriteErrorMessage(L"��Ⱦʧ�ܣ�" + e.what());
        } catch (...) {
            Debug::WriteErrorMessage(L"��Ⱦ����δ֪����");
        }
    }

    LRESULT _HostWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
        switch (message) {
        case _WM_RENDER:
        {
            // ǰ̨���ڸı�ʱ�Զ��ر�ȫ������
            // ���� shell ��Ϣ��ʱ���ɿ�
            if (GetForegroundWindow() != _hwndSrc) {
                $instance = nullptr;
                break;
            }

            _Render();

            // ������Ⱦ��һ֡
            // ��ֱͬ������ʱ�Զ�����֡��
            if (!PostMessage(hWnd, _WM_RENDER, 0, 0)) {
                Debug::WriteErrorMessage(L"PostMessage ʧ��");
            }
            break;
        }
        default:
        {
            /*if (message == $instance->_WM_SHELLHOOKMESSAGE) {
                // �����滷���仯ʱ�ر�ȫ������
                // �ĵ�û�ᵽ�����������ضϳ� byte�������޷�����
                BYTE code = (BYTE)wParam;

                if (code == HSHELL_WINDOWACTIVATED
                    || code == HSHELL_WINDOWREPLACED
                    || code == HSHELL_WINDOWREPLACING
                    || (code == HSHELL_WINDOWDESTROYED && (HWND)lParam == $instance->_hwndSrc)
                ) {
                    $instance = nullptr;
                }
            } else */if (message == _WM_NEWCURSOR) {
                // ���� CursorHook ����Ϣ
                // HCURSOR �ƺ��ǹ�����Դ���������Ա�Ľ��̵�����ֱ��ʹ��
                _cursorManager->AddHookCursor((HCURSOR)wParam, (HCURSOR)lParam);
            } else {
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        }

        return 0;
    }

    static LRESULT CALLBACK _HostWndProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
        if (!$instance) {
            return DefWindowProc(hWnd, message, wParam, lParam);
        } else {
            return $instance->_HostWndProc(hWnd, message, wParam, lParam);
        }
    }

    // ע��ȫ��������
    void _RegisterHostWndClass() const {
        WNDCLASSEX wcex = {};
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.lpfnWndProc = _HostWndProcStatic;
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
    static constexpr const UINT _WM_RENDER = WM_USER;
    static UINT _WM_NEWCURSOR;

    UINT _WM_SHELLHOOKMESSAGE{};

    HINSTANCE _hInst;
    HWND _hwndHost = NULL;
    HWND _hwndSrc;
    RECT _srcClient{};

    ComPtr<IWICImagingFactory2> _wicImgFactory = nullptr;
    std::unique_ptr<EffectRenderer> _effectRenderer = nullptr;
    std::unique_ptr<WindowCapturerBase> _windowCapturer = nullptr;
    std::unique_ptr<CursorManager> _cursorManager = nullptr;
    std::unique_ptr<FrameCatcher> _frameCatcher = nullptr;
    ComPtr<IDWriteFactory> _dwFactory = nullptr;
};
