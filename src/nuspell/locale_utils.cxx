/* Copyright 2016-2017 Dimitrij Mijoski
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

#include "locale_utils.hxx"

#include <algorithm>
#include <limits>

#ifdef _MSC_VER
#include <intrin.h>
#endif

#if !defined(_WIN32)
#if !defined(__STDC_ISO_10646__) || defined(__STDC_MB_MIGHT_NEQ_WC__)
#error "Platform has poor Unicode support. wchar_t must be Unicode."
#endif
#endif

namespace nuspell {
inline namespace encoding {
using namespace std;

#ifdef __GNUC__
#define likely(expr) __builtin_expect(!!(expr), 1)
#define unlikely(expr) __builtin_expect(!!(expr), 0)
#else
#define likely(expr) (expr)
#define unlikely(expr) (expr)
#endif

namespace {

auto inline count_leading_ones(unsigned char c)
{
#ifdef __GNUC__
	unsigned cc = c;
	unsigned cc_shifted = cc << (numeric_limits<unsigned>::digits - 8);
	return __builtin_clz(~cc_shifted); // gcc only.
#elif _MSC_VER
	using ulong = unsigned long;
	ulong cc = c;
	ulong cc_shifted = cc << (numeric_limits<ulong>::digits - 8);
	ulong clz;
	BitScanReverse(&clz, ~cc_shifted);
	clz = numeric_limits<ulong>::digits - 1 - clz;
	return clz;
#else
	unsigned cc = c;
	int clz;
	// note the operator presedence
	// all parenthesis are necessary
	if ((cc & 0x80) == 0)
		clz = 0;
	else if ((cc & 0x40) == 0)
		clz = 1;
	else if ((cc & 0x20) == 0)
		clz = 2;
	else if ((cc & 0x10) == 0)
		clz = 3;
	else if ((cc & 0x08) == 0)
		clz = 4;
	else
		clz = 5;
	return clz;
#endif
}

auto is_not_continuation_byte(unsigned char c)
{
	return (c & 0b11000000) != 0b10000000;
}
auto update_cp_with_continuation_byte(char32_t& cp, unsigned char c)
{
	cp = (cp << 6) | (c & 0b00111111);
}
}

template <class InpIter, class OutIter>
auto decode_utf8(InpIter first, InpIter last, OutIter out) -> OutIter
{
	constexpr auto REP_CH = U'\uFFFD';
	char32_t cp;
	bool min_rep_err;
	bool surrogate_err;
	for (auto i = first; i != last; ++i) {
		unsigned char c = *i;
		switch (count_leading_ones(c)) {
		case 0:
			*out++ = c;
			break;
		case 1:
			*out++ = REP_CH;
			break;
		case 2:
			min_rep_err = (c & 0b00011110) == 0;
			if (unlikely(min_rep_err)) {
				*out++ = REP_CH;
				break;
			}
			cp = c & 0b00011111;

			// processing second byte
			if (unlikely(++i == last)) {
				*out++ = REP_CH;
				goto out_of_u8_loop;
			}
			c = *i;
			if (unlikely(is_not_continuation_byte(c))) {
				// sequence too short error
				--i;
				*out++ = REP_CH;
				break;
			}
			update_cp_with_continuation_byte(cp, c);
			*out++ = cp;
			break;
		case 3:
			cp = c & 0b00001111;

			// processing second byte
			if (unlikely(++i == last)) {
				*out++ = REP_CH;
				goto out_of_u8_loop;
			}
			c = *i;
			if (unlikely(is_not_continuation_byte(c))) {
				// sequence too short error
				--i;
				*out++ = REP_CH;
				break;
			}
			min_rep_err = cp == 0 && (c & 0b00100000) == 0;
			surrogate_err =
			    cp == 0b00001101 && (c & 0b00100000) != 0;
			if (unlikely(min_rep_err || surrogate_err)) {
				*out++ = REP_CH;
				break;
			}
			update_cp_with_continuation_byte(cp, c);

			// proccesing third byte
			if (unlikely(++i == last)) {
				*out++ = REP_CH;
				goto out_of_u8_loop;
			}
			c = *i;
			if (unlikely(is_not_continuation_byte(c))) {
				// sequence too short error
				--i;
				*out++ = REP_CH;
				break;
			}
			update_cp_with_continuation_byte(cp, c);

			*out++ = cp;
			break;
		case 4:
			cp = c & 0b00000111;

			// processing second byte
			if (unlikely(++i == last)) {
				*out++ = REP_CH;
				goto out_of_u8_loop;
			}
			c = *i;
			if (unlikely(is_not_continuation_byte(c))) {
				// sequence too short error
				--i;
				*out++ = REP_CH;
				break;
			}
			min_rep_err = cp == 0 && (c & 0b00110000) == 0;
			if (min_rep_err) {
				*out++ = REP_CH;
				break;
			}
			update_cp_with_continuation_byte(cp, c);

			// proccesing third byte
			if (unlikely(++i == last)) {
				*out++ = REP_CH;
				goto out_of_u8_loop;
			}
			c = *i;
			if (unlikely(is_not_continuation_byte(c))) {
				// sequence too short error
				--i;
				*out++ = REP_CH;
				break;
			}
			update_cp_with_continuation_byte(cp, c);

			// proccesing fourth byte
			if (unlikely(++i == last)) {
				*out++ = REP_CH;
				goto out_of_u8_loop;
			}
			c = *i;
			if (unlikely(is_not_continuation_byte(c))) {
				// sequence too short error
				--i;
				*out++ = REP_CH;
				break;
			}
			update_cp_with_continuation_byte(cp, c);

			*out++ = cp;
			break;
		default:
			*out++ = REP_CH;
			break;
		}
	}
out_of_u8_loop:
	return out;
}

auto decode_utf8(const std::string& s) -> std::u32string
{
	u32string ret(s.size(), 0);
	auto last = decode_utf8(begin(s), end(s), begin(ret));
	ret.erase(last, ret.end());
	return ret;
}

auto validate_utf8(const std::string& s) -> bool
{
	using namespace boost::locale::conv;
	try {
		utf_to_utf<char32_t>(s, stop);
	}
	catch (const conversion_error& e) {
		return false;
	}
	return true;
}

auto is_ascii(char c) -> bool { return (unsigned char)c <= 127; }

auto is_all_ascii(const std::string& s) -> bool
{
	return all_of(s.begin(), s.end(), is_ascii);
}

template <class CharT>
auto widen_latin1(char c) -> CharT
{
	return (unsigned char)c;
}

auto latin1_to_ucs2(const std::string& s) -> std::u16string
{
	u16string ret(s.size(), 0);
	transform(s.begin(), s.end(), ret.begin(), widen_latin1<char16_t>);
	return ret;
}

auto latin1_to_u32(const std::string& s) -> std::u32string
{
	// UNUSED FUNCTION, maybe remove
	u32string ret(s.size(), 0);
	transform(s.begin(), s.end(), ret.begin(), widen_latin1<char32_t>);
	return ret;
}

auto is_bmp(char32_t c) -> bool { return c <= 0xFFFF; }

auto is_all_bmp(const std::u32string& s) -> bool
{
	return all_of(s.begin(), s.end(), is_bmp);
}

auto u32_to_ucs2_skip_non_bmp(const std::u32string& s) -> std::u16string
{
	u16string ret(s.size(), 0);
	auto i = ret.begin();
	i = copy_if(s.begin(), s.end(), i, is_bmp);
	ret.erase(i, ret.end());
	return ret;
}

auto to_wide(const std::string& in, const std::locale& inloc) -> std::wstring
{
	using namespace std;
	auto& cvt = use_facet<codecvt<wchar_t, char, mbstate_t>>(inloc);
	auto wide = std::wstring();
	wide.resize(in.size(), L'\0');
	auto state = mbstate_t{};
	auto char_ptr = in.c_str();
	auto last = in.c_str() + in.size();
	auto wchar_ptr = &wide[0];
	auto wlast = &wide[wide.size()];
	for (;;) {
		auto err = cvt.in(state, char_ptr, last, char_ptr, wchar_ptr,
		                  wlast, wchar_ptr);
		if (err == cvt.ok || err == cvt.noconv) {
			break;
		}
		if (wchar_ptr == wlast) {
			auto idx = wchar_ptr - &wide[0];
			wide.resize(wide.size() * 2);
			wchar_ptr = &wide[idx];
			wlast = &wide[wide.size()];
		}
		if (err == cvt.partial) {
			if (char_ptr == last) {
				*wchar_ptr++ = L'\uFFFD';
				break;
			}
		}
		else if (err == cvt.error) {
			char_ptr++;
			*wchar_ptr++ = L'\uFFFD';
		}
		else {
			break; // should never happend
		}
	}
	wide.erase(wchar_ptr - &wide[0]);
	return wide;
}

auto to_singlebyte(const std::wstring& in, const std::locale& outloc)
    -> std::string
{
	using namespace std;
	auto& cvt = use_facet<ctype<wchar_t>>(outloc);
	string out(in.size(), 0);
	cvt.narrow(&in[0], &in[in.size()], '?', &out[0]);
	return out;
}
}
}
