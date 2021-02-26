#pragma once
#include <string>
#include <cassert>

using namespace std::literals::string_literals;


// COM ����ʱ�������쳣
class com_exception : public std::exception {
public:
    com_exception(HRESULT hr) : _result(hr) {}

    const char* what() const override {
        static char s_str[64] = {};
        sprintf_s(s_str, "Failure with HRESULT of %08X",
            static_cast<unsigned int>(_result));
        return s_str;
    }

private:
    HRESULT _result;
};

// ���� WIN32 API ����ʱ�������쳣
class win32_exception : public std::exception {
};

class Debug {
public:
    Debug() = delete;
    Debug(const Debug&) = delete;
    Debug(Debug&&) = delete;

    template<typename T>
    static void writeLine(T msg) {
#ifdef _DEBUG
        writeLine(std::to_wstring(msg));
#endif // _DEBUG
    }

    static void writeLine(std::wstring_view msg) {
#ifdef _DEBUG
        OutputDebugString(L"##DEBUG##: ");
        OutputDebugString(msg.data());
        OutputDebugString(L"\n");
#endif // _DEBUG
    }

    static void writeLine(std::wstring msg) {
#ifdef _DEBUG
        writeLine(std::wstring_view(msg));
#endif // _DEBUG
    }

    static void writeLine(const wchar_t* msg) {
#ifdef _DEBUG
        writeLine(std::wstring_view(msg));
#endif // _DEBUG
    }

    // �� COM �Ĵ���ת��Ϊ�쳣
    static void ThrowIfFailed(HRESULT hr, std::wstring_view failMsg) {
        if (FAILED(hr)) {
            writeLine(std::wstring(failMsg) + L", hr=" + std::to_wstring(static_cast<unsigned int>(hr)));

            throw com_exception(hr);
        }
    }

    // �� WIN32 API �Ĵ���ת�����쳣
    template <typename T>
    static void ThrowIfFalse(T result, std::wstring_view failMsg) {
        if (result) {
            return;
        }

        writeLine(failMsg);
        throw win32_exception();
    }
};
