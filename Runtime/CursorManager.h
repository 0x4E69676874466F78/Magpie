#pragma once
#include "pch.h"
#include "Utils.h"
#include "Renderable.h"
#include "Env.h"


// ���������Ⱦ
class CursorManager: public Renderable {
public:
    CursorManager(const RECT& destRect, bool debugMode = false) : _destRect(destRect), _debugMode(debugMode) {
        _cursorSize.cx = GetSystemMetrics(SM_CXCURSOR);
        _cursorSize.cy = GetSystemMetrics(SM_CYCURSOR);

        HCURSOR hCursorArrow = LoadCursor(NULL, IDC_ARROW);
        HCURSOR hCursorHand = LoadCursor(NULL, IDC_HAND);
        HCURSOR hCursorAppStarting = LoadCursor(NULL, IDC_APPSTARTING);

        // �����滻֮ǰ�� arrow ���ͼ��
        // SetSystemCursor ����ı�ϵͳ���ľ��
        _cursorMap.emplace(hCursorArrow, _CursorToD2DBitmap(hCursorArrow));
        _cursorMap.emplace(hCursorHand, _CursorToD2DBitmap(hCursorHand));
        _cursorMap.emplace(hCursorAppStarting, _CursorToD2DBitmap(hCursorAppStarting));

        if (Env::$instance->IsNoDisturb()) {
            return;
        }

        Debug::ThrowIfWin32Failed(
            SetSystemCursor(_CreateTransparentCursor(hCursorArrow), OCR_NORMAL),
            L"���� OCR_NORMAL ʧ��"
        );
        Debug::ThrowIfWin32Failed(
            SetSystemCursor(_CreateTransparentCursor(hCursorHand), OCR_HAND),
            L"���� OCR_HAND ʧ��"
        );
        Debug::ThrowIfWin32Failed(
            SetSystemCursor(_CreateTransparentCursor(hCursorAppStarting), OCR_APPSTARTING),
            L"���� OCR_APPSTARTING ʧ��"
        );

        // ��������ڴ�����
        Debug::ThrowIfWin32Failed(ClipCursor(&Env::$instance->GetSrcClient()), L"ClipCursor ʧ��");

        // ��������ƶ��ٶ�
        Debug::ThrowIfWin32Failed(
            SystemParametersInfo(SPI_GETMOUSESPEED, 0, &_cursorSpeed, 0),
            L"��ȡ����ٶ�ʧ��"
        );

        const RECT& srcRect = Env::$instance->GetSrcClient();
        _scale = float(_destRect.right - _destRect.left) / (srcRect.right - srcRect.left);

        long newSpeed = std::clamp(lroundf(_cursorSpeed / _scale), 1L, 20L);
        Debug::ThrowIfWin32Failed(
            SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)(intptr_t)newSpeed, 0),
            L"��������ٶ�ʧ��"
        );
    }

	CursorManager(const CursorManager&) = delete;
	CursorManager(CursorManager&&) = delete;

    ~CursorManager() {
        if (Env::$instance->IsNoDisturb()) {
            return;
        }

        ClipCursor(nullptr);

        SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)(intptr_t)_cursorSpeed, 0);

        // ��ԭϵͳ���
        SystemParametersInfo(SPI_SETCURSORS, 0, NULL, 0);
    }

    void Render() override {
        CURSORINFO ci{};
        ci.cbSize = sizeof(ci);
        Debug::ThrowIfWin32Failed(
            GetCursorInfo(&ci),
            L"GetCursorInfo ʧ��"
        );

        if (_debugMode) {
            _RenderDebugLayer(ci.hCursor);
        }

        if (ci.hCursor == NULL) {
            return;
        }

        _DrawCursor(ci.hCursor, ci.ptScreenPos);
    }


    std::pair<bool, LRESULT> WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
        if (message == _WM_NEWCURSOR32) {
            // ���� CursorHook ����Ϣ
            // HCURSOR �ƺ��ǹ�����Դ���������Ա�Ľ��̵�����ֱ��ʹ��
            // 
            // �����Ϣ���� 32 λ���̣�������Ϊ 64 λ������ת��Ϊ������λ��չ������Ϊ�˺� SetCursor �Ĵ�����һ��
            // SendMessage Ϊ�� 0 ��չ��SetCursor Ϊ������λ��չ
            _AddHookCursor((HCURSOR)(INT_PTR)(INT32)wParam, (HCURSOR)(INT_PTR)(INT32)lParam);
            return { true, 0 };
        } else if (message == _WM_NEWCURSOR64) {
            // �����Ϣ���� 64 λ���̣�������Ϊ 32 λ��HCURSOR �ᱻ�ض�
            // Q: ������ض��Ƿ�������������
            _AddHookCursor((HCURSOR)wParam, (HCURSOR)lParam);
            return { true, 0 };
        }

        return { false, 0 };
    }
private:
    void _AddHookCursor(HCURSOR hTptCursor, HCURSOR hCursor) {
        if (hTptCursor == NULL || hCursor == NULL) {
            return;
        }

        _cursorMap[hTptCursor] = _CursorToD2DBitmap(hCursor);
    }

    void _RenderDebugLayer(HCURSOR hCursorCur) {
        D2D1_POINT_2F leftTop{ 0, 50 };

        D2D1_RECT_F cursorRect{
            leftTop.x, leftTop.y, leftTop.x + _cursorSize.cx, leftTop.y + _cursorSize.cy
        };

        ID2D1DeviceContext* d2dDC = Env::$instance->GetD2DDC();
        ComPtr<ID2D1SolidColorBrush> brush;
        d2dDC->CreateSolidColorBrush({ 0,0,0,1 }, &brush);

        for (const auto& a : _cursorMap) {
            d2dDC->FillRectangle(cursorRect, brush.Get());

            if (a.first != hCursorCur) {
                d2dDC->DrawBitmap(a.second.Get(), &cursorRect);
            }

            cursorRect.left += _cursorSize.cx;
            cursorRect.right += _cursorSize.cy;
        }
    }

    void _DrawCursor(HCURSOR hCursor, POINT ptScreenPos) {
        assert(hCursor != NULL);

        ID2D1DeviceContext* d2dDC = Env::$instance->GetD2DDC();

        auto it = _cursorMap.find(hCursor);
        ID2D1Bitmap* bmpCursor = nullptr;

        if (it != _cursorMap.end()) {
            bmpCursor = it->second.Get();
        } else {
            try {
                // δ��ӳ�����ҵ���������ӳ��
                ComPtr<ID2D1Bitmap> newBmpCursor = _CursorToD2DBitmap(hCursor);

                // ��ӽ�ӳ��
                _cursorMap[hCursor] = newBmpCursor;

                bmpCursor = newBmpCursor.Get();
            } catch (...) {
                // �������ֻ������ͨ���
                Debug::WriteLine(L"_CursorToD2DBitmap ����");

                hCursor = LoadCursor(NULL, IDC_ARROW);
                bmpCursor = _cursorMap[hCursor].Get();
            }
        }

        // ӳ������
        // �������Ϊ��������������ģ��
        const RECT& srcClient = Env::$instance->GetSrcClient();
        D2D1_POINT_2F targetScreenPos{
            roundf((ptScreenPos.x - srcClient.left) * _scale + _destRect.left),
            roundf((ptScreenPos.y - srcClient.top) * _scale + _destRect.top)
        };

        auto [xHotSpot, yHotSpot] = _GetCursorHotSpot(hCursor);

        D2D1_RECT_F rect{};
        Debug::ThrowIfComFailed(
            d2dDC->GetImageLocalBounds(bmpCursor, &rect),
            L"GetImageLocalBoundsʧ��"
        );
        D2D1_RECT_F cursorRect = {
           targetScreenPos.x - xHotSpot,
           targetScreenPos.y - yHotSpot,
           targetScreenPos.x + rect.right - rect.left - xHotSpot,
           targetScreenPos.y + rect.bottom - rect.top - yHotSpot
        };

        d2dDC->DrawBitmap(bmpCursor, &cursorRect);
    }

    HCURSOR _CreateTransparentCursor(HCURSOR hCursorHotSpot) {
        int len = _cursorSize.cx * _cursorSize.cy;
        BYTE* andPlane = new BYTE[len];
        memset(andPlane, 0xff, len);
        BYTE* xorPlane = new BYTE[len]{};

        auto hotSpot = _GetCursorHotSpot(hCursorHotSpot);

        HCURSOR result = CreateCursor(
            Env::$instance->GetHInstance(),
            hotSpot.first, hotSpot.second,
            _cursorSize.cx, _cursorSize.cy,
            andPlane, xorPlane
        );
        Debug::ThrowIfWin32Failed(result, L"����͸�����ʧ��");

        delete[] andPlane;
        delete[] xorPlane;
        return result;
    }

    std::pair<int, int> _GetCursorHotSpot(HCURSOR hCursor) {
        if (hCursor == NULL) {
            return {};
        }

        ICONINFO ii{};
        Debug::ThrowIfWin32Failed(
            GetIconInfo(hCursor, &ii),
            L"GetIconInfo ʧ��"
        );
        
        DeleteBitmap(ii.hbmColor);
        DeleteBitmap(ii.hbmMask);

        return {
            min((int)ii.xHotspot, (int)_cursorSize.cx),
            min((int)ii.yHotspot, (int)_cursorSize.cy)
        };
    }

    ComPtr<ID2D1Bitmap> _CursorToD2DBitmap(HCURSOR hCursor) {
        assert(hCursor != NULL);

        IWICImagingFactory2* wicImgFactory = Env::$instance->GetWICImageFactory();

        ComPtr<IWICBitmap> wicCursor = nullptr;
        ComPtr<IWICFormatConverter> wicFormatConverter = nullptr;
        ComPtr<ID2D1Bitmap> d2dBmpCursor = nullptr;

        Debug::ThrowIfComFailed(
            wicImgFactory->CreateBitmapFromHICON(hCursor, &wicCursor),
            L"�������ͼ��λͼʧ��"
        );
        Debug::ThrowIfComFailed(
            wicImgFactory->CreateFormatConverter(&wicFormatConverter),
            L"CreateFormatConverter ʧ��"
        );
        Debug::ThrowIfComFailed(
            wicFormatConverter->Initialize(
                wicCursor.Get(),
                GUID_WICPixelFormat32bppPBGRA,
                WICBitmapDitherTypeNone,
                NULL,
                0.f,
                WICBitmapPaletteTypeMedianCut
            ),
            L"IWICFormatConverter ��ʼ��ʧ��"
        );
        Debug::ThrowIfComFailed(
            Env::$instance->GetD2DDC()->CreateBitmapFromWicBitmap(wicFormatConverter.Get(), &d2dBmpCursor),
            L"CreateBitmapFromWicBitmap ʧ��"
        );

        return d2dBmpCursor;
    }


    std::map<HCURSOR, ComPtr<ID2D1Bitmap>> _cursorMap;

    SIZE _cursorSize{};

    RECT _destRect;
    float _scale;

    INT _cursorSpeed = 0;

    bool _debugMode = false;

    static UINT _WM_NEWCURSOR32;
    static UINT _WM_NEWCURSOR64;
};
