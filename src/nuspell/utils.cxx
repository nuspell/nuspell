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

#include "utils.hxx"

#include <algorithm>
#include <limits>

#include <unicode/uchar.h>
#include <unicode/ucnv.h>
#include <unicode/unistr.h>
#include <unicode/ustring.h>
#include <unicode/utf16.h>
#include <unicode/utf8.h>

#if ' ' != 32 || '.' != 46 || 'A' != 65 || 'Z' != 90 || 'a' != 97 || 'z' != 122
#error "Basic execution character set is not ASCII"
#elif L' ' != 32 || L'.' != 46 || L'A' != 65 || L'Z' != 90 || L'a' != 97 ||    \
    L'z' != 122
#error "Basic wide execution character set is not ASCII-compatible"
#endif

using namespace std;

namespace nuspell {
inline namespace v4 {

template <class SepT>
static auto& split_on_any_of_low(std::string_view s, const SepT& sep,
                                 std::vector<std::string>& out)
{
	size_t i1 = 0;
	size_t i2;
	do {
		i2 = s.find_first_of(sep, i1);
		out.push_back(std::string(s.substr(i1, i2 - i1)));
		i1 = i2 + 1; // we can only add +1 if sep is single char.

		// i2 gets s.npos after the last separator.
		// Length of i2-i1 will always go past the end. That is defined.
	} while (i2 != s.npos);
	return out;
}

/**
 * @internal
 * @brief Splits string on single char seperator.
 *
 * Consecutive separators are treated as separate and will emit empty strings.
 *
 * @param s string to split.
 * @param sep char that acts as separator to split on.
 * @param out vector where separated strings are appended.
 * @return @p out.
 */
auto split(std::string_view s, char sep, std::vector<std::string>& out)
    -> std::vector<std::string>&
{
	return split_on_any_of_low(s, sep, out);
}

/**
 * @internal
 * @brief Splits string on set of single char seperators.
 *
 * Consecutive separators are treated as separate and will emit empty strings.
 *
 * @param s string to split.
 * @param sep seperator(s) to split on.
 * @param out vector where separated strings are appended.
 * @return @p out.
 */
auto split_on_any_of(std::string_view s, const char* sep,
                     std::vector<std::string>& out) -> std::vector<std::string>&
{
	return split_on_any_of_low(s, sep, out);
}

template <class CharT>
struct UTF8_Traits {
	static_assert(sizeof(CharT) == 1);

	using String_View = std::basic_string_view<CharT>;

	static constexpr size_t max_width = U8_MAX_LENGTH;

	struct Encoded_CP {
		CharT seq[max_width];
		size_t size = 0;
		Encoded_CP(char32_t cp) { U8_APPEND_UNSAFE(seq, size, cp); }
	};
	UTF8_Traits() = delete;
	auto static decode(String_View s, size_t& i) -> int32_t
	{
		int32_t c;
#if U_ICU_VERSION_MAJOR_NUM <= 60
		auto s_ptr = s.data();
		int32_t idx = i;
		int32_t len = s.size();
		U8_NEXT(s_ptr, idx, len, c);
		i = idx;
#else
		auto len = s.size();
		U8_NEXT(s, i, len, c);
#endif
		return c;
	}
	auto static is_decoded_cp_error(int32_t cp) -> bool { return cp < 0; }
	auto static decode_valid(String_View s, size_t& i) -> int32_t
	{
		int32_t c;
		U8_NEXT_UNSAFE(s, i, c);
		return c;
	}
	auto static encode_valid(char32_t cp) -> Encoded_CP { return cp; }
};

template <class CharT>
struct UTF16_Traits {
	static_assert(sizeof(CharT) == 2);

	using String_View = std::basic_string_view<CharT>;

	static constexpr size_t max_width = U16_MAX_LENGTH;
	struct Encoded_CP {
		CharT seq[max_width];
		size_t size = 0;
		Encoded_CP(char32_t cp) { U16_APPEND_UNSAFE(seq, size, cp); }
	};
	UTF16_Traits() = delete;
	auto static decode(String_View s, size_t& i) -> int32_t
	{
		auto len = s.size();
		int32_t c;
		U16_NEXT(s, i, len, c);
		return c;
	}
	auto static is_decoded_cp_error(int32_t cp) -> bool
	{
		return U_IS_SURROGATE(cp);
	}
	auto static decode_valid(String_View s, size_t& i) -> int32_t
	{
		int32_t c;
		U16_NEXT_UNSAFE(s, i, c);
		return c;
	}
	auto static encode_valid(char32_t cp) -> Encoded_CP { return cp; }
};

template <class CharT>
struct UTF32_Traits {
	static_assert(sizeof(CharT) == 4);

	using String_View = std::basic_string_view<CharT>;

	static constexpr size_t max_width = 1;
	struct Encoded_CP {
		CharT seq[1];
		static constexpr size_t size = 1;
		Encoded_CP(char32_t cp) { seq[0] = cp; }
	};
	UTF32_Traits() = delete;
	auto static decode(String_View s, size_t& i) -> int32_t
	{
		return s[i++];
	}
	auto static is_decoded_cp_error(int32_t cp) -> bool
	{
		return !(0 <= cp && cp <= 0x10ffff);
	}
	auto static decode_valid(String_View s, size_t& i) -> int32_t
	{
		return s[i++];
	}
	auto static encode_valid(char32_t cp) -> Encoded_CP { return cp; }
};

template <class CharT>
struct UTF_Traits;

template <>
struct UTF_Traits<char> : UTF8_Traits<char> {
};
template <>
struct UTF_Traits<char16_t> : UTF16_Traits<char16_t> {
};
#if U_SIZEOF_WCHAR_T == 4
template <>
struct UTF_Traits<wchar_t> : UTF32_Traits<wchar_t> {
};
#elif U_SIZEOF_WCHAR_T == 2
template <>
struct UTF_Traits<wchar_t> : UTF16_Traits<wchar_t> {
};
#endif

enum class Utf_Error_Handling { ALWAYS_VALID, REPLACE, SKIP };

template <Utf_Error_Handling eh, class InChar, class OutChar>
auto static utf_to_utf(std::basic_string_view<InChar> in,
                       std::basic_string<OutChar>& out) -> bool
{
	using UEH = Utf_Error_Handling;

	out.clear();
	if (in.size() > out.capacity())
		out.reserve(in.size());
	auto valid = true;
	for (size_t i = 0; i != in.size();) {
		auto cp = int32_t();
		if (eh == UEH::ALWAYS_VALID) {
			cp = UTF_Traits<InChar>::decode_valid(in, i);
		}
		else {
			cp = UTF_Traits<InChar>::decode(in, i);
			if (unlikely(
			        UTF_Traits<InChar>::is_decoded_cp_error(cp))) {
				valid = false;
				if (eh == UEH::SKIP)
					continue;
				else if (eh == UEH::REPLACE)
					cp = 0xFFFD;
			}
		}
		auto encoded_cp = UTF_Traits<OutChar>::encode_valid(cp);
		out.append(encoded_cp.seq, encoded_cp.size);
	}
	return valid;
}

template <class InChar, class OutChar>
auto static valid_utf_to_utf(std::basic_string_view<InChar> in,
                             std::basic_string<OutChar>& out) -> void
{
	utf_to_utf<Utf_Error_Handling::ALWAYS_VALID>(in, out);
}

template <class InChar, class OutChar>
auto static utf_to_utf_replace_err(std::basic_string_view<InChar> in,
                                   std::basic_string<OutChar>& out) -> bool
{
	return utf_to_utf<Utf_Error_Handling::REPLACE>(in, out);
}

auto wide_to_utf8(std::wstring_view in, std::string& out) -> void
{
#if U_SIZEOF_WCHAR_T == 4
	valid_utf_to_utf(in, out);
#else
	// TODO: remove the ifdefs once we move to char32_t
	// This is needed for Windows, as there, wchar_t strings are UTF-16 and
	// in our suggestion routines, where we operate on single wchar_t values
	// (code units), we can easlt get faulty surrogate pair.
	// On Linux where wchar_t is UTF-32, a single code unit (single wchar_t
	// value) is also a code point and there (on Linux) we can assume all
	// wchar_t strings are always valid.
	utf_to_utf_replace_err(in, out);
#endif
}
auto wide_to_utf8(std::wstring_view in) -> std::string
{
	auto out = string();
	wide_to_utf8(in, out);
	return out;
}

auto utf8_to_wide(std::string_view in, std::wstring& out) -> bool
{
	return utf_to_utf_replace_err(in, out);
}
auto utf8_to_wide(std::string_view in) -> std::wstring
{
	auto out = wstring();
	utf_to_utf_replace_err(in, out);
	return out;
}

auto utf8_to_16(std::string_view in) -> std::u16string
{
	auto out = u16string();
	utf_to_utf_replace_err(in, out);
	return out;
}

bool utf8_to_16(std::string_view in, std::u16string& out)
{
	return utf_to_utf_replace_err(in, out);
}

auto static is_ascii(char c) -> bool
{
	return static_cast<unsigned char>(c) <= 127;
}

auto is_all_ascii(std::string_view s) -> bool
{
	return all_of(begin(s), end(s), is_ascii);
}

auto static widen_latin1(char c) -> char16_t
{
	return static_cast<unsigned char>(c);
}

auto latin1_to_ucs2(std::string_view s) -> std::u16string
{
	u16string ret;
	latin1_to_ucs2(s, ret);
	return ret;
}
auto latin1_to_ucs2(std::string_view s, std::u16string& out) -> void
{
	out.resize(s.size());
	transform(begin(s), end(s), begin(out), widen_latin1);
}

auto static is_surrogate_pair(char16_t c) -> bool
{
	return 0xD800 <= c && c <= 0xDFFF;
}
auto is_all_bmp(std::u16string_view s) -> bool
{
	return none_of(begin(s), end(s), is_surrogate_pair);
}

auto to_wide(std::string_view in, const std::locale& loc, std::wstring& out)
    -> bool
{
	auto& cvt = use_facet<codecvt<wchar_t, char, mbstate_t>>(loc);
	out.resize(in.size(), L'\0');
	auto state = mbstate_t();
	auto in_ptr = begin_ptr(in);
	auto in_last = end_ptr(in);
	auto out_ptr = begin_ptr(out);
	auto out_last = end_ptr(out);
	auto valid = true;
	for (;;) {
		auto err = cvt.in(state, in_ptr, in_last, in_ptr, out_ptr,
		                  out_last, out_ptr);
		if (err == cvt.ok || err == cvt.noconv)
			break;
#if U_SIZEOF_WCHAR_T == 4
		auto no_space = out_ptr == out_last;
#elif U_SIZEOF_WCHAR_T == 2
		auto no_space = out_last - out_ptr < 2;
#endif
		if (err == cvt.partial && no_space) {
			// no space in output buf
			auto idx = out_ptr - begin_ptr(out);
			out.resize(out.size() * 2);
			out_ptr = &out[idx];
			out_last = end_ptr(out);
		}
		else if (err == cvt.partial && !no_space) {
			// incomplete sequence at the end
			*out_ptr++ = L'\uFFFD';
			valid = false;
			break;
		}
		else if (err == cvt.error) {
			if (out_ptr == out_last) {
				auto idx = out_ptr - begin_ptr(out);
				out.resize(out.size() * 2);
				out_ptr = &out[idx];
				out_last = end_ptr(out);
			}
			in_ptr++;
			*out_ptr++ = L'\uFFFD';
			valid = false;
		}
	}
	out.erase(out_ptr - begin_ptr(out));
	return valid;
}

auto to_wide(std::string_view in, const std::locale& loc) -> std::wstring
{
	auto ret = wstring();
	to_wide(in, loc, ret);
	return ret;
}

auto to_narrow(std::wstring_view in, std::string& out, const std::locale& loc)
    -> bool
{
	auto& cvt = use_facet<codecvt<wchar_t, char, mbstate_t>>(loc);
	out.resize(in.size(), '\0');
	auto state = mbstate_t();
	auto in_ptr = begin_ptr(in);
	auto in_last = end_ptr(in);
	auto out_ptr = begin_ptr(out);
	auto out_last = end_ptr(out);
	auto max_len_for_CP = cvt.max_length();
	auto valid = true;
	for (;;) {
		auto err = cvt.out(state, in_ptr, in_last, in_ptr, out_ptr,
		                   out_last, out_ptr);
		if (err == cvt.ok || err == cvt.noconv)
			break;
		auto no_space = out_last - out_ptr < max_len_for_CP;
		if (err == cvt.partial && no_space) {
			// no space in output buf
			auto idx = out_ptr - begin_ptr(out);
			out.resize(out.size() * 2);
			out_ptr = &out[idx];
			out_last = end_ptr(out);
		}
		else if (err == cvt.partial && !no_space) {
			// incomplete sequence at the end
			*out_ptr++ = '?';
			valid = false;
			break;
		}
		else if (err == cvt.error) {
			if (out_ptr == out_last) {
				auto idx = out_ptr - begin_ptr(out);
				out.resize(out.size() * 2);
				out_ptr = &out[idx];
				out_last = end_ptr(out);
			}
			in_ptr++;
			*out_ptr++ = '?';
			valid = false;
		}
	}
	out.erase(out_ptr - begin_ptr(out));
	return valid;
}

auto to_narrow(std::wstring_view in, const std::locale& loc) -> std::string
{
	auto ret = string();
	to_narrow(in, ret, loc);
	return ret;
}

auto to_upper_ascii(std::string& s) -> void
{
	auto& char_type = use_facet<ctype<char>>(locale::classic());
	char_type.toupper(begin_ptr(s), end_ptr(s));
}

auto static wide_to_icu(wstring_view in) -> icu::UnicodeString
{
#if U_SIZEOF_WCHAR_T == 2
	// TODO: consider using aliasing features of UnicodeString
	return icu::UnicodeString(in.data(), in.size());
#elif U_SIZEOF_WCHAR_T == 4
	return icu::UnicodeString::fromUTF32(
	    reinterpret_cast<const UChar32*>(in.data()), in.size());
#else
#error "wchar_t can not fit utf-16 or utf-32"
#endif
}
auto static icu_to_wide(const icu::UnicodeString& in, std::wstring& out) -> bool
{
	out.resize(in.length());
#if U_SIZEOF_WCHAR_T == 2
	in.extract(0, in.length(), out.data());
	return true;
#elif U_SIZEOF_WCHAR_T == 4
	auto err = U_ZERO_ERROR;
	auto len =
	    in.toUTF32(reinterpret_cast<UChar32*>(out.data()), out.size(), err);
	if (U_SUCCESS(err)) {
		out.erase(len);
		return true;
	}
	out.clear();
	return false;
#endif
}

auto to_upper(wstring_view in, const icu::Locale& loc) -> std::wstring
{
	auto out = wstring();
	to_upper(in, loc, out);
	return out;
}
auto to_title(wstring_view in, const icu::Locale& loc) -> std::wstring
{
	auto out = wstring();
	to_title(in, loc, out);
	return out;
}
auto to_lower(wstring_view in, const icu::Locale& loc) -> std::wstring
{
	auto out = wstring();
	to_lower(in, loc, out);
	return out;
}

auto to_upper(wstring_view in, const icu::Locale& loc, wstring& out) -> void
{
	auto us = wide_to_icu(in);
	us.toUpper(loc);
	icu_to_wide(us, out);
}
auto to_title(wstring_view in, const icu::Locale& loc, wstring& out) -> void
{
	auto us = wide_to_icu(in);
	us.toTitle(nullptr, loc);
	icu_to_wide(us, out);
}
auto to_lower(wstring_view in, const icu::Locale& loc, wstring& out) -> void
{
	auto us = wide_to_icu(in);
	us.toLower(loc);
	icu_to_wide(us, out);
}

auto to_lower_char_at(std::wstring& s, size_t i, const icu::Locale& loc) -> void
{
	auto us = icu::UnicodeString(UChar32(s[i]));
	us.toLower(loc);
	if (likely(us.length() == 1)) {
		s[i] = us[0];
		return;
	}
	auto ws = wstring();
	icu_to_wide(us, ws);
	s.replace(i, 1, ws);
}
auto to_title_char_at(std::wstring& s, size_t i, const icu::Locale& loc) -> void
{
	auto us = icu::UnicodeString(UChar32(s[i]));
	us.toTitle(nullptr, loc);
	if (likely(us.length() == 1)) {
		s[i] = us[0];
		return;
	}
	auto ws = wstring();
	icu_to_wide(us, ws);
	s.replace(i, 1, ws);
}

/**
 * @internal
 * @brief Determines casing (capitalization) type for a word.
 *
 * Casing is sometimes referred to as capitalization.
 *
 * @param s word.
 * @return The casing type.
 */
auto classify_casing(wstring_view s) -> Casing
{
	// TODO implement Default Case Detection from unicode standard
	// https://www.unicode.org/versions/Unicode11.0.0/ch03.pdf
	// See Chapter 13.3. This might be feature for ICU.

	size_t upper = 0;
	size_t lower = 0;
	for (auto& c : s) {
		if (u_isupper(c))
			upper++;
		else if (u_islower(c))
			lower++;
		// else neutral
	}
	if (upper == 0)               // all lowercase, maybe with some neutral
		return Casing::SMALL; // most common case

	auto first_capital = u_isupper(s[0]);
	if (first_capital && upper == 1)
		return Casing::INIT_CAPITAL; // second most common

	if (lower == 0)
		return Casing::ALL_CAPITAL;

	if (first_capital)
		return Casing::PASCAL;
	else
		return Casing::CAMEL;
}

/**
 * @internal
 * @brief Check if word[i] or word[i-1] are uppercase
 *
 * Check if the two chars are alphabetic and at least one of them is in
 * uppercase.
 *
 * @return true if at least one is uppercase, false otherwise.
 */
auto has_uppercase_at_compound_word_boundary(wstring_view word, size_t i)
    -> bool
{
	if (u_isupper(word[i])) {
		if (u_isalpha(word[i - 1]))
			return true;
	}
	else if (u_isupper(word[i - 1]) && u_isalpha(word[i]))
		return true;
	return false;
}

Encoding_Converter::Encoding_Converter(const char* enc)
{
	auto err = UErrorCode();
	cnv = ucnv_open(enc, &err);
}

Encoding_Converter::~Encoding_Converter()
{
	if (cnv)
		ucnv_close(cnv);
}

Encoding_Converter::Encoding_Converter(const Encoding_Converter& other)
{
	auto err = UErrorCode();
	cnv = ucnv_safeClone(other.cnv, nullptr, nullptr, &err);
}

auto Encoding_Converter::operator=(const Encoding_Converter& other)
    -> Encoding_Converter&
{
	this->~Encoding_Converter();
	auto err = UErrorCode();
	cnv = ucnv_safeClone(other.cnv, nullptr, nullptr, &err);
	return *this;
}

auto Encoding_Converter::to_wide(string_view in, wstring& out) -> bool
{
	if (ucnv_getType(cnv) == UCNV_UTF8)
		return utf8_to_wide(in, out);

	auto err = U_ZERO_ERROR;
	auto us = icu::UnicodeString(in.data(), in.size(), cnv, err);
	if (U_FAILURE(err)) {
		out.clear();
		return false;
	}
	if (icu_to_wide(us, out))
		return true;
	return false;
}

auto Encoding_Converter::to_wide(string_view in) -> wstring
{
	auto out = wstring();
	this->to_wide(in, out);
	return out;
}

auto replace_char(wstring& s, wchar_t from, wchar_t to) -> void
{
	for (auto i = s.find(from); i != s.npos; i = s.find(from, i + 1)) {
		s[i] = to;
	}
}

auto erase_chars(wstring& s, wstring_view erase_chars) -> void
{
	if (erase_chars.empty())
		return;
	auto is_erasable = [&](auto c) {
		return erase_chars.find(c) != erase_chars.npos;
	};
	auto it = remove_if(begin(s), end(s), is_erasable);
	s.erase(it, end(s));
	return;
}

/**
 * @internal
 * @brief Tests if word is a number.
 *
 * Allow numbers with dot ".", dash "-" or comma "," inbetween the digits, but
 * forbids double separators such as "..", "--" and ".,".
 */
auto is_number(wstring_view s) -> bool
{
	if (s.empty())
		return false;

	auto it = begin(s);
	if (s[0] == '-')
		++it;
	while (it != end(s)) {
		auto next = std::find_if(
		    it, end(s), [](auto c) { return c < '0' || c > '9'; });
		if (next == it)
			return false;
		if (next == end(s))
			return true;
		it = next;
		auto c = *it;
		if (c == '.' || c == ',' || c == '-')
			++it;
		else
			return false;
	}
	return false;
}

auto count_appereances_of(wstring_view haystack, wstring_view needles) -> size_t
{
	return std::count_if(begin(haystack), end(haystack), [&](auto c) {
		return needles.find(c) != needles.npos;
	});
}
} // namespace v4
} // namespace nuspell
