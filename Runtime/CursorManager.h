#pragma once
#include "pch.h"
#include "Utils.h"
#include "Renderable.h"


class CursorManager: public Renderable {
public:
    CursorManager(
        D2DContext& d2dContext,
        HINSTANCE hInstance,
        IWICImagingFactory2* wicImgFactory,
        const D2D1_RECT_F& srcRect,
        const D2D1_RECT_F& destRect,
        bool noDisturb = false,
        bool debugMode = false
    ) : Renderable(d2dContext), _hInstance(hInstance), _wicImgFactory(wicImgFactory),
        _srcRect(srcRect), _destRect(destRect), _noDisturb(noDisturb), _debugMode(debugMode) {
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

        if (_noDisturb) {
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
        RECT r{ lroundf(srcRect.left), lroundf(srcRect.top), lroundf(srcRect.right), lroundf(srcRect.bottom) };
        Debug::ThrowIfWin32Failed(ClipCursor(&r), L"ClipCursor ʧ��");

        // ��������ƶ��ٶ�
        Debug::ThrowIfWin32Failed(
            SystemParametersInfo(SPI_GETMOUSESPEED, 0, &_cursorSpeed, 0),
            L"��ȡ����ٶ�ʧ��"
        );

        float scale = float(destRect.right - destRect.left) / (srcRect.right - srcRect.left);
        long newSpeed = std::clamp(lroundf(_cursorSpeed / scale), 1L, 20L);
        Debug::ThrowIfWin32Failed(
            SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)(intptr_t)newSpeed, 0),
            L"��������ٶ�ʧ��"
        );
    }

	CursorManager(const CursorManager&) = delete;
	CursorManager(CursorManager&&) = delete;

    ~CursorManager() {
        if (_noDisturb) {
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

    void AddHookCursor(HCURSOR hTptCursor, HCURSOR hCursor) {
        if (hTptCursor == NULL || hCursor == NULL) {
            return;
        }

        _cursorMap[hTptCursor] = _CursorToD2DBitmap(hCursor);
    }
private:
    void _RenderDebugLayer(HCURSOR hCursorCur) {
        D2D1_POINT_2F leftTop{ 0, 50 };

        D2D1_RECT_F cursorRect{
            leftTop.x, leftTop.y, leftTop.x + _cursorSize.cx, leftTop.y + _cursorSize.cy
        };

        ComPtr<ID2D1SolidColorBrush> brush;
        _d2dContext.GetD2DDC()->CreateSolidColorBrush({ 0,0,0,1 }, &brush);

        for (const auto& a : _cursorMap) {
            _d2dContext.GetD2DDC()->FillRectangle(cursorRect, brush.Get());

            if (a.first != hCursorCur) {
                _d2dContext.GetD2DDC()->DrawBitmap(a.second.Get(), &cursorRect);
            }

            cursorRect.left += _cursorSize.cx;
            cursorRect.right += _cursorSize.cy;
        }
    }

    void _DrawCursor(HCURSOR hCursor, POINT ptScreenPos) {
        assert(hCursor != NULL);

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
        float factor = (_destRect.right - _destRect.left) / (_srcRect.right - _srcRect.left);
        D2D1_POINT_2F targetScreenPos{
            ((FLOAT)ptScreenPos.x - _srcRect.left) * factor + _destRect.left,
            ((FLOAT)ptScreenPos.y - _srcRect.top) * factor + _destRect.top
        };

        // �������Ϊ��������������ģ��
        targetScreenPos.x = roundf(targetScreenPos.x);
        targetScreenPos.y = roundf(targetScreenPos.y);

        auto [xHotSpot, yHotSpot] = _GetCursorHotSpot(hCursor);

        D2D1_RECT_F cursorRect = {
           targetScreenPos.x - xHotSpot,
           targetScreenPos.y - yHotSpot,
           targetScreenPos.x + _cursorSize.cx - xHotSpot,
           targetScreenPos.y + _cursorSize.cy - yHotSpot
        };

        _d2dContext.GetD2DDC()->DrawBitmap(bmpCursor, &cursorRect);
    }

    HCURSOR _CreateTransparentCursor(HCURSOR hCursorHotSpot) {
        int len = _cursorSize.cx * _cursorSize.cy;
        BYTE* andPlane = new BYTE[len];
        memset(andPlane, 0xff, len);
        BYTE* xorPlane = new BYTE[len]{};

        auto hotSpot = _GetCursorHotSpot(hCursorHotSpot);

        HCURSOR result = CreateCursor(
            _hInstance,
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

        ComPtr<IWICBitmap> wicCursor = nullptr;
        ComPtr<IWICFormatConverter> wicFormatConverter = nullptr;
        ComPtr<ID2D1Bitmap> d2dBmpCursor = nullptr;

        Debug::ThrowIfComFailed(
            _wicImgFactory->CreateBitmapFromHICON(hCursor, &wicCursor),
            L"�������ͼ��λͼʧ��"
        );
        Debug::ThrowIfComFailed(
            _wicImgFactory->CreateFormatConverter(&wicFormatConverter),
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
            _d2dContext.GetD2DDC()->CreateBitmapFromWicBitmap(wicFormatConverter.Get(), &d2dBmpCursor),
            L"CreateBitmapFromWicBitmap ʧ��"
        );

        return d2dBmpCursor;
    }

    HINSTANCE _hInstance;
    IWICImagingFactory2* _wicImgFactory;

    std::map<HCURSOR, ComPtr<ID2D1Bitmap>> _cursorMap;

    SIZE _cursorSize{};

    D2D1_RECT_F _srcRect;
    D2D1_RECT_F _destRect;

    INT _cursorSpeed = 0;

    bool _noDisturb = false;
    bool _debugMode = false;
};
