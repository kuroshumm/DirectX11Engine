#ifdef _DEBUG

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "DebugLog.h"

namespace Engine::Detail
{
    void WriteDebugLogW(std::wstring_view message)
    {
        if (message.empty())
        {
            OutputDebugStringW(L"\n");
            return;
        }

        // 呼び出し側での改行有無に関わらず、出力ウィンドウ上で1メッセージ1行に揃える
        std::wstring buffer(message);
        if (buffer.back() != L'\n')
        {
            buffer.push_back(L'\n');
        }
        OutputDebugStringW(buffer.c_str());
    }

    void WriteDebugLogUtf8(std::string_view message)
    {
        if (message.empty())
        {
            OutputDebugStringW(L"\n");
            return;
        }

        const int inputLen = static_cast<int>(message.size());
        // /utf-8 ビルド時の文字リテラルは UTF-8 として変換し、非UTF-8文字列のみ CP_ACP へフォールバックする
        UINT codePage = CP_UTF8;
        int wideLen = MultiByteToWideChar(codePage, MB_ERR_INVALID_CHARS, message.data(), inputLen, nullptr, 0);
        if (wideLen <= 0)
        {
            codePage = CP_ACP;
            wideLen = MultiByteToWideChar(codePage, 0, message.data(), inputLen, nullptr, 0);
        }
        if (wideLen <= 0)
        {
            return;
        }

        std::wstring wideBuffer(static_cast<size_t>(wideLen), L'\0');
        MultiByteToWideChar(codePage, 0, message.data(), inputLen, wideBuffer.data(), wideLen);
        WriteDebugLogW(wideBuffer);
    }
} // namespace Engine::Detail

#endif
