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

// TODO What is the source of the information below?
namespace {
// const unsigned char shift[] = {0, 6, 0, 0, 0, /**/ 0, 0, 0, 0};
const unsigned char mask[] = {0xff, 0x3f, 0x1f, 0x0f, 0x07, /**/
                              0x03, 0x01, 0x00, 0x00};
const unsigned char min_rep_mask[] = {
    0xff, 0xff, 0b00011110, 0b00001111, 0b00000111, 0, 0, 0, 0};
// for decoding
const unsigned char next_state[][9] = {{0, 4, 1, 2, 3, 4, 4, 4, 4},
                                       {0, 0, 1, 2, 3, 4, 4, 4, 4},
                                       {0, 1, 1, 2, 3, 4, 4, 4, 4},
                                       {0, 2, 1, 2, 3, 4, 4, 4, 4},
                                       {0, 4, 1, 2, 3, 4, 4, 4, 4}};

// for validating
const unsigned char next_state2[][9] = {{0, 4, 1, 2, 3, 4, 4, 4, 4},
                                        {4, 0, 4, 4, 4, 4, 4, 4, 4},
                                        {4, 1, 4, 4, 4, 4, 4, 4, 4},
                                        {4, 2, 4, 4, 4, 4, 4, 4, 4},
                                        {4, 4, 4, 4, 4, 4, 4, 4, 4}};

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
}

struct Utf8_Decoder {
	unsigned char state = 0;
	bool short_sequence_error = false;
	// bool long_sequence_error = false;
	char32_t cp = 0;

	auto next(unsigned char in) -> void;

	template <class InpIter, class OutIter>
	auto decode(InpIter first, InpIter last, OutIter out) -> OutIter;
#define minimal_representation_error(in, clz)                                  \
	(((in | 0x80) & min_rep_mask[clz]) == 0)
};

/**
 * Finite state transducer used for decoding UTF-8 stream.
 *
 * Formally, the state is a pair (state, cp) and
 * output is a triplet (is_cp_ready, out_cp, too_short_error).
 * This function mutates the state and returns that third element of the output.
 *
 * Initial state is (0, 0).
 *
 *  - is_cp_ready <=> state == 0 so we do not return it since it is
 *    directly derived from the UPDATED state (after the call).
 *  - out_cp = cp for states 0 to 3, otherwise it is FFFD.
 *    We do not return out_cp for same reason.
 *
 * We only return too_short_err.
 *
 * If too_short_err is true, we should generally emit FFFD in the output
 * stream. Then we check the state.
 * Whenever it goes to state = 0, emit out_cp = cp,   reset cp = 0.
 * Whenever it goes to state = 4, emit out_cp = FFFD, reset cp = 0.
 *
 * At the end of the input stream, we should check if the state is 1, 2 or 3
 * which indicates that too_short_err happend. FFFD should be emitted.
 *
 * @param in Input byte.
 * @return true if too short sequence error happend. False otherwise.
 */
auto inline Utf8_Decoder::next(unsigned char in) -> void
{
	char32_t cc = in;
	auto clz = count_leading_ones(in);
	// cp = (cp << shift[clz]) | (cc & mask[clz]);
	if (clz == 1)
		cp <<= 6;
	cp |= cc & mask[clz];

	//(state & 3)!=0 == state >=1 && state <= 3
	short_sequence_error = (state & 3) && clz != 1;
	state = next_state[state][clz];

	if (unlikely(minimal_representation_error(in, clz)))
		state = 4;
}

template <class InpIter, class OutIter>
auto decode_utf8(InpIter first, InpIter last, OutIter out) -> OutIter
{
	Utf8_Decoder u8;
	constexpr auto REP_CH = U'\uFFFD';
	for (auto i = first; i != last; ++i) {
		auto&& c = *i;
		u8.next(c);
		if (unlikely(u8.short_sequence_error)) {
			*out++ = REP_CH;
		}
		if (u8.state == 0) {
			*out++ = u8.cp;
			u8.cp = 0;
		}
		else if (unlikely(u8.state == 4)) {
			*out++ = REP_CH;
			u8.cp = 0;
		}
	}
	if (unlikely(u8.state & 3))
		*out++ = REP_CH;
	return out;
}

auto decode_utf8(const std::string& s) -> std::u32string
{
	u32string ret(s.size(), 0);
	auto last = decode_utf8(begin(s), end(s), begin(ret));
	ret.erase(last, ret.end());
	return ret;
}

auto inline utf8_validate_dfa(unsigned char state, char in) -> unsigned char
{
	auto clz = count_leading_ones(in);
	state = next_state2[state][clz];
	if (unlikely(minimal_representation_error(in, clz))) {
		state = 4;
	}
	return state;
}

auto validate_utf8(const std::string& s) -> bool
{
	unsigned char state = 0;
	for (auto& c : s) {
		state = utf8_validate_dfa(state, c);
		if (unlikely(state == 4))
			return false;
	}
	return state == 0;
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
