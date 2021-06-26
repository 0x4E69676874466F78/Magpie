#pragma once
#include <d2d1effectauthor.h>
#include <d2d1effecthelpers.h>


// �Զ��� Effect �Ļ���
class EffectBase : public ID2D1EffectImpl {
public:
    virtual ~EffectBase() {}

    /*
    * ����Ϊ ID2D1EffectImpl �ķ���
    */

    IFACEMETHODIMP Initialize(
        _In_ ID2D1EffectContext* pEffectContext,
        _In_ ID2D1TransformGraph* pTransformGraph
    ) {
        return E_NOTIMPL;
    }

    IFACEMETHODIMP PrepareForRender(D2D1_CHANGE_TYPE changeType) {
        return S_OK;
    }

    IFACEMETHODIMP SetGraph(_In_ ID2D1TransformGraph* pGraph) {
        return E_NOTIMPL;
    }

    /*
    * ����Ϊ IUnkown �ķ���
    */
    
    IFACEMETHODIMP_(ULONG) AddRef() {
        InterlockedIncrement(&_cRef);
        return _cRef;
    }

    IFACEMETHODIMP_(ULONG) Release() {
        ULONG ulRefCount = InterlockedDecrement(&_cRef);
        if (0 == _cRef) {
            delete this;
        }
        return ulRefCount;
    }

    IFACEMETHODIMP QueryInterface(_In_ REFIID riid, _Outptr_ void** ppOutput) {
        // Always set out parameter to NULL, validating it first.
        if (!ppOutput)
            return E_INVALIDARG;

        *ppOutput = NULL;
        if (riid == __uuidof(ID2D1EffectImpl)
            || riid == __uuidof(IUnknown)
            ) {
            // Increment the reference count and return the pointer.
            *ppOutput = (LPVOID)this;
            AddRef();
            return NOERROR;
        }
        return E_NOINTERFACE;
    }

protected:
    // ʵ�ֲ��ܹ������캯��
    EffectBase() {}

private:
    ULONG _cRef = 1;
};
