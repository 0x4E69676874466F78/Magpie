#pragma once
#include "pch.h"
#include "Utils.h"


class CursorManager {
public:
    CursorManager(
        HINSTANCE hInstance,
        const ComPtr<IWICImagingFactory2>& wicImgFactory,
        const ComPtr<ID2D1DeviceContext>& d2dDC,
        const D2D1_RECT_F& srcRect,
        const D2D1_RECT_F& destRect,
        bool noDisturb = false
    ) : _hInstance(hInstance), _wicImgFactory(wicImgFactory), _d2dDC(d2dDC),
        _srcRect(srcRect), _destRect(destRect), _noDistrub(noDisturb) {
        _cursorSize.cx = GetSystemMetrics(SM_CXCURSOR);
        _cursorSize.cy = GetSystemMetrics(SM_CYCURSOR);

        _systemCursors.hand = CopyCursor(LoadCursor(NULL, IDC_HAND));
        _systemCursors.normal = CopyCursor(LoadCursor(NULL, IDC_ARROW));
        Debug::ThrowIfFalse(_systemCursors.normal && _systemCursors.hand, L"����ϵͳ���ʧ��");

        _d2dBmpNormalCursor = _CursorToD2DBitmap(_systemCursors.normal);
        _d2dBmpHandCursor = _CursorToD2DBitmap(_systemCursors.hand);
        
        if (!noDisturb) {
            HCURSOR tptCursor = _CreateTransparentCursor();
            Debug::ThrowIfFalse(tptCursor, L"����͸�����ʧ��");

            SetSystemCursor(CopyCursor(tptCursor), OCR_HAND);
            SetSystemCursor(tptCursor, OCR_NORMAL);
            // tptCursor ������

            // ��������ڴ�����
            RECT r{ lroundf(srcRect.left), lroundf(srcRect.top), lroundf(srcRect.right), lroundf(srcRect.bottom) };
            Debug::ThrowIfFalse(ClipCursor(&r), L"ClipCursor ʧ��");
            
            // ��������ƶ��ٶ�
            Debug::ThrowIfFalse(
                SystemParametersInfo(SPI_GETMOUSESPEED, 0, &_cursorSpeed, 0),
                L"��ȡ����ٶ�ʧ��"
            );

            float scale = float(destRect.right - destRect.left) / (srcRect.right - srcRect.left);
            long newSpeed = std::clamp((long)lround(_cursorSpeed / scale), 1L, 20L);
            Debug::ThrowIfFalse(
                SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)(intptr_t)newSpeed, 0),
                L"��������ٶ�ʧ��"
            );
        }
	}

	CursorManager(const CursorManager&) = delete;
	CursorManager(CursorManager&&) = delete;

    ~CursorManager() {
        if (!_noDistrub) {
            ClipCursor(nullptr);

            SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)(intptr_t)_cursorSpeed, 0);

            // ��ԭϵͳ���
            SystemParametersInfo(SPI_SETCURSORS, 0, NULL, 0);
            //SetSystemCursor(_systemCursors.normal, OCR_NORMAL);
            //SetSystemCursor(_systemCursors.hand, OCR_HAND);

            // _systemCursors �е���Դ���ͷ�
        }
    }

	void DrawCursor() {
        CURSORINFO ci{};
        ci.cbSize = sizeof(ci);
        GetCursorInfo(&ci);
        
        if (ci.hCursor == NULL) {
            return;
        }
        
        D2D1_POINT_2F targetScreenPos = Utils::MapPoint(
            D2D1_POINT_2F{ (FLOAT)ci.ptScreenPos.x, (FLOAT)ci.ptScreenPos.y },
            _srcRect, _destRect
        );
        // �������Ϊ��������������ģ��
        targetScreenPos.x = roundf(targetScreenPos.x);
        targetScreenPos.y = roundf(targetScreenPos.y);

        ICONINFO ii{};
        GetIconInfo(ci.hCursor, &ii);
        DeleteBitmap(ii.hbmColor);
        DeleteBitmap(ii.hbmMask);

        D2D1_RECT_F cursorRect{
            targetScreenPos.x - ii.xHotspot,
            targetScreenPos.y - ii.yHotspot,
            targetScreenPos.x + _cursorSize.cx - ii.xHotspot,
            targetScreenPos.y + _cursorSize.cy - ii.yHotspot
        };
        
        if (ci.hCursor == LoadCursor(NULL, IDC_ARROW)) {
            _d2dDC->DrawBitmap(_d2dBmpNormalCursor.Get(), &cursorRect);
        } else if (ci.hCursor == LoadCursor(NULL, IDC_HAND)) {
            _d2dDC->DrawBitmap(_d2dBmpHandCursor.Get(), &cursorRect);
        } else {
            try {
                ComPtr<ID2D1Bitmap> c = _CursorToD2DBitmap(ci.hCursor);
                _d2dDC->DrawBitmap(c.Get(), &cursorRect);
            } catch (...) {
                // �������ֻ������ͨ���
                Debug::writeLine(L"_CursorToD2DBitmap ����");
                _d2dDC->DrawBitmap(_d2dBmpNormalCursor.Get(), &cursorRect);
            }
        }
	}

private:
    HCURSOR _CreateTransparentCursor() {
        int len = _cursorSize.cx * _cursorSize.cy;
        BYTE* andPlane = new BYTE[len];
        memset(andPlane, 0xff, len);
        BYTE* xorPlane = new BYTE[len]{};

        HCURSOR _result = CreateCursor(_hInstance, 0, 0, _cursorSize.cx, _cursorSize.cy, andPlane, xorPlane);

        delete[] andPlane;
        delete[] xorPlane;
        return _result;
    }

    ComPtr<ID2D1Bitmap> _CursorToD2DBitmap(HCURSOR hCursor) {
        assert(hCursor != NULL);

        ComPtr<IWICBitmap> wicCursor = nullptr;
        ComPtr<IWICFormatConverter> wicFormatConverter = nullptr;
        ComPtr<ID2D1Bitmap> d2dBmpCursor = nullptr;

        Debug::ThrowIfFailed(
            _wicImgFactory->CreateBitmapFromHICON(hCursor, &wicCursor),
            L"�������ͼ��λͼʧ��"
        );
        Debug::ThrowIfFailed(
            _wicImgFactory->CreateFormatConverter(&wicFormatConverter),
            L"CreateFormatConverter ʧ��"
        );
        Debug::ThrowIfFailed(
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
        Debug::ThrowIfFailed(
            _d2dDC->CreateBitmapFromWicBitmap(wicFormatConverter.Get(), &d2dBmpCursor),
            L"CreateBitmapFromWicBitmap ʧ��"
        );

        return d2dBmpCursor;
    }

    HINSTANCE _hInstance;
    ComPtr<IWICImagingFactory2> _wicImgFactory;
    ComPtr<ID2D1DeviceContext> _d2dDC;

    ComPtr<ID2D1Bitmap> _d2dBmpNormalCursor = nullptr;
    ComPtr<ID2D1Bitmap> _d2dBmpHandCursor = nullptr;

    SIZE _cursorSize{};

    D2D1_RECT_F _srcRect;
    D2D1_RECT_F _destRect;

    struct {
        HCURSOR normal;
        HCURSOR hand;
    } _systemCursors{};

    INT _cursorSpeed = 0;

    bool _noDistrub = false;
};
