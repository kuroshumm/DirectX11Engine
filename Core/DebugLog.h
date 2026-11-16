#pragma once
#include <cstddef>
#include <string>
#include <string_view>

#ifdef _DEBUG
#include <format>
#include <utility>
#endif

namespace Engine
{
    inline void DebugLog(std::nullptr_t) = delete;

#ifdef _DEBUG
    namespace Detail
    {
        /// <summary>
        /// ワイド文字列をデバッガの出力ウィンドウへ出力する（末尾の改行を自動補完）。
        /// </summary>
        void WriteDebugLogW(std::wstring_view message);

        /// <summary>
        /// UTF-8文字列をワイド文字列へ変換し、デバッガの出力ウィンドウへ出力する。
        /// </summary>
        void WriteDebugLogUtf8(std::string_view message);
    } // namespace Detail

    /// <summary>
    /// C形式ワイド文字列をデバッグ出力する（nullptr安全）。
    /// </summary>
    inline void DebugLog(const wchar_t* message)
    {
        if (message != nullptr)
        {
            Detail::WriteDebugLogW(message);
        }
    }

    /// <summary>
    /// ワイド文字列ビューをそのままデバッグ出力する。
    /// </summary>
    inline void DebugLog(std::wstring_view message)
    {
        Detail::WriteDebugLogW(message);
    }

    /// <summary>
    /// 書式指定付きワイド文字列を整形してデバッグ出力する。
    /// </summary>
    template <typename Arg0, typename... Args> inline void DebugLog(std::wformat_string<Arg0, Args...> fmt, Arg0&& arg0, Args&&... args)
    {
        std::wstring formatted = std::format(fmt, std::forward<Arg0>(arg0), std::forward<Args>(args)...);
        Detail::WriteDebugLogW(formatted);
    }

    /// <summary>
    /// C形式UTF-8文字列をデバッグ出力する（nullptr安全）。
    /// </summary>
    inline void DebugLog(const char* message)
    {
        if (message != nullptr)
        {
            Detail::WriteDebugLogUtf8(message);
        }
    }

    /// <summary>
    /// UTF-8文字列ビューをそのままデバッグ出力する。
    /// </summary>
    inline void DebugLog(std::string_view message)
    {
        Detail::WriteDebugLogUtf8(message);
    }

    /// <summary>
    /// 書式指定付きUTF-8文字列を整形してデバッグ出力する。
    /// </summary>
    template <typename Arg0, typename... Args> inline void DebugLog(std::format_string<Arg0, Args...> fmt, Arg0&& arg0, Args&&... args)
    {
        std::string formatted = std::format(fmt, std::forward<Arg0>(arg0), std::forward<Args>(args)...);
        Detail::WriteDebugLogUtf8(formatted);
    }
#else
    inline void DebugLog(const wchar_t*) noexcept
    {
    }
    inline void DebugLog(std::wstring_view) noexcept
    {
    }
    template <typename Arg0, typename... Args> inline void DebugLog(const wchar_t*, Arg0&&, Args&&...) noexcept
    {
    }

    inline void DebugLog(const char*) noexcept
    {
    }
    inline void DebugLog(std::string_view) noexcept
    {
    }
    template <typename Arg0, typename... Args> inline void DebugLog(const char*, Arg0&&, Args&&...) noexcept
    {
    }
#endif
} // namespace Engine
