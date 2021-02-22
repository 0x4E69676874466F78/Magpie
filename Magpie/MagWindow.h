#pragma once
#include "pch.h"
#include <vector>
#include <map>
#include "Utils.h"
#include "EffectRenderer.h"


using namespace std;

class MagWindow {
public:
	MagWindow(HINSTANCE hInstance, HWND hwndSrc, UINT frameRate, const std::wstring_view& effectsJson) 
        : _hInst(hInstance), _hwndSrc(hwndSrc), _frameRate(frameRate)
    {
        assert(_frameRate >= 1 && _frameRate <= 200);

        Debug::ThrowIfFalse(
            IsWindow(_hwndSrc) && IsWindowVisible(_hwndSrc),
            L"hwndSrc ���Ϸ�"
        );

        Debug::ThrowIfFalse(
            Utils::GetClientScreenRect(_hwndSrc, _srcClient),
            L"�޷���ȡԴ���ڿͻ����ߴ�"
        );

        _RegisterHostWndClass();
        _CreateHostWnd();
        _InitWICImgFactory();

        // ��ʼ�� EffectRenderer
        _effectRenderer.reset(new EffectRenderer(_hwndHost, effectsJson, Utils::GetSize(_srcClient)));

        /*ClipCursor(&srcClient);

        systemCursors.hand = CopyCursor(LoadCursor(NULL, IDC_HAND));
        systemCursors.normal = CopyCursor(LoadCursor(NULL, IDC_ARROW));
        SetSystemCursor(CopyCursor(tptCursors.hand), OCR_HAND);
        SetSystemCursor(CopyCursor(tptCursors.normal), OCR_NORMAL);*/
        ShowWindow(_hwndHost, SW_NORMAL);
        SetTimer(_hwndHost, 1, 1000 / _frameRate, nullptr);
	}
    
    ~MagWindow() {
        DestroyWindow(_hwndHost);
    }

    static LRESULT CALLBACK HostWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
        switch (message) {
        case WM_CREATE:
        {
            MagInitialize();
            break;
        }
        case WM_DESTROY:
        {
            _instMap.erase(hWnd);
            MagUninitialize();
            //PostQuitMessage(0);

            /*hwndHost = NULL;
            hwndSrc = NULL;
            hwndMag = NULL;

            ClipCursor(NULL);

            SetSystemCursor(systemCursors.hand, OCR_HAND);
            SetSystemCursor(systemCursors.normal, OCR_NORMAL);
            systemCursors.hand = NULL;
            systemCursors.normal = NULL;*/
            break;
        }
        case WM_TIMER:
        {
            MagWindow* that = GetInstance(hWnd);
            if (!that) {
                // ����������
                KillTimer(hWnd, wParam);
                break;
            }

            // ��Ⱦһ֡
            MagSetWindowSource(that->_hwndMag, that->_srcClient);
            break;
        }
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        return 0;
    }


private:
    // ע��ȫ��������
    void _RegisterHostWndClass() {
        WNDCLASSEX wcex = {};
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.lpfnWndProc = HostWndProc;
        wcex.hInstance = _hInst;
        wcex.lpszClassName = _HOST_WINDOW_CLASS_NAME;

        // �����ظ�ע����ɵĴ���
        RegisterClassEx(&wcex);
    }

    void _CreateHostWnd() {
        // ����ȫ������
        SIZE screenSize = Utils::GetScreenSize(_hwndSrc);
        _hwndHost = CreateWindowEx(
            WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_LAYERED | WS_EX_TRANSPARENT,
            _HOST_WINDOW_CLASS_NAME, NULL, WS_CLIPCHILDREN | WS_POPUP | WS_VISIBLE,
            0, 0, screenSize.cx, screenSize.cy,
            NULL, NULL, _hInst, NULL);
        Debug::ThrowIfFalse(_hwndHost, L"����ȫ������ʧ��");

        _instMap[_hwndHost] = this;

        SetLayeredWindowAttributes(_hwndHost, 0, 255, LWA_ALPHA);

        // �������ɼ��ķŴ󾵿ؼ�
        // ��СΪĿ�괰�ڿͻ���
        SIZE srcClientSize = Utils::GetSize(_srcClient);

        _hwndMag = CreateWindow(WC_MAGNIFIER, L"MagnifierWindow",
            WS_CHILD,
            0, 0, srcClientSize.cx, srcClientSize.cy,
            _hwndHost, NULL, _hInst, NULL);
        Debug::ThrowIfFalse(_hwndMag, L"�����Ŵ󾵿ؼ�ʧ��");

        Debug::ThrowIfFalse(
            MagSetImageScalingCallback(_hwndMag, &_ImageScalingCallback),
            L"���÷Ŵ󾵻ص�ʧ��"
        );

        
        //ShowWindow(_hwndHost, SW_SHOW);
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
        MagWindow* that = _instMap.begin()->second;//GetInstance(GetParent(hWnd));
        
        Debug::ThrowIfFalse(
            srcheader.width * srcheader.height * 4 == srcheader.cbSize,
            L"srcdata ����BGRA��ʽ"
        );

        ComPtr<IWICBitmap> wicBmpSource = nullptr;
        Debug::ThrowIfFailed(
            that->_wicImgFactory->CreateBitmapFromMemory(
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

        that->_effectRenderer->Render(wicBmpSource);

        //DrawCursor(_d2dDC);

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

    // ���� MagWindow ʵ��
    static MagWindow* GetInstance(HWND hWnd) {
        return _instMap[hWnd];
    }

    // ȫ����������
    const wchar_t* _HOST_WINDOW_CLASS_NAME = L"Window_Magpie_967EB565-6F73-4E94-AE53-00CC42592A22";

    HINSTANCE _hInst;
    HWND _hwndHost = NULL;
    HWND _hwndMag = NULL;
    HWND _hwndSrc;
    RECT _srcClient{};
    UINT _frameRate;
    ComPtr<IWICImagingFactory2> _wicImgFactory = nullptr;
    std::unique_ptr<EffectRenderer> _effectRenderer = nullptr;

    // �洢����ʵ������ͨ�����ھ������
    static std::map<HWND, MagWindow*> _instMap;
};
