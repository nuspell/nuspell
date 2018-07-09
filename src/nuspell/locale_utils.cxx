/* Copyright 2016-2018 Dimitrij Mijoski
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

/**
 * @file locale_utils.cxx
 * Encoding transformations.
 */

#include "locale_utils.hxx"

#include <algorithm>
#include <limits>

#include <boost/locale.hpp>

#include <unicode/uchar.h>
#include <unicode/ucnv.h>
#include <unicode/unistr.h>

#ifdef _MSC_VER
#include <intrin.h>
#endif

#if !defined(_WIN32) && !defined(__FreeBSD__)
#if !defined(__STDC_ISO_10646__) || defined(__STDC_MB_MIGHT_NEQ_WC__)
#error "Platform has poor Unicode support. wchar_t must be Unicode."
#endif
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
} // namespace

template <class InpIter, class OutIter>
auto decode_utf8(InpIter first, InpIter last, OutIter out) -> OutIter
{
	constexpr auto REP_CH = U'\uFFFD';
	char32_t cp;
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
			// min_rep_err
			if (unlikely((c & 0b00011110) == 0)) {
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
			update_cp_with_continuation_byte(cp, c);
			// min_rep_err || surrogate_err
			if (unlikely(cp <= 0x1f || (cp >> 5) == 0b11011)) {
				*out++ = REP_CH;
				break;
			}

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
			update_cp_with_continuation_byte(cp, c);

			// min_rep_err
			if (unlikely(cp <= 0x0f)) {
				*out++ = REP_CH;
				break;
			}
			// cp above 0x10ffff error
			if (unlikely(cp > 0x10f)) {
				if (cp > 0x13f) {
					// error was in first byte
					--i;
				}
				// else it was in the second
				*out++ = REP_CH;
				break;
			}

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
	ret.erase(last, end(ret));
	return ret;
}

auto validate_utf8(const std::string& s) -> bool
{
	using namespace boost::locale::utf;
	auto first = begin(s);
	auto last = end(s);
	while (first != last) {
		auto cp = utf_traits<char>::decode(first, last);
		if (unlikely(cp == incomplete || cp == illegal))
			return false;
	}
	return true;
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

auto is_bmp(char32_t c) -> bool { return c <= 0xFFFF; }

auto is_all_bmp(const std::u32string& s) -> bool
{
	return all_of(begin(s), end(s), is_bmp);
}

auto u32_to_ucs2_skip_non_bmp(const std::u32string& s) -> std::u16string
{
	u16string ret;
	u32_to_ucs2_skip_non_bmp(s, ret);
	return ret;
}
auto u32_to_ucs2_skip_non_bmp(const std::u32string& s, std::u16string& out)
    -> void
{
	out.resize(s.size());
	auto i = copy_if(begin(s), end(s), begin(out), is_bmp);
	out.erase(i, end(out));
}

auto to_wide(const std::string& in, const std::locale& loc) -> std::wstring
{
	using namespace std;
	auto& cvt = use_facet<codecvt<wchar_t, char, mbstate_t>>(loc);
	auto out = std::wstring(in.size(), L'\0');
	auto state = mbstate_t();
	auto in_ptr = in.c_str();
	auto in_last = in.c_str() + in.size();
	auto out_ptr = &out[0];
	auto out_last = &out[out.size()];
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
		}
	}
	out.erase(out_ptr - &out[0]);
	return out;
}

auto to_narrow(const std::wstring& in, const std::locale& loc) -> std::string
{
	using namespace std;
	auto& cvt = use_facet<codecvt<wchar_t, char, mbstate_t>>(loc);
	auto out = string(in.size(), '\0');
	auto state = mbstate_t();
	auto in_ptr = in.c_str();
	auto in_last = in.c_str() + in.size();
	auto out_ptr = &out[0];
	auto out_last = &out[out.size()];
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
		}
	}
	out.erase(out_ptr - &out[0]);
	return out;
}

auto get_char_mask(UChar32 cp)
{
	auto ret = ctype_base::mask();
	if (u_isspace(cp)) {
		ret |= ctype_base::space;
	}
	if (u_isprint(cp)) {
		ret |= ctype_base::print;
	}
	if (u_iscntrl(cp)) {
		ret |= ctype_base::cntrl;
	}
	if (u_isupper(cp)) {
		ret |= ctype_base::upper;
	}
	if (u_islower(cp)) {
		ret |= ctype_base::lower;
	}
	if (u_isalpha(cp)) {
		ret |= ctype_base::alpha;
	}
	if (u_isdigit(cp)) {
		ret |= ctype_base::digit;
	}
	if (u_ispunct(cp)) {
		ret |= ctype_base::punct;
	}
	if (u_isxdigit(cp)) {
		ret |= ctype_base::xdigit;
	}
	if (u_isblank(cp)) {
		ret |= ctype_base::blank;
	}

	// Do not uncomment the following, because it will cause a bug. Its
	// functionality is already covered by the code above.
	// if (u_isalnum(cp)) {
	//	ret |= ctype_base::alnum;
	//}
	// if (u_isgraph(cp)) {
	//	ret |= ctype_base::graph;
	//}
	return ret;
}

auto general_category_to_ctype_mask(UCharCategory cat) -> ctype_base::mask
{
	switch (cat) {
	case U_UNASSIGNED:
		// case U_GENERAL_OTHER_TYPES:
		// no print, graph
		return {};
	case U_UPPERCASE_LETTER:
		return ctype_base::upper | ctype_base::alpha |
		       ctype_base::print;
	case U_LOWERCASE_LETTER:
		return ctype_base::lower | ctype_base::alpha |
		       ctype_base::print;
	case U_TITLECASE_LETTER:
		return ctype_base::upper | ctype_base::alpha |
		       ctype_base::print;
	case U_MODIFIER_LETTER:
		return ctype_base::alpha | ctype_base::print;
	case U_OTHER_LETTER:
		return ctype_base::alpha | ctype_base::print;
	case U_NON_SPACING_MARK:
		return ctype_base::print;
	case U_ENCLOSING_MARK:
		return ctype_base::print;
	case U_COMBINING_SPACING_MARK:
		return ctype_base::print;
	case U_DECIMAL_DIGIT_NUMBER:
	case U_LETTER_NUMBER:
	case U_OTHER_NUMBER:
		return ctype_base::digit | ctype_base::print;
	case U_SPACE_SEPARATOR:
		return ctype_base::space | ctype_base::blank |
		       ctype_base::print; // no graph
	case U_LINE_SEPARATOR:
		return ctype_base::space | ctype_base::cntrl |
		       ctype_base::print; // no graph
	case U_PARAGRAPH_SEPARATOR:
		return ctype_base::space | ctype_base::cntrl |
		       ctype_base::print; // no graph
	case U_CONTROL_CHAR:
		return ctype_base::cntrl; // no print, graph
	case U_FORMAT_CHAR:
		return ctype_base::cntrl; // no print, graph
	case U_PRIVATE_USE_CHAR:
		// no print
		return {};
	case U_SURROGATE:
		// no print, graph
		return {};
	case U_DASH_PUNCTUATION:
	case U_START_PUNCTUATION:
	case U_END_PUNCTUATION:
	case U_CONNECTOR_PUNCTUATION:
	case U_OTHER_PUNCTUATION:
		return ctype_base::punct | ctype_base::print;

	case U_MATH_SYMBOL:
	case U_CURRENCY_SYMBOL:
	case U_MODIFIER_SYMBOL:
	case U_OTHER_SYMBOL:
		return ctype_base::print;
	case U_INITIAL_PUNCTUATION:
	case U_FINAL_PUNCTUATION:
		return ctype_base::punct | ctype_base::print;
	case U_CHAR_CATEGORY_COUNT:
		return {};
	}
	return {};
}

auto fill_ctype(const string& enc, ctype_base::mask* m, char* upper,
                char* lower)
{
	auto err = UErrorCode::U_ZERO_ERROR;
	auto cvt = ucnv_open(enc.c_str(), &err);
	auto out = icu::UnicodeString();
	for (size_t i = 0; i < 256; ++i) {
		const char ch = i;
		auto ch_ptr = &ch;
		auto cp = ucnv_getNextUChar(cvt, &ch_ptr, ch_ptr + 1, &err);
		ucnv_resetToUnicode(cvt);
		if (cp != 0xfffd && U_SUCCESS(err)) {
			m[i] = get_char_mask(cp);
			out = u_toupper(cp);
			out.extract(&upper[i], 1, cvt, err);
			if (U_FAILURE(err)) {
				upper[i] = i;
				err = UErrorCode::U_ZERO_ERROR;
			}
			out = u_tolower(cp);
			out.extract(&lower[i], 1, cvt, err);
			if (U_FAILURE(err)) {
				lower[i] = i;
				err = UErrorCode::U_ZERO_ERROR;
			}
		}
		else {
			m[i] = ctype_base::mask();
			upper[i] = i;
			lower[i] = i;
			err = UErrorCode::U_ZERO_ERROR;
		}
	}
	ucnv_close(cvt);
}

template <class CharT>
class my_ctype;

template <>
class my_ctype<char> final : public std::ctype<char> {
      private:
	mask tbl[256];
	char upper[256];
	char lower[256];

      public:
	my_ctype(const std::string& enc, std::size_t refs = 0)
	    : std::ctype<char>(tbl, false, refs)
	{
		fill_ctype(enc, tbl, upper, lower);
	}
	char_type do_toupper(char_type c) const override
	{
		return upper[static_cast<unsigned char>(c)];
	}
	const char_type* do_toupper(char_type* first,
	                            const char_type* last) const override
	{
		for (; first != last; ++first) {
			*first = do_toupper(*first);
		}
		return last;
	}
	char_type do_tolower(char_type c) const override
	{
		return lower[static_cast<unsigned char>(c)];
	}
	const char_type* do_tolower(char_type* first,
	                            const char_type* last) const override
	{
		for (; first != last; ++first) {
			*first = do_tolower(*first);
		}
		return last;
	}
};

auto fill_ctype_wide(const string& enc, wchar_t* widen)
{
	auto err = UErrorCode::U_ZERO_ERROR;
	auto cvt = ucnv_open(enc.c_str(), &err);
	auto out = icu::UnicodeString();
	for (size_t i = 0; i < 256; ++i) {
		const char ch = i;
		auto ch_ptr = &ch;
		auto cp = ucnv_getNextUChar(cvt, &ch_ptr, ch_ptr + 1, &err);
		ucnv_resetToUnicode(cvt);
		if (U_SUCCESS(err)) {
			widen[i] = cp;
		}
		else {
			widen[i] = -1;
		}
	}
	ucnv_close(cvt);
}

template <>
class my_ctype<wchar_t> final : public std::ctype<wchar_t> {
      private:
	char_type wd[256];

      public:
	my_ctype(const std::string& enc, std::size_t refs = 0)
	    : std::ctype<wchar_t>(refs)
	{
		fill_ctype_wide(enc, wd);
	}

	virtual bool do_is(mask m, char_type c) const
	{
		if ((m & space) && u_isspace(c))
			return true;
		if ((m & print) && u_isprint(c))
			return true;
		if ((m & cntrl) && u_iscntrl(c))
			return true;
		if ((m & upper) && u_isupper(c))
			return true;
		if ((m & lower) && u_islower(c))
			return true;
		if ((m & alpha) && u_isalpha(c))
			return true;
		if ((m & digit) && u_isdigit(c))
			return true;
		if ((m & punct) && u_ispunct(c))
			return true;
		if ((m & xdigit) && u_isxdigit(c))
			return true;
		if ((m & blank) && u_isblank(c))
			return true;

		// don't uncoment this
		// if ((m & alnum) && u_isalnum(c))
		//	return true;
		// if ((m & graph) && u_isgraph(c))
		//	return true;

		return false;
	}
	virtual const char_type* do_is(const char_type* first,
	                               const char_type* last, mask* vec) const
	{
		transform(first, last, vec,
		          [&](auto c) { return get_char_mask(c); });
		return last;
	}
	virtual const char_type* do_scan_is(mask m, const char_type* first,
	                                    const char_type* last) const
	{
		return find_if(first, last,
		               [&](auto c) { return do_is(m, c); });
	}
	virtual const char_type* do_scan_not(mask m, const char_type* first,
	                                     const char_type* last) const
	{
		return find_if_not(first, last,
		                   [&](auto c) { return do_is(m, c); });
	}

	virtual char_type do_toupper(char_type c) const { return u_toupper(c); }
	virtual const char_type* do_toupper(char_type* low,
	                                    const char_type* high) const
	{
		for (; low != high; ++low) {
			*low = u_toupper(*low);
		}
		return high;
	}
	virtual char_type do_tolower(char_type c) const { return u_tolower(c); }
	virtual const char_type* do_tolower(char_type* first,
	                                    const char_type* last) const
	{
		for (; first != last; ++first) {
			*first = u_tolower(*first);
		}
		return last;
	}

	virtual char_type do_widen(char c) const
	{
		return wd[static_cast<unsigned char>(c)];
	}
	virtual const char* do_widen(const char* low, const char* high,
	                             char_type* dest) const
	{
		transform(low, high, dest, [&](auto c) { return do_widen(c); });
		return high;
	}
	virtual char do_narrow(char_type c, char dfault) const
	{
		auto n = char_traits<char_type>::find(wd, 256, c);
		if (n)
			return n - wd;
		return dfault;
	}
	virtual const char_type* do_narrow(const char_type* low,
	                                   const char_type* high, char dfault,
	                                   char* dest) const
	{
		transform(low, high, dest,
		          [&](auto c) { return do_narrow(c, dfault); });
		return high;
	}
};

auto install_ctype_facets_inplace(std::locale& boost_loc) -> void
{

	auto& info = use_facet<boost::locale::info>(boost_loc);
	auto enc = info.encoding();
	boost_loc = locale(boost_loc, new my_ctype<char>(enc));
	boost_loc = locale(boost_loc, new my_ctype<wchar_t>(enc));
}

auto Locale_Input::cvt_for_byte_dict(const std::string& in,
                                     const std::locale& inloc,
                                     const std::locale& dicloc) -> std::string
{
	using namespace std;
	using info_t = boost::locale::info;
	using namespace boost::locale::conv;
	if (has_facet<boost::locale::info>(inloc)) {
		auto& in_info = use_facet<info_t>(inloc);
		auto& dic_info = use_facet<info_t>(dicloc);
		auto in_enc = in_info.encoding();
		auto dic_enc = dic_info.encoding();
		if (in_enc == dic_enc) {
			return in;
		}
	}
	return to_narrow(to_wide(in, inloc), dicloc);
}

auto Locale_Input::cvt_for_u8_dict(const std::string& in,
                                   const std::locale& inloc) -> std::wstring
{
	using namespace std;
	using info_t = boost::locale::info;
	using namespace boost::locale::conv;
	if (has_facet<boost::locale::info>(inloc)) {
		auto& in_info = use_facet<info_t>(inloc);
		if (in_info.utf8())
			return utf_to_utf<wchar_t>(in);
	}
	return to_wide(in, inloc);
}

template <class CharT>
auto classify_casing(const std::basic_string<CharT>& s, const std::locale& loc)
    -> Casing
{
	// TODO implement Default Case Detection from unicode standard
	// https://www.unicode.org/versions/Unicode10.0.0/ch03.pdf
	// See Chapter 13.3
	//
	// use boost::locale::to_lower to upper etc.

	using namespace std;
	size_t upper = 0;
	size_t lower = 0;
	auto& f = use_facet<my_ctype<CharT>>(loc);
	for (auto& c : s) {
		if (f.is(ctype_base::upper, c))
			upper++;
		else if (f.is(ctype_base::lower, c))
			lower++;
		// else neutral
	}
	if (upper == 0)               // all lowercase, maybe with some neutral
		return Casing::SMALL; // most common case

	auto first_capital = isupper(s[0], loc);
	if (first_capital && upper == 1)
		return Casing::INIT_CAPITAL; // second most common

	if (lower == 0)
		return Casing::ALL_CAPITAL;

	if (first_capital)
		return Casing::PASCAL;
	else
		return Casing::CAMEL;
}
template auto classify_casing(const std::string&, const std::locale&) -> Casing;
template auto classify_casing(const std::wstring&, const std::locale&)
    -> Casing;

} // namespace nuspell
