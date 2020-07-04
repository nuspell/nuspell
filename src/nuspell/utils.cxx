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

#include <boost/locale/utf8_codecvt.hpp>

#include <unicode/uchar.h>
#include <unicode/ucnv.h>
#include <unicode/unistr.h>
#include <unicode/ustring.h>

#if ' ' != 32 || '.' != 46 || 'A' != 65 || 'Z' != 90 || 'a' != 97 || 'z' != 122
#error "Basic execution character set is not ASCII"
#elif L' ' != 32 || L'.' != 46 || L'A' != 65 || L'Z' != 90 || L'a' != 97 ||    \
    L'z' != 122
#error "Basic wide execution character set is not ASCII-compatible"
#endif

namespace nuspell {
using namespace std;

template <class SepT>
static auto& split_on_any_of_low(const std::string& s, const SepT& sep,
                                 std::vector<std::string>& out)
{
	size_t i1 = 0;
	size_t i2;
	do {
		i2 = s.find_first_of(sep, i1);
		out.push_back(s.substr(i1, i2 - i1));
		i1 = i2 + 1; // we can only add +1 if sep is single char.

		// i2 gets s.npos after the last separator.
		// Length of i2-i1 will always go past the end. That is defined.
	} while (i2 != s.npos);
	return out;
}

/**
 * @brief Splits string on single char seperator.
 *
 * Consecutive separators are treated as separate and will emit empty strings.
 *
 * @param s string to split.
 * @param sep char that acts as separator to split on.
 * @param out vector where separated strings are appended.
 * @return @p out.
 */
auto split(const std::string& s, char sep, std::vector<std::string>& out)
    -> std::vector<std::string>&
{
	return split_on_any_of_low(s, sep, out);
}

/**
 * @brief Splits string on set of single char seperators.
 *
 * Consecutive separators are treated as separate and will emit empty strings.
 *
 * @param s string to split.
 * @param sep seperator(s) to split on.
 * @param out vector where separated strings are appended.
 * @return @p out.
 */
auto split_on_any_of(const std::string& s, const char* sep,
                     std::vector<std::string>& out) -> std::vector<std::string>&
{
	return split_on_any_of_low(s, sep, out);
}

enum class Utf_Error_Handling { ALWAYS_VALID, REPLACE, SKIP };

template <Utf_Error_Handling eh, class InChar, class OutContainer>
auto static utf_to_utf(const std::basic_string<InChar>& in, OutContainer& out)
    -> bool
{
	using OutChar = typename OutContainer::value_type;
	using namespace boost::locale::utf;
	using UEH = Utf_Error_Handling;
	auto constexpr max_out_width = utf_traits<OutChar>::max_width;

	if (in.size() <= out.capacity() / max_out_width)
		out.resize(in.size() * max_out_width);
	else if (in.size() <= out.capacity())
		out.resize(out.capacity());
	else
		out.resize(in.size());

	auto it = begin(in);
	auto last = end(in);
	auto out_it = begin(out);
	auto out_last = end(out);
	auto valid = true;
	while (it != last) {
		auto cp = code_point();
		if (eh == UEH::ALWAYS_VALID) {
			cp = utf_traits<InChar>::decode_valid(it);
		}
		else {
			cp = utf_traits<InChar>::decode(it, last);
			if (unlikely(cp == incomplete || cp == illegal)) {
				valid = false;
				if (eh == UEH::SKIP)
					continue;
				else if (eh == UEH::REPLACE)
					cp = 0xFFFD;
			}
		}
		auto remaining_space = out_last - out_it;
		auto width_cp = utf_traits<OutChar>::width(cp);
		if (unlikely(remaining_space < width_cp)) {
			// resize
			auto i = out_it - begin(out);
			auto min_resize = width_cp - remaining_space;
			out.resize(out.size() + min_resize + (last - it));
			out_it = begin(out) + i;
			out_last = end(out);
		}
		out_it = utf_traits<OutChar>::encode(cp, out_it);
	}
	out.erase(out_it, end(out));
	return valid;
}

template <class InChar, class OutContainer>
auto static valid_utf_to_utf(const std::basic_string<InChar>& in,
                             OutContainer& out) -> void
{
	utf_to_utf<Utf_Error_Handling::ALWAYS_VALID>(in, out);
}

template <class InChar, class OutContainer>
auto static utf_to_utf_my(const std::basic_string<InChar>& in,
                          OutContainer& out) -> bool
{
	return utf_to_utf<Utf_Error_Handling::REPLACE>(in, out);
}

auto wide_to_utf8(const std::wstring& in, std::string& out) -> void
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
	utf_to_utf_my(in, out);
#endif
}
auto wide_to_utf8(const std::wstring& in) -> std::string
{
	auto out = string();
	wide_to_utf8(in, out);
	return out;
}

auto utf8_to_wide(const std::string& in, std::wstring& out) -> bool
{
	return utf_to_utf_my(in, out);
}
auto utf8_to_wide(const std::string& in) -> std::wstring
{
	auto out = wstring();
	utf_to_utf_my(in, out);
	return out;
}

auto utf8_to_16(const std::string& in) -> std::u16string
{
	auto out = u16string();
	utf_to_utf_my(in, out);
	return out;
}

bool utf8_to_16(const std::string& in, std::u16string& out)
{
	return utf_to_utf_my(in, out);
}

auto static is_ascii(char c) -> bool
{
	return static_cast<unsigned char>(c) <= 127;
}

auto is_all_ascii(const std::string& s) -> bool
{
	return all_of(begin(s), end(s), is_ascii);
}

auto static widen_latin1(char c) -> char16_t
{
	return static_cast<unsigned char>(c);
}

auto latin1_to_ucs2(const std::string& s) -> std::u16string
{
	u16string ret;
	latin1_to_ucs2(s, ret);
	return ret;
}
auto latin1_to_ucs2(const std::string& s, std::u16string& out) -> void
{
	out.resize(s.size());
	transform(begin(s), end(s), begin(out), widen_latin1);
}

auto static is_surrogate_pair(char16_t c) -> bool
{
	return 0xD800 <= c && c <= 0xDFFF;
}
auto is_all_bmp(const std::u16string& s) -> bool
{
	return none_of(begin(s), end(s), is_surrogate_pair);
}

auto to_wide(const std::string& in, const std::locale& loc, std::wstring& out)
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
		if (err == cvt.ok || err == cvt.noconv) {
			break;
		}
		else if (err == cvt.partial && out_ptr == out_last) {
			// no space in output buf
			auto idx = out_ptr - begin_ptr(out);
			out.resize(out.size() * 2);
			out_ptr = &out[idx];
			out_last = end_ptr(out);
		}
		else if (err == cvt.partial && out_ptr != out_last) {
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

auto to_wide(const std::string& in, const std::locale& loc) -> std::wstring
{
	auto ret = wstring();
	to_wide(in, loc, ret);
	return ret;
}

auto to_narrow(const std::wstring& in, std::string& out, const std::locale& loc)
    -> bool
{
	auto& cvt = use_facet<codecvt<wchar_t, char, mbstate_t>>(loc);
	out.resize(in.size(), '\0');
	auto state = mbstate_t();
	auto in_ptr = begin_ptr(in);
	auto in_last = end_ptr(in);
	auto out_ptr = begin_ptr(out);
	auto out_last = end_ptr(out);
	auto valid = true;
	for (size_t i = 2;;) {
		auto err = cvt.out(state, in_ptr, in_last, in_ptr, out_ptr,
		                   out_last, out_ptr);
		if (err == cvt.ok || err == cvt.noconv) {
			break;
		}
		else if (err == cvt.partial && i != 0) {
			// probably no space in output buf
			auto idx = out_ptr - begin_ptr(out);
			out.resize(out.size() * 2);
			out_ptr = &out[idx];
			out_last = end_ptr(out);
			--i;
		}
		else if (err == cvt.partial && i == 0) {
			// size is big enough after 2 resizes,
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
				--i;
			}
			in_ptr++;
			*out_ptr++ = '?';
			valid = false;
		}
	}
	out.erase(out_ptr - begin_ptr(out));
	return valid;
}

auto to_narrow(const std::wstring& in, const std::locale& loc) -> std::string
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

auto is_locale_known_utf8(const locale& loc) -> bool
{
	return has_facet<boost::locale::utf8_codecvt<wchar_t>>(loc);
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
 * @brief Determines casing (capitalization) type for a word.
 *
 * Casing is sometimes referred to as capitalization.
 *
 * @param s word for which casing is determined.
 * @return The casing type.
 */
auto classify_casing(wstring_view s) -> Casing
{
	// TODO implement Default Case Detection from unicode standard
	// https://www.unicode.org/versions/Unicode11.0.0/ch03.pdf
	// See Chapter 13.3. This might be feature for Boost or ICU.

	using namespace std;
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
 * @brief Check if word[i] or word[i-1] are uppercase
 *
 * Check if the two chars are alphabetic and at least one of them is in
 * uppercase.
 *
 * @param word
 * @param i
 * @param loc
 * @return true if at least one is uppercase, false otherwise.
 */
auto has_uppercase_at_compound_word_boundary(const std::wstring& word, size_t i)
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

auto Encoding_Converter::to_wide(const string& in, wstring& out) -> bool
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

auto Encoding_Converter::to_wide(const string& in) -> wstring
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
 * @brief Tests if word is a number.
 *
 * Allow numbers with dots ".", dashes "-" and commas ",", but forbids double
 * separators such as "..", "--" and ".,".  This implementation increases
 * performance over the regex implementation in the standard library.
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
} // namespace nuspell
