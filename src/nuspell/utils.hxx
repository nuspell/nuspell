/* Copyright 2016-2020 Dimitrij Mijoski
 *
 * This file is part of Nuspell.
 *
 * Nuspell is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Nuspell is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Nuspell.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NUSPELL_UTILS_HXX
#define NUSPELL_UTILS_HXX

#include "nuspell_export.h"

#include <clocale>
#include <locale>
#include <string>
#include <string_view>
#include <vector>

#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) ||               \
                         (defined(__APPLE__) && defined(__MACH__)))
#include <unistd.h>
#endif

#include <unicode/locid.h>

#ifdef __GNUC__
#define likely(expr) __builtin_expect(!!(expr), 1)
#define unlikely(expr) __builtin_expect(!!(expr), 0)
#else
#define likely(expr) (expr)
#define unlikely(expr) (expr)
#endif

struct UConverter; // unicode/ucnv.h

namespace nuspell {
inline namespace v4 {

auto split(std::string_view s, char sep, std::vector<std::string>& out)
    -> std::vector<std::string>&;
NUSPELL_EXPORT auto split_on_any_of(std::string_view s, const char* sep,
                                    std::vector<std::string>& out)
    -> std::vector<std::string>&;

NUSPELL_EXPORT auto wide_to_utf8(std::wstring_view in, std::string& out)
    -> void;
NUSPELL_EXPORT auto wide_to_utf8(std::wstring_view in) -> std::string;

auto utf8_to_wide(std::string_view in, std::wstring& out) -> bool;
auto utf8_to_wide(std::string_view in) -> std::wstring;

auto utf8_to_16(std::string_view in) -> std::u16string;
auto utf8_to_16(std::string_view in, std::u16string& out) -> bool;

NUSPELL_EXPORT auto is_all_ascii(std::string_view s) -> bool;

NUSPELL_EXPORT auto latin1_to_ucs2(std::string_view s) -> std::u16string;
auto latin1_to_ucs2(std::string_view s, std::u16string& out) -> void;

NUSPELL_EXPORT auto is_all_bmp(std::u16string_view s) -> bool;

NUSPELL_EXPORT auto to_wide(std::string_view in, const std::locale& inloc,
                            std::wstring& out) -> bool;
NUSPELL_EXPORT auto to_wide(std::string_view in, const std::locale& inloc)
    -> std::wstring;
NUSPELL_EXPORT auto to_narrow(std::wstring_view in, std::string& out,
                              const std::locale& outloc) -> bool;
NUSPELL_EXPORT auto to_narrow(std::wstring_view in, const std::locale& outloc)
    -> std::string;

auto to_upper_ascii(std::string& s) -> void;

[[nodiscard]] NUSPELL_EXPORT auto to_upper(std::wstring_view in,
                                           const icu::Locale& loc)
    -> std::wstring;
[[nodiscard]] NUSPELL_EXPORT auto to_title(std::wstring_view in,
                                           const icu::Locale& loc)
    -> std::wstring;
[[nodiscard]] NUSPELL_EXPORT auto to_lower(std::wstring_view in,
                                           const icu::Locale& loc)
    -> std::wstring;

auto to_upper(std::wstring_view in, const icu::Locale& loc, std::wstring& out)
    -> void;
auto to_title(std::wstring_view in, const icu::Locale& loc, std::wstring& out)
    -> void;
auto to_lower(std::wstring_view in, const icu::Locale& loc, std::wstring& out)
    -> void;
auto to_lower_char_at(std::wstring& s, size_t i, const icu::Locale& loc)
    -> void;
auto to_title_char_at(std::wstring& s, size_t i, const icu::Locale& loc)
    -> void;

/**
 * @internal
 * @brief Enum that identifies the casing type of a word.
 *
 * Neutral characters like numbers are ignored, so "abc" and "abc123abc" are
 * both classified as small.
 */
enum class Casing : char {
	SMALL,
	INIT_CAPITAL,
	ALL_CAPITAL,
	CAMEL /**< @internal camelCase i.e. mixed case with first small */,
	PASCAL /**< @internal  PascalCase i.e. mixed case with first capital */
};

NUSPELL_EXPORT auto classify_casing(std::wstring_view s) -> Casing;

auto has_uppercase_at_compound_word_boundary(std::wstring_view word, size_t i)
    -> bool;

class Encoding_Converter {
	UConverter* cnv = nullptr;

      public:
	Encoding_Converter() = default;
	explicit Encoding_Converter(const char* enc);
	explicit Encoding_Converter(const std::string& enc)
	    : Encoding_Converter(enc.c_str())
	{
	}
	~Encoding_Converter();
	Encoding_Converter(const Encoding_Converter& other);
	Encoding_Converter(Encoding_Converter&& other) noexcept
	{
		cnv = other.cnv;
		cnv = nullptr;
	}
	auto operator=(const Encoding_Converter& other) -> Encoding_Converter&;
	auto operator=(Encoding_Converter&& other) noexcept
	    -> Encoding_Converter&
	{
		std::swap(cnv, other.cnv);
		return *this;
	}
	auto to_wide(std::string_view in, std::wstring& out) -> bool;
	auto to_wide(std::string_view in) -> std::wstring;
	auto valid() -> bool { return cnv != nullptr; }
};

//#if _POSIX_VERSION >= 200809L
#if defined(_POSIX_VERSION) && !defined(__NetBSD__)
class Setlocale_To_C_In_Scope {
	locale_t old_loc = nullptr;

      public:
	Setlocale_To_C_In_Scope()
	    : old_loc{uselocale(newlocale(0, "C", nullptr))}
	{
	}
	~Setlocale_To_C_In_Scope()
	{
		auto new_loc = uselocale(old_loc);
		if (new_loc != old_loc)
			freelocale(new_loc);
	}
	Setlocale_To_C_In_Scope(const Setlocale_To_C_In_Scope&) = delete;
};
#else
class Setlocale_To_C_In_Scope {
	std::string old_name;
#ifdef _WIN32
	int old_per_thread;
#endif
      public:
	Setlocale_To_C_In_Scope() : old_name(setlocale(LC_ALL, nullptr))
	{
#ifdef _WIN32
		old_per_thread = _configthreadlocale(_ENABLE_PER_THREAD_LOCALE);
#endif
		auto x = setlocale(LC_ALL, "C");
		if (!x)
			old_name.clear();
	}
	~Setlocale_To_C_In_Scope()
	{
#ifdef _WIN32
		_configthreadlocale(old_per_thread);
		if (old_per_thread == _ENABLE_PER_THREAD_LOCALE)
#endif
		{
			if (!old_name.empty())
				setlocale(LC_ALL, old_name.c_str());
		}
	}
	Setlocale_To_C_In_Scope(const Setlocale_To_C_In_Scope&) = delete;
};
#endif

auto replace_char(std::wstring& s, wchar_t from, wchar_t to) -> void;
auto erase_chars(std::wstring& s, std::wstring_view erase_chars) -> void;
NUSPELL_EXPORT auto is_number(std::wstring_view s) -> bool;
auto count_appereances_of(std::wstring_view haystack, std::wstring_view needles)
    -> size_t;

auto inline begins_with(std::wstring_view haystack, std::wstring_view needle)
    -> bool
{
	return haystack.compare(0, needle.size(), needle) == 0;
}
auto inline ends_with(std::wstring_view haystack, std::wstring_view needle)
    -> bool
{
	return haystack.size() >= needle.size() &&
	       haystack.compare(haystack.size() - needle.size(), needle.size(),
	                        needle) == 0;
}

template <class T>
auto begin_ptr(T& x)
{
	return x.data();
}
template <class T>
auto end_ptr(T& x)
{
	return x.data() + x.size();
}
} // namespace v4
} // namespace nuspell
#endif // NUSPELL_UTILS_HXX
