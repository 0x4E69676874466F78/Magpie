#pragma once
#include "pch.h"
#include "Utils.h"
#include "Renderable.h"
#include "Env.h"
#include "MonochromeCursorEffect.h"

using namespace D2D1;


// ���������Ⱦ
class CursorManager: public Renderable {
public:
    CursorManager() {
        _cursorSize.cx = GetSystemMetrics(SM_CXCURSOR);
        _cursorSize.cy = GetSystemMetrics(SM_CYCURSOR);

        HCURSOR hCursorArrow = LoadCursor(NULL, IDC_ARROW);
        HCURSOR hCursorHand = LoadCursor(NULL, IDC_HAND);
        HCURSOR hCursorAppStarting = LoadCursor(NULL, IDC_APPSTARTING);
        HCURSOR hCursorIBeam = LoadCursor(NULL, IDC_IBEAM);
        
        // �����滻֮ǰ�� arrow ���ͼ��
        // SetSystemCursor ����ı�ϵͳ���ľ��
        _ResolveCursor(hCursorArrow, hCursorArrow);
        _ResolveCursor(hCursorHand, hCursorHand);
        _ResolveCursor(hCursorAppStarting, hCursorAppStarting);
        //_ResolveCursor(hCursorIBeam, hCursorIBeam);

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
        /*Debug::ThrowIfWin32Failed(
            SetSystemCursor(_CreateTransparentCursor(hCursorIBeam), OCR_IBEAM),
            L"���� OCR_APPSTARTING ʧ��"
        );*/

        // ��������ڴ�����
        Debug::ThrowIfWin32Failed(ClipCursor(&Env::$instance->GetSrcClient()), L"ClipCursor ʧ��");

        // ��������ƶ��ٶ�
        Debug::ThrowIfWin32Failed(
            SystemParametersInfo(SPI_GETMOUSESPEED, 0, &_cursorSpeed, 0),
            L"��ȡ����ٶ�ʧ��"
        );

        const RECT& srcClient = Env::$instance->GetSrcClient();
        const D2D_RECT_F& destRect = Env::$instance->GetDestRect();
        float scaleX = (destRect.right - destRect.left) / (srcClient.right - srcClient.left);
        float scaleY = (destRect.bottom - destRect.top) / (srcClient.bottom - srcClient.top);

        long newSpeed = std::clamp(lroundf(_cursorSpeed / (scaleX + scaleY) * 2), 1L, 20L);
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

private:
    struct CursorInfo {
        HCURSOR handle = NULL;
        ComPtr<ID2D1Bitmap> bmp = nullptr;
        int xHotSpot = 0;
        int yHotSpot = 0;
        int width = 0;
        int height = 0;
        bool isMonochrome = false;
    };

    CursorInfo* _cursorInfo = nullptr;
    D2D1_POINT_2L _targetScreenPos{};

public:
    ComPtr<ID2D1Image> RenderEffect(ComPtr<ID2D1Image> input) {
        _CalcCursorPos();

        if (!_cursorInfo || !_cursorInfo->isMonochrome) {
            return input;
        }

        if (!_monochromeCursorEffect) {
            Debug::ThrowIfComFailed(
                MonochromeCursorEffect::Register(Env::$instance->GetD2DFactory()),
                L"ע��MonochromeCursorEffectʧ��"
            );
            Debug::ThrowIfComFailed(
                Env::$instance->GetD2DDC()->CreateEffect(CLSID_MAGPIE_MONOCHROME_CURSOR_EFFECT, &_monochromeCursorEffect),
                L"����MonochromeCursorEffectʧ��"
            );
        }

        _monochromeCursorEffect->SetInput(0, input.Get());
        _monochromeCursorEffect->SetInput(1, _cursorInfo->bmp.Get());

        auto& destRect = Env::$instance->GetDestRect();
        _monochromeCursorEffect->SetValue(
            MonochromeCursorEffect::PROP_CURSOR_POS,
            D2D_VECTOR_2F{ FLOAT(_targetScreenPos.x) - destRect.left, FLOAT(_targetScreenPos.y) - destRect.top }
        );

        ComPtr<ID2D1Image> output;
        _monochromeCursorEffect->GetOutput(&output);
        return output;
    }

    void Render() override {
        if (!_cursorInfo || _cursorInfo->isMonochrome) {
            return;
        }

        D2D1_RECT_F cursorRect = {
            FLOAT(_targetScreenPos.x),
            FLOAT(_targetScreenPos.y),
            FLOAT(_targetScreenPos.x + _cursorInfo->width),
            FLOAT(_targetScreenPos.y + _cursorInfo->height)
        };

        Env::$instance->GetD2DDC()->DrawBitmap(_cursorInfo->bmp.Get(), &cursorRect);
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
    void _CalcCursorPos() {
        CURSORINFO ci{};
        ci.cbSize = sizeof(ci);
        Debug::ThrowIfWin32Failed(
            GetCursorInfo(&ci),
            L"GetCursorInfo ʧ��"
        );

        if (ci.hCursor == NULL) {
            _cursorInfo = nullptr;
            return;
        }

        auto it = _cursorMap.find(ci.hCursor);
        if (it != _cursorMap.end()) {
            _cursorInfo = &it->second;
        } else {
            try {
                // δ��ӳ�����ҵ���������ӳ��
                _ResolveCursor(ci.hCursor, ci.hCursor);

                _cursorInfo = &_cursorMap[ci.hCursor];
            } catch (...) {
                // ������������ƹ��
                _cursorInfo = nullptr;
                Debug::WriteLine(L"Can't find cursor");
                return;
            }
        }

        // ӳ������
        // �������Ϊ��������������ģ��
        const RECT& srcClient = Env::$instance->GetSrcClient();
        const D2D_RECT_F& destRect = Env::$instance->GetDestRect();
        float scaleX = (destRect.right - destRect.left) / (srcClient.right - srcClient.left);
        float scaleY = (destRect.bottom - destRect.top) / (srcClient.bottom - srcClient.top);

        _targetScreenPos = {
            lroundf((ci.ptScreenPos.x - srcClient.left) * scaleX + destRect.left) - _cursorInfo->xHotSpot,
            lroundf((ci.ptScreenPos.y - srcClient.top) * scaleY + destRect.top) - _cursorInfo->yHotSpot
        };
    }

    void _AddHookCursor(HCURSOR hTptCursor, HCURSOR hCursor) {
        if (hTptCursor == NULL || hCursor == NULL) {
            return;
        }

        _ResolveCursor(hTptCursor, hCursor);
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

        return { (int)ii.xHotspot , (int)ii.yHotspot };
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

    ComPtr<ID2D1Bitmap> _MonochromeToD2DBitmap(HBITMAP hbmMask) {
        assert(hbmMask != NULL);

        IWICImagingFactory2* wicImgFactory = Env::$instance->GetWICImageFactory();

        ComPtr<IWICBitmap> wicCursor = nullptr;
        ComPtr<IWICFormatConverter> wicFormatConverter = nullptr;
        ComPtr<ID2D1Bitmap> d2dBmpCursor = nullptr;

        Debug::ThrowIfComFailed(
            wicImgFactory->CreateBitmapFromHBITMAP(hbmMask, NULL, WICBitmapAlphaChannelOption::WICBitmapIgnoreAlpha, &wicCursor),
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

    void _ResolveCursor(HCURSOR hTptCursor, HCURSOR hCursor) {
        assert(hCursor != NULL);

        ICONINFO ii{};
        Debug::ThrowIfWin32Failed(
            GetIconInfo(hCursor, &ii),
            L"GetIconInfo ʧ��"
        );

        CursorInfo cursorInfo;
        cursorInfo.handle = hCursor;
        cursorInfo.xHotSpot = ii.xHotspot;
        cursorInfo.yHotSpot = ii.yHotspot;
        cursorInfo.isMonochrome = (ii.hbmColor == NULL);

        if (!cursorInfo.isMonochrome) {
            SIZE size = _GetSizeOfHBmp(ii.hbmColor);
            cursorInfo.width = size.cx;
            cursorInfo.height = size.cy;

            cursorInfo.bmp = _CursorToD2DBitmap(hCursor);
        } else {
            SIZE size = _GetSizeOfHBmp(ii.hbmMask);
            cursorInfo.width = size.cx;
            cursorInfo.height = size.cy / 2;

            cursorInfo.bmp = _MonochromeToD2DBitmap(ii.hbmMask);
        }

        if (ii.hbmColor) {
            DeleteBitmap(ii.hbmColor);
        }
        DeleteBitmap(ii.hbmMask);

        _cursorMap.emplace(hTptCursor, cursorInfo);
    }

    SIZE _GetSizeOfHBmp(HBITMAP hBmp) {
        BITMAP bmp{};
        Debug::ThrowIfWin32Failed(
            GetObject(hBmp, sizeof(bmp), &bmp),
            L"GetObject ʧ��"
        );
        return { bmp.bmWidth, bmp.bmHeight };
    }
    
    
    std::map<HCURSOR, CursorInfo> _cursorMap;

    SIZE _cursorSize{};

    INT _cursorSpeed = 0;

    ComPtr<ID2D1Effect> _monochromeCursorEffect = nullptr;

    static UINT _WM_NEWCURSOR32;
    static UINT _WM_NEWCURSOR64;
};
