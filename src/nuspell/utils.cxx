/* Copyright 2016-2019 Dimitrij Mijoski
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

#ifdef __GNUC__
#define likely(expr) __builtin_expect(!!(expr), 1)
#define unlikely(expr) __builtin_expect(!!(expr), 0)
#else
#define likely(expr) (expr)
#define unlikely(expr) (expr)
#endif

enum class Utf_Error_Handling { ALWAYS_VALID, REPLACE, SKIP };

template <Utf_Error_Handling eh, class InChar, class OutContainer>
auto utf_to_utf(const std::basic_string<InChar>& in, OutContainer& out) -> bool
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
auto valid_utf_to_utf(const std::basic_string<InChar>& in, OutContainer& out)
    -> void
{
	utf_to_utf<Utf_Error_Handling::ALWAYS_VALID>(in, out);
}

template <class InChar, class OutContainer>
auto utf_to_utf_my(const std::basic_string<InChar>& in, OutContainer& out)
    -> bool
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

auto is_ascii(char c) -> bool { return static_cast<unsigned char>(c) <= 127; }

auto is_all_ascii(const std::string& s) -> bool
{
	return all_of(begin(s), end(s), is_ascii);
}

template <class CharT>
auto widen_latin1(char c) -> CharT
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
	transform(begin(s), end(s), begin(out), widen_latin1<char16_t>);
}

auto is_surrogate_pair(char16_t c) -> bool
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
	auto in_ptr = &in[0];
	auto in_last = &in[in.size()];
	auto out_ptr = &out[0];
	auto out_last = &out[out.size()];
	auto valid = true;
	for (;;) {
		auto err = cvt.in(state, in_ptr, in_last, in_ptr, out_ptr,
		                  out_last, out_ptr);
		if (err == cvt.ok || err == cvt.noconv) {
			break;
		}
		else if (err == cvt.partial && out_ptr == out_last) {
			// no space in output buf
			auto idx = out_ptr - &out[0];
			out.resize(out.size() * 2);
			out_ptr = &out[idx];
			out_last = &out[out.size()];
		}
		else if (err == cvt.partial && out_ptr != out_last) {
			// incomplete sequence at the end
			*out_ptr++ = L'\uFFFD';
			valid = false;
			break;
		}
		else if (err == cvt.error) {
			if (out_ptr == out_last) {
				auto idx = out_ptr - &out[0];
				out.resize(out.size() * 2);
				out_ptr = &out[idx];
				out_last = &out[out.size()];
			}
			in_ptr++;
			*out_ptr++ = L'\uFFFD';
			valid = false;
		}
	}
	out.erase(out_ptr - &out[0]);
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
	auto in_ptr = &in[0];
	auto in_last = &in[in.size()];
	auto out_ptr = &out[0];
	auto out_last = &out[out.size()];
	auto valid = true;
	for (size_t i = 2;;) {
		auto err = cvt.out(state, in_ptr, in_last, in_ptr, out_ptr,
		                   out_last, out_ptr);
		if (err == cvt.ok || err == cvt.noconv) {
			break;
		}
		else if (err == cvt.partial && i != 0) {
			// probably no space in output buf
			auto idx = out_ptr - &out[0];
			out.resize(out.size() * 2);
			out_ptr = &out[idx];
			out_last = &out[out.size()];
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
				auto idx = out_ptr - &out[0];
				out.resize(out.size() * 2);
				out_ptr = &out[idx];
				out_last = &out[out.size()];
				--i;
			}
			in_ptr++;
			*out_ptr++ = '?';
			valid = false;
		}
	}
	out.erase(out_ptr - &out[0]);
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
	char_type.toupper(&s[0], &s[s.size()]);
}

auto is_locale_known_utf8(const locale& loc) -> bool
{
	return has_facet<boost::locale::utf8_codecvt<wchar_t>>(loc);
}

auto wide_to_icu(wstring_view in, icu::UnicodeString& out) -> bool
{
	int32_t capacity = in.size();
	if (in.size() > numeric_limits<int32_t>::max()) {
		out.remove();
		return false;
	}
#if U_SIZEOF_WCHAR_T == 4
	if (capacity < out.getCapacity())
		capacity = out.getCapacity();
#endif
	auto buf = out.getBuffer(capacity);
	int32_t len;
	auto err = U_ZERO_ERROR;
	u_strFromWCS(buf, capacity, &len, in.data(), in.size(), &err);
	if (U_SUCCESS(err)) {
		out.releaseBuffer(len);
		return true;
	}
#if U_SIZEOF_WCHAR_T == 4
	if (err == U_BUFFER_OVERFLOW_ERROR) {
		out.releaseBuffer(0);
		// handle buffer overflow
		buf = out.getBuffer(len);
		err = U_ZERO_ERROR;
		u_strFromWCS(buf, len, nullptr, in.data(), in.size(), &err);
		if (U_SUCCESS(err)) {
			out.releaseBuffer(len);
			return true;
		}
	}
#endif
	out.releaseBuffer(0);
	return false;
}
auto icu_to_wide(const icu::UnicodeString& in, std::wstring& out) -> bool
{
	int32_t len;
	auto err = U_ZERO_ERROR;
	out.resize(in.length());
	u_strToWCS(&out[0], out.size(), &len, in.getBuffer(), in.length(),
	           &err);
	if (U_SUCCESS(err)) {
		out.erase(len);
		return true;
	}
	out.clear();
	return false;
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
	auto us = icu::UnicodeString();
	wide_to_icu(in, us);
	us.toUpper(loc);
	icu_to_wide(us, out);
}
auto to_title(wstring_view in, const icu::Locale& loc, wstring& out) -> void
{
	auto us = icu::UnicodeString();
	wide_to_icu(in, us);
	us.toTitle(nullptr, loc);
	icu_to_wide(us, out);
}
auto to_lower(wstring_view in, const icu::Locale& loc, wstring& out) -> void
{
	auto us = icu::UnicodeString();
	wide_to_icu(in, us);
	us.toLower(loc);
	icu_to_wide(us, out);
}

/**
 * @brief Determines casing (capitalization) type for a word.
 *
 * Casing is sometimes referred to as capitalization.
 *
 * @param s word for which casing is determined.
 * @return The casing type.
 */
auto classify_casing(const std::wstring& s) -> Casing
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
	auto us = icu::UnicodeString(in.c_str(), in.size(), cnv, err);
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

auto count_appereances_of(const wstring& haystack, const wstring& needles)
    -> size_t
{
	return std::count_if(begin(haystack), end(haystack), [&](auto c) {
		return needles.find(c) != needles.npos;
	});
}

} // namespace nuspell
