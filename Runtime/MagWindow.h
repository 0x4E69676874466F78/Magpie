#pragma once
#include "pch.h"
#include "Utils.h"
#include "D2DContext.h"
#include "RenderManager.h"


// ����ȫ�����ڵĴ���������
class MagWindow {
public:
    // ȷ��ֻ��ͬʱ����һ��ȫ������
    static std::unique_ptr<MagWindow> $instance;

    static void CreateInstance() {
        $instance.reset(new MagWindow());
    }
    
    // ���ɸ��ƣ������ƶ�
    MagWindow(const MagWindow&) = delete;
    MagWindow(MagWindow&&) = delete;

    ~MagWindow() {
        UnregisterClass(_HOST_WINDOW_CLASS_NAME, Env::$instance->GetHInstance());
    }


public:
    static std::wstring RunMsgLoop() {
        std::wstring errMsg;

        while (true) {
            MSG msg;
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) {
                    return std::move(errMsg);
                }

                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            try {
                if ($instance) {
                    $instance->_renderManager->Render();
                }
            } catch (const magpie_exception& e) {
                errMsg = L"��Ⱦ����" + e.what();
                DestroyWindow(Env::$instance->GetHwndHost());
            } catch (...) {
                errMsg = L"��Ⱦ����δ֪����";
                DestroyWindow(Env::$instance->GetHwndHost());
            }
        }
    }

private:
    MagWindow() {
        if ($instance) {
            Debug::Assert(false, L"�Ѵ���ȫ������");
        }

        HWND hwndSrc = Env::$instance->GetHwndSrc();
        Debug::Assert(
            IsWindow(hwndSrc) && IsWindowVisible(hwndSrc),
            L"hwndSrc ���Ϸ�"
        );

        _RegisterHostWndClass();
        _CreateHostWnd();

        _renderManager.reset(new RenderManager());

        Debug::ThrowIfWin32Failed(
            ShowWindow(Env::$instance->GetHwndHost(), SW_NORMAL),
            L"ShowWindowʧ��"
        );
    }


    LRESULT _HostWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
        if (message == _WM_DESTORYMAG) {
            DestroyWindow(Env::$instance->GetHwndHost());
            return 0;
        }
        if (message == WM_DESTROY) {
            // �������˳�·����
            // 1. ǰ̨���ڷ����ı�
            // 2. �յ�_WM_DESTORYMAG ��Ϣ
            $instance = nullptr;
            PostQuitMessage(0);
            return 0;
        } else {
            auto [resolved, rt] = _renderManager->WndProc(hWnd, message, wParam, lParam);

            if (resolved) {
                return rt;
            } else {
                return DefWindowProc(hWnd, message, wParam, lParam);
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
        wcex.hInstance = Env::$instance->GetHInstance();
        wcex.lpszClassName = _HOST_WINDOW_CLASS_NAME;

        // �����ظ�ע����ɵĴ���
        RegisterClassEx(&wcex);
    }

    void _CreateHostWnd() {
        // ����ȫ������
        SIZE screenSize = Utils::GetScreenSize(Env::$instance->GetHwndSrc());
        HWND hwndHost = CreateWindowEx(
            (Env::$instance->IsNoDisturb() ? 0 : WS_EX_TOPMOST) | WS_EX_NOACTIVATE | WS_EX_LAYERED | WS_EX_TRANSPARENT,
            _HOST_WINDOW_CLASS_NAME, NULL, WS_CLIPCHILDREN | WS_POPUP | WS_VISIBLE,
            0, 0, screenSize.cx, screenSize.cy,
            NULL, NULL, Env::$instance->GetHInstance(), NULL);
        Debug::ThrowIfWin32Failed(hwndHost, L"����ȫ������ʧ��");

        // ���ô��ڲ�͸��
        Debug::ThrowIfWin32Failed(
            SetLayeredWindowAttributes(hwndHost, 0, 255, LWA_ALPHA),
            L"SetLayeredWindowAttributes ʧ��"
        );

        Env::$instance->SetHwndHost(hwndHost);
    }


    // ȫ����������
    static constexpr const wchar_t* _HOST_WINDOW_CLASS_NAME = L"Window_Magpie_967EB565-6F73-4E94-AE53-00CC42592A22";

    static UINT _WM_DESTORYMAG;
    
    std::unique_ptr<RenderManager> _renderManager = nullptr;
};
