#pragma once
#include "pch.h"
#include "Utils.h"
#include "D2DContext.h"
#include "MagCallbackWindowCapturer.h"
#include "GDIWindowCapturer.h"
#include "WinRTCapturer.h"
#include "CursorManager.h"
#include "FrameCatcher.h"
#include "RenderManager.h"


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
        // �������˳���ͷ���Դ
        _renderManager = nullptr;
        _windowCapturer = nullptr;
        _d2dContext = nullptr;

        DestroyWindow(_hwndHost);
        UnregisterClass(_HOST_WINDOW_CLASS_NAME, _hInst);

        CoUninitialize();

        PostQuitMessage(0);
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
            CoInitializeEx(NULL, COINIT_MULTITHREADED),
            L"��ʼ�� COM ����"
        );

        Debug::Assert(
            IsWindow(_hwndSrc) && IsWindowVisible(_hwndSrc),
            L"hwndSrc ���Ϸ�"
        );

        Debug::Assert(
            captureMode >= 0 && captureMode <= 2,
            L"�Ƿ���ץȡģʽ"
        );

        Utils::GetClientScreenRect(_hwndSrc, _srcClient);

        _RegisterHostWndClass();
        _CreateHostWnd(noDisturb);

        Utils::GetClientScreenRect(_hwndHost, _hostClient);

        Debug::ThrowIfComFailed(
            CoCreateInstance(
                CLSID_WICImagingFactory,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(&_wicImgFactory)
            ),
            L"���� WICImagingFactory ʧ��"
        );


        // ��ʼ�� D2DContext
        _d2dContext.reset(new D2DContext(
            hInstance,
            _hwndHost,
            _hostClient,
            lowLatencyMode,
            noVSync,
            noDisturb
        ));

        if (captureMode == 0) {
            _windowCapturer.reset(new WinRTCapturer(
                *_d2dContext,
                _hwndSrc,
                _srcClient
            ));
        } else if (captureMode == 1) {
            _windowCapturer.reset(new GDIWindowCapturer(
                *_d2dContext,
                _hwndSrc,
                _srcClient,
                _wicImgFactory.Get()
            ));
        } else {
            _windowCapturer.reset(new MagCallbackWindowCapturer(
                *_d2dContext,
                hInstance,
                _hwndHost,
                _srcClient
            ));
        }

        _renderManager.reset(new RenderManager(
            *_d2dContext,
            effectsJson,
            _srcClient,
            _hostClient,
            noDisturb
        ));

        // ��ʼ�� CursorManager
        _renderManager->AddCursorManager(_hInst, _wicImgFactory.Get());

        if (showFPS) {
            // ��ʼ�� FrameCatcher
            _renderManager->AddFrameCatcher();
        }

        ShowWindow(_hwndHost, SW_NORMAL);

        PostMessage(_hwndHost, _WM_RENDER, 0, 0);

        // ���� Shell ��Ϣ��ʱ���ɿ�
        // _RegisterHookMsg();
    }

    // �ر�ȫ�����ڲ��˳��߳�
    // �������˳�·����
    // 1. ǰ̨���ڷ����ı�
    // 2. �յ�_WM_DESTORYMAG ��Ϣ
    static void _DestroyMagWindow() {
        $instance = nullptr;
    }

    // ��Ⱦһ֡�����׳��쳣
    void _Render() {
        try {
            const auto& frame = _windowCapturer->GetFrame();
            if (frame) {
                _renderManager->Render(frame);
            }
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
                _DestroyMagWindow();
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
            if (message == _WM_NEWCURSOR32) {
                // ���� CursorHook ����Ϣ
                // HCURSOR �ƺ��ǹ�����Դ���������Ա�Ľ��̵�����ֱ��ʹ��
                // 
                // �����Ϣ���� 32 λ���̣�������Ϊ 64 λ������ת��Ϊ������λ��չ������Ϊ�˺� SetCursor �Ĵ�����һ��
                // SendMessage Ϊ�� 0 ��չ��SetCursor Ϊ������λ��չ
                _renderManager->AddHookCursor((HCURSOR)(INT_PTR)(INT32)wParam, (HCURSOR)(INT_PTR)(INT32)lParam);
            } else if (message == _WM_NEWCURSOR64) {
                // �����Ϣ���� 64 λ���̣�������Ϊ 32 λ��HCURSOR �ᱻ�ض�
                // Q: ������ض��Ƿ�������������
                _renderManager->AddHookCursor((HCURSOR)wParam, (HCURSOR)lParam);
            } else if (message == _WM_DESTORYMAG) {
                _DestroyMagWindow();
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
    static UINT _WM_NEWCURSOR32;
    static UINT _WM_NEWCURSOR64;
    static UINT _WM_DESTORYMAG;

    UINT _WM_SHELLHOOKMESSAGE{};

    HINSTANCE _hInst;
    HWND _hwndHost = NULL;
    HWND _hwndSrc;
    RECT _srcClient{};
    RECT _hostClient{};

    ComPtr<IWICImagingFactory2> _wicImgFactory = nullptr;
    std::unique_ptr<D2DContext> _d2dContext = nullptr;
    std::unique_ptr<WindowCapturerBase> _windowCapturer = nullptr;
    std::unique_ptr<RenderManager> _renderManager = nullptr;
};
