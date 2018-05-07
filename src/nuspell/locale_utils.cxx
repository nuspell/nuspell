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
} // namespace

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
			if (unlikely(cp > 0x10f)) {
				// cp larger that 0x10FFFF
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

auto to_wide(const std::string& in, const std::locale& loc) -> std::wstring
{
	using namespace std;
	auto& cvt = use_facet<codecvt<wchar_t, char, mbstate_t>>(loc);
	auto out = std::wstring(in.size(), L'\0');
	auto state = mbstate_t{};
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
	auto out = std::string(in.size(), '\0');
	auto state = mbstate_t{};
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
	auto ret = ctype_base::mask{};
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

	// don't uncoment this
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
		{};
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
	auto err = UErrorCode{};
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
				err = UErrorCode{};
			}
			out = u_tolower(cp);
			out.extract(&lower[i], 1, cvt, err);
			if (U_FAILURE(err)) {
				lower[i] = i;
				err = UErrorCode{};
			}
		}
		else {
			m[i] = ctype_base::mask{};
			upper[i] = i;
			lower[i] = i;
			err = UErrorCode{};
		}
	}
	ucnv_close(cvt);
}

class icu_ctype_char final : public std::ctype<char> {
      private:
	mask tbl[256];
	char upper[256];
	char lower[256];

      public:
	icu_ctype_char(const std::string& enc, std::size_t refs = 0)
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
	auto err = UErrorCode{};
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

class icu_ctype_wide final : public std::ctype<wchar_t> {
      private:
	char_type wd[256];

      public:
	icu_ctype_wide(const std::string& enc, std::size_t refs = 0)
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
		std::transform(first, last, vec,
		               [&](auto c) { return get_char_mask(c); });
		return last;
	}
	virtual const char_type* do_scan_is(mask m, const char_type* first,
	                                    const char_type* last) const
	{
		return std::find_if(first, last,
		                    [&](auto c) { return do_is(m, c); });
	}
	virtual const char_type* do_scan_not(mask m, const char_type* first,
	                                     const char_type* last) const
	{
		return std::find_if_not(first, last,
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
		std::transform(low, high, dest,
		               [&](auto c) { return do_widen(c); });
		return high;
	}
	virtual char do_narrow(char_type c, char dfault) const
	{
		auto n = std::char_traits<char_type>::find(wd, 256, c);
		if (n)
			return n - wd;
		return dfault;
	}
	virtual const char_type* do_narrow(const char_type* low,
	                                   const char_type* high, char dfault,
	                                   char* dest) const
	{
		std::transform(low, high, dest,
		               [&](auto c) { return do_narrow(c, dfault); });
		return high;
	}
};

auto install_ctype_facets_inplace(std::locale& boost_loc) -> void
{

	auto& info = use_facet<boost::locale::info>(boost_loc);
	auto enc = info.encoding();
	boost_loc = locale(boost_loc, new icu_ctype_char(enc));
	boost_loc = locale(boost_loc, new icu_ctype_wide(enc));
}
} // namespace encoding
} // namespace nuspell
