#pragma once
#include "pch.h"
#include "DrawTransformBase.h"

// ����ȡ���򵥵� DrawTransform
// �� DrawTransform ������������������
//  * һ������
//  * ���������ߴ���ͬ
//  * ֻ�Ǽ򵥵ض�����Ӧ����һ��������ɫ��
//  * ��״̬
class SimpleDrawTransform : public DrawTransformBase {
protected:
    SimpleDrawTransform(const GUID &shaderID): _shaderID(shaderID) {}

public:
    virtual ~SimpleDrawTransform() {}

    static HRESULT Create(
        _In_ ID2D1EffectContext* d2dEC, 
        _Outptr_ SimpleDrawTransform** ppOutput,
        _In_ const wchar_t* shaderPath, 
        const GUID &shaderID
    ) {
        if (!ppOutput) {
            return E_INVALIDARG;
        }

        HRESULT hr = DrawTransformBase::LoadShader(d2dEC, shaderPath, shaderID);
        if (FAILED(hr)) {
            return hr;
        }

        *ppOutput = new SimpleDrawTransform(shaderID);
        return S_OK;
    }

    IFACEMETHODIMP MapInputRectsToOutputRect(
        _In_reads_(inputRectCount) const D2D1_RECT_L* pInputRects,
        _In_reads_(inputRectCount) const D2D1_RECT_L* pInputOpaqueSubRects,
        UINT32 inputRectCount,
        _Out_ D2D1_RECT_L* pOutputRect,
        _Out_ D2D1_RECT_L* pOutputOpaqueSubRect
    ) override {
        HRESULT hr = DrawTransformBase::MapInputRectsToOutputRect(
            pInputRects, 
            pInputOpaqueSubRects, 
            inputRectCount, 
            pOutputRect, 
            pOutputOpaqueSubRect
        );
        if (FAILED(hr)) {
            return hr;
        }

        SetShaderContantBuffer(SIZE {
            pInputRects->right - pInputRects->left,
            pInputRects->bottom - pInputRects->top
        });

        return S_OK;
    }

    IFACEMETHODIMP SetDrawInfo(ID2D1DrawInfo* pDrawInfo) override {
        _drawInfo = pDrawInfo;
        return pDrawInfo->SetPixelShader(_shaderID);
    }

protected:
    // �̳е�����Ը��Ǵ˷�������ɫ�����ݲ���
    virtual void SetShaderContantBuffer(const SIZE& srcSize) {
        struct {
            INT32 srcWidth;
            INT32 srcHeight;
        } shaderConstants{
            srcSize.cx,
            srcSize.cy
        };

        _drawInfo->SetPixelShaderConstantBuffer((BYTE*)&shaderConstants, sizeof(shaderConstants));
    }

    ComPtr<ID2D1DrawInfo> _drawInfo = nullptr;

private:
    const GUID& _shaderID;
};
