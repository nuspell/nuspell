/* Copyright 2016-2024 Dimitrij Mijoski
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

#include "defines.hxx"
#include "nuspell_export.h"

#include <string>
#include <string_view>
#include <vector>

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
NUSPELL_BEGIN_INLINE_NAMESPACE

NUSPELL_DEPRECATED_EXPORT auto split_on_any_of(std::string_view s,
                                               const char* sep,
                                               std::vector<std::string>& out)
    -> std::vector<std::string>&;

NUSPELL_EXPORT auto utf32_to_utf8(std::u32string_view in, std::string& out)
    -> void;
NUSPELL_EXPORT auto utf32_to_utf8(std::u32string_view in) -> std::string;

auto valid_utf8_to_32(std::string_view in, std::u32string& out) -> void;
auto valid_utf8_to_32(std::string_view in) -> std::u32string;

auto utf8_to_16(std::string_view in) -> std::u16string;
auto utf8_to_16(std::string_view in, std::u16string& out) -> bool;

auto validate_utf8(std::string_view s) -> bool;

NUSPELL_EXPORT auto is_all_ascii(std::string_view s) -> bool;

NUSPELL_EXPORT auto latin1_to_ucs2(std::string_view s) -> std::u16string;
auto latin1_to_ucs2(std::string_view s, std::u16string& out) -> void;

NUSPELL_EXPORT auto is_all_bmp(std::u16string_view s) -> bool;

auto to_upper_ascii(std::string& s) -> void;

[[nodiscard]] NUSPELL_EXPORT auto to_upper(std::string_view in,
                                           const icu::Locale& loc)
    -> std::string;
[[nodiscard]] NUSPELL_EXPORT auto to_title(std::string_view in,
                                           const icu::Locale& loc)
    -> std::string;
[[nodiscard]] NUSPELL_EXPORT auto to_lower(std::string_view in,
                                           const icu::Locale& loc)
    -> std::string;

auto to_upper(std::string_view in, const icu::Locale& loc, std::string& out)
    -> void;
auto to_title(std::string_view in, const icu::Locale& loc, std::string& out)
    -> void;
auto to_lower(std::u32string_view in, const icu::Locale& loc,
              std::u32string& out) -> void;
auto to_lower(std::string_view in, const icu::Locale& loc, std::string& out)
    -> void;
auto to_lower_char_at(std::string& s, size_t i, const icu::Locale& loc) -> void;
auto to_title_char_at(std::string& s, size_t i, const icu::Locale& loc) -> void;

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

NUSPELL_EXPORT auto classify_casing(std::string_view s) -> Casing;

auto has_uppercase_at_compound_word_boundary(std::string_view word, size_t i)
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
	Encoding_Converter(const Encoding_Converter& other) = delete;
	Encoding_Converter(Encoding_Converter&& other) noexcept
	{
		cnv = other.cnv;
		cnv = nullptr;
	}
	auto operator=(const Encoding_Converter& other)
	    -> Encoding_Converter& = delete;
	auto operator=(Encoding_Converter&& other) noexcept
	    -> Encoding_Converter&
	{
		std::swap(cnv, other.cnv);
		return *this;
	}
	auto to_utf8(std::string_view in, std::string& out) -> bool;
	auto valid() -> bool { return cnv != nullptr; }
};

auto replace_ascii_char(std::string& s, char from, char to) -> void;
auto erase_chars(std::string& s, std::string_view erase_chars) -> void;
NUSPELL_EXPORT auto is_number(std::string_view s) -> bool;
auto count_appereances_of(std::string_view haystack, std::string_view needles)
    -> size_t;

auto inline begins_with(std::string_view haystack, std::string_view needle)
    -> bool
{
	return haystack.compare(0, needle.size(), needle) == 0;
}

auto inline ends_with(std::string_view haystack, std::string_view needle)
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
NUSPELL_END_INLINE_NAMESPACE
} // namespace nuspell
#endif // NUSPELL_UTILS_HXX
