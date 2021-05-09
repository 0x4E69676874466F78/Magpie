#pragma once
#include "pch.h"
#include "GUIDs.h"
#include <d2d1effectauthor.h>


// �Զ��� Effect ʹ�õ� Transform ����
// ʵ���� IUnkown �ӿ�
// Ĭ��ʵ�ּ����������Ϊ 1 �Ҳ��ı���״���������뵽�����ӳ�����κμ���
class DrawTransformBase : public ID2D1DrawTransform {
public:
    virtual ~DrawTransformBase() {}

    // ���ɸ��ƣ������ƶ�
    DrawTransformBase(const DrawTransformBase&) = delete;
    DrawTransformBase(DrawTransformBase&&) = delete;

protected:
    // �� hlsl ��ȡ�� Effect Context
    static HRESULT LoadShader(_In_ ID2D1EffectContext* d2dEC, _In_ const wchar_t* path, const GUID& shaderID) {
        if (!d2dEC->IsShaderLoaded(shaderID)) {
            HANDLE hFile = CreateFile(
                path,
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL);
            if (hFile == NULL) {
                Debug::WriteLine(L"��\""s + path + L"\"ʧ��");
                return E_FAIL;
            }

            LARGE_INTEGER liSize{};
            if (!GetFileSizeEx(hFile, &liSize)) {
                Debug::WriteLine(L"��ȡ\""s + path + L"\"�ļ���Сʧ��");
                return E_FAIL;
            }

            DWORD size = (DWORD)liSize.QuadPart;
            BYTE* buf = new BYTE[size];
            DWORD readed = 0;
            if (!ReadFile(hFile, buf, size, &readed, nullptr) || readed == 0) {
                Debug::WriteLine(L"��ȡ\""s + path + L"\"ʧ��");
                return E_FAIL;
            }

            HRESULT hr = d2dEC->LoadPixelShader(shaderID, buf, size);
            delete[] buf;

            if (FAILED(hr)) {
                Debug::WriteLine(L"������ɫ��\""s + path + L"\"ʧ��");
                return hr;
            }
        }

        return S_OK;
    }

public:
    /*
    * ����Ϊ ID2D1DrawTransform �ķ���
    */

    // ��һ�μ��� Effect ʱ������
    IFACEMETHODIMP SetDrawInfo(ID2D1DrawInfo* d2dDrawInfo) override {
        return S_OK;
    }

    /*
    * ����Ϊ ID2D1Transform �ķ���
    */

    // ָ���������
    IFACEMETHODIMP_(UINT32) GetInputCount() const override {
        return 1;
    }

    // D2D ��ÿ����Ⱦʱ���ô˺������������뵽�����ӳ��
    IFACEMETHODIMP MapInputRectsToOutputRect(
        _In_reads_(inputRectCount) const D2D1_RECT_L* pInputRects,
        _In_reads_(inputRectCount) const D2D1_RECT_L* pInputOpaqueSubRects,
        UINT32 inputRectCount,
        _Out_ D2D1_RECT_L* pOutputRect,
        _Out_ D2D1_RECT_L* pOutputOpaqueSubRect
    ) override {
        if (inputRectCount != 1) {
            return E_INVALIDARG;
        }

        // �����״��������ͬ
        *pOutputRect = *pInputRects;
        _inputRect = *pInputRects;

        // �Բ�͸������������
        *pOutputOpaqueSubRect = { 0,0,0,0 };

        return S_OK;
    }

    // ��������������ӳ��
    // �����и����ã���Ϊû����ȷ�ı�����ʱ��
    IFACEMETHODIMP MapOutputRectToInputRects(
        _In_ const D2D1_RECT_L* pOutputRect,
        _Out_writes_(inputRectCount) D2D1_RECT_L* pInputRects,
        UINT32 inputRectCount
    ) const override {
        if (inputRectCount != 1) {
            return E_INVALIDARG;
        }

        // ������״�������ͬ
        pInputRects[0] = _inputRect;

        return S_OK;
    }

    // ��������仯ʱ������ĸ�������Ҫ������Ⱦ
    IFACEMETHODIMP MapInvalidRect(
        UINT32 inputIndex,
        D2D1_RECT_L invalidInputRect,
        _Out_ D2D1_RECT_L* pInvalidOutputRect
    ) const override {
        if (inputIndex != 0) {
            return E_INVALIDARG;
        }

        // �������뵽�����ӳ�����κμ��裬���Խ���Ч������Ϊ�������
        *pInvalidOutputRect = D2D1::RectL(LONG_MIN, LONG_MIN, LONG_MAX, LONG_MAX);

        return S_OK;
    }

    /*
    * ����Ϊ IUnkown �ķ���
    */

    IFACEMETHODIMP_(ULONG) AddRef() override {
        InterlockedIncrement(&_cRef);
        return _cRef;
    }

    IFACEMETHODIMP_(ULONG) Release() override {
        ULONG ulRefCount = InterlockedDecrement(&_cRef);
        if (0 == _cRef) {
            delete this;
        }
        return ulRefCount;
    }

    IFACEMETHODIMP QueryInterface(REFIID riid, _Outptr_ void** ppOutput) override {
        if (!ppOutput)
            return E_INVALIDARG;

        *ppOutput = nullptr;
        if (riid == __uuidof(ID2D1DrawTransform)
            || riid == __uuidof(ID2D1Transform)
            || riid == __uuidof(ID2D1TransformNode)
            || riid == __uuidof(IUnknown)
            ) {
            // ����ָ�룬���Ӽ���
            *ppOutput = static_cast<void*>(this);
            AddRef();
            return NOERROR;
        }
        return E_NOINTERFACE;
    }

protected:
    // ʵ�ֲ��ܹ������캯��
    DrawTransformBase() {}

    // ����������״�� MapOutputRectToInputRects ʹ�ã�������ʹ�� pOutputRect
    // �� https://stackoverflow.com/questions/36920282/pixel-shader-in-direct2d-render-error-along-the-middle
    D2D1_RECT_L _inputRect{};

private:
    // ���ü���
    // ��Ϊ���ü����� 1 ��ʼ�����Դ���ʵ��ʱ���� AddRef
    ULONG _cRef = 1;
};