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

        _d2dBmpNormalCursor = _CursorToD2DBitmap(LoadCursor(NULL, IDC_ARROW));
        _d2dBmpHandCursor = _CursorToD2DBitmap(LoadCursor(NULL, IDC_HAND));
        
        if (!noDisturb) {
            HCURSOR tptCursor = _CreateTransparentCursor();

            Debug::ThrowIfWin32Failed(
                SetSystemCursor(CopyCursor(tptCursor), OCR_HAND),
                L"���� OCR_HAND ʧ��"
            );
            Debug::ThrowIfWin32Failed(
                SetSystemCursor(tptCursor, OCR_NORMAL),
                L"���� OCR_NORMAL ʧ��"
            );
            // tptCursor ������

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
	}

	CursorManager(const CursorManager&) = delete;
	CursorManager(CursorManager&&) = delete;

    ~CursorManager() {
        if (!_noDistrub) {
            ClipCursor(nullptr);

            SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)(intptr_t)_cursorSpeed, 0);

            // ��ԭϵͳ���
            SystemParametersInfo(SPI_SETCURSORS, 0, NULL, 0);
        }
    }

	void DrawCursor() {
        CURSORINFO ci{};
        ci.cbSize = sizeof(ci);
        Debug::ThrowIfWin32Failed(GetCursorInfo(&ci), L"GetCursorInfo ʧ��");
        
        if (ci.hCursor == NULL) {
            return;
        }
        
        // ӳ������
        float factor = (_destRect.right - _destRect.left) / (_srcRect.right - _srcRect.left);
        D2D1_POINT_2F targetScreenPos{
            ((FLOAT)ci.ptScreenPos.x - _srcRect.left) * factor + _destRect.left,
            ((FLOAT)ci.ptScreenPos.y - _srcRect.top) * factor + _destRect.top
        };
        // �������Ϊ��������������ģ��
        targetScreenPos.x = roundf(targetScreenPos.x);
        targetScreenPos.y = roundf(targetScreenPos.y);


        ICONINFO ii{};
        Debug::ThrowIfWin32Failed(GetIconInfo(ci.hCursor, &ii), L"GetIconInfo ʧ��");
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
            } catch (magpie_exception) {
                // �������ֻ������ͨ���
                Debug::WriteLine(L"_CursorToD2DBitmap ����");
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

        HCURSOR result = CreateCursor(_hInstance, 0, 0, _cursorSize.cx, _cursorSize.cy, andPlane, xorPlane);
        Debug::ThrowIfWin32Failed(result, L"����͸�����ʧ��");

        delete[] andPlane;
        delete[] xorPlane;
        return result;
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

    INT _cursorSpeed = 0;

    bool _noDistrub = false;
};
