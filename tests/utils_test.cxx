/* Copyright 2017-2020 Sander van Geloven, Dimitrij Mijoski
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

#include <nuspell/utils.hxx>

#include <boost/locale.hpp>
#include <catch2/catch.hpp>

using namespace std;
using namespace nuspell;

TEST_CASE("is_all_ascii", "[locale_utils]")
{
	CHECK(is_all_ascii(""));
	CHECK(is_all_ascii("the brown fox~"));
	CHECK_FALSE(is_all_ascii("brown foxĳӤ"));
}

TEST_CASE("latin1_to_ucs2", "[locale_utils]")
{
	CHECK(u"" == latin1_to_ucs2(""));
	CHECK(u"abc\u0080" == latin1_to_ucs2("abc\x80"));

	CHECK(u"²¿ýþÿ" != latin1_to_ucs2("²¿ýþÿ"));
	CHECK(u"Ӥ日本に" != latin1_to_ucs2("Ӥ日本に"));
}

TEST_CASE("is_all_bmp", "[locale_utils]")
{
	CHECK(is_all_bmp(u"abcýþÿӤ"));
	CHECK_FALSE(is_all_bmp(u"abcý \U00010001 þÿӤ"));
}

class latin1_codecvt : public codecvt<wchar_t, char, mbstate_t> {
	using Base = codecvt<wchar_t, char, mbstate_t>;

      public:
	using Base::Base;

      protected:
	result do_out(state_type& /*state*/, const wchar_t* from,
	              const wchar_t* from_end, const wchar_t*& from_next,
	              char* to, char* to_end, char*& to_next) const override
	{
		for (; from != from_end && to != to_end; ++from, ++to) {
			auto c = *from;
			if (0 <= c && c < 256) {
				*to = c;
			}
			else {
				from_next = from;
				to_next = to;
				return Base::error;
			}
		}
		from_next = from;
		to_next = to;
		if (from == from_end)
			return Base::ok;
		else
			return Base::partial;
	}
	result do_unshift(state_type& /*state*/, char* to, char* /*to_end*/,
	                  char*& to_next) const override
	{
		to_next = to;
		return Base::noconv;
	}
	result do_in(state_type& /*state*/, const char* from,
	             const char* from_end, const extern_type*& from_next,
	             wchar_t* to, intern_type* to_end,
	             intern_type*& to_next) const override
	{
		for (; from != from_end && to != to_end; ++from, ++to) {
			*to = static_cast<unsigned char>(*from);
		}
		from_next = from;
		to_next = to;
		if (from == from_end)
			return Base::ok;
		else
			return Base::partial;
	}
	int do_encoding() const noexcept override { return 1; }
	bool do_always_noconv() const noexcept override { return false; }
	int do_length(state_type& /*state*/, const char* from,
	              const char* from_end, size_t max) const override
	{
		size_t len = from_end - from;
		return min(len, max);
	}
	int do_max_length() const noexcept override { return 1; }
};

/*
These codecvts are buggy, especially on MSVC.
#if U_SIZEOF_WCHAR_T == 2
using u8_to_wide_codecvt = codecvt_utf8_utf16<wchar_t>;
#elif U_SIZEOF_WCHAR_T == 4
using u8_to_wide_codecvt = codecvt_utf8<wchar_t>;
#endif
*/

TEST_CASE("to_wide", "[locale_utils]")
{
	auto gen = boost::locale::generator();
	gen.characters(boost::locale::wchar_t_facet);
	gen.categories(boost::locale::codepage_facet);
	auto loc = gen("en_US.UTF-8");
	auto in = string("\U0010FFFF ß");
	CHECK(L"\U0010FFFF ß" == to_wide(in, loc));

	in = "\U00011D59\U00011D59\U00011D59\U00011D59\U00011D59";
	auto out = wstring();
	auto exp = L"\U00011D59\U00011D59\U00011D59\U00011D59\U00011D59";
	CHECK(true == to_wide(in, loc, out));
	CHECK(exp == out);

	loc = locale(locale::classic(), new latin1_codecvt());
	in = "abcd\xDF";
	CHECK(L"abcdß" == to_wide(in, loc));
}

TEST_CASE("to_narrow", "[locale_utils]")
{
	auto gen = boost::locale::generator();
	gen.characters(boost::locale::wchar_t_facet);
	gen.categories(boost::locale::codepage_facet);
	auto loc = gen("en_US.UTF-8");
	auto in = wstring(L"\U0010FFFF ß");
	CHECK("\U0010FFFF ß" == to_narrow(in, loc));

	in = L"\U00011D59\U00011D59\U00011D59\U00011D59\U00011D59";
	auto out = string();
	CHECK(true == to_narrow(in, out, loc));
	CHECK("\U00011D59\U00011D59\U00011D59\U00011D59\U00011D59" == out);

	loc = locale(locale::classic(), new latin1_codecvt());
	in = L"abcdß";
	CHECK("abcd\xDF" == to_narrow(in, loc));

	in = L"\U00011D59\U00011D59\U00011D59\U00011D59\U00011D59";
	out = string();
	CHECK(false == to_narrow(in, out, loc));
	CHECK(all_of(begin(out), end(out), [](auto c) { return c == '?'; }));
}

TEST_CASE("wide_to_utf8", "[locale_utils]")
{
	CHECK("abгшß" == wide_to_utf8(L"abгшß"));
	CHECK("\U0010FFFF" == wide_to_utf8(L"\U0010FFFF"));
	CHECK("\U0010FFFF\U0010FF12" == wide_to_utf8(L"\U0010FFFF\U0010FF12"));
	CHECK("\U0010FFFF ß" == wide_to_utf8(L"\U0010FFFF ß"));

	auto in =
	    wstring(L"\U00011D59\U00011D59\U00011D59\U00011D59\U00011D59");
	auto out = string();
	wide_to_utf8(in, out);
	CHECK("\U00011D59\U00011D59\U00011D59\U00011D59\U00011D59" == out);

	in.assign(256, L'a');
	in += L"\U0010FFFF";
	out.clear();
	out.shrink_to_fit();
	auto exp = string(256, 'a');
	exp += "\U0010FFFF";
	wide_to_utf8(in, out);
	CHECK(exp == out);
}

TEST_CASE("classify_casing", "[locale_utils]")
{
	CHECK(Casing::SMALL == classify_casing(L""));
	CHECK(Casing::SMALL == classify_casing(L"alllowercase"));
	CHECK(Casing::SMALL == classify_casing(L"alllowercase3"));
	CHECK(Casing::INIT_CAPITAL == classify_casing(L"Initandlowercase"));
	CHECK(Casing::INIT_CAPITAL == classify_casing(L"Initandlowercase_"));
	CHECK(Casing::ALL_CAPITAL == classify_casing(L"ALLUPPERCASE"));
	CHECK(Casing::ALL_CAPITAL == classify_casing(L"ALLUPPERCASE."));
	CHECK(Casing::CAMEL == classify_casing(L"iCamelCase"));
	CHECK(Casing::CAMEL == classify_casing(L"iCamelCase@"));
	CHECK(Casing::PASCAL == classify_casing(L"InitCamelCase"));
	CHECK(Casing::PASCAL == classify_casing(L"InitCamelCase "));
	CHECK(Casing::INIT_CAPITAL == classify_casing(L"İstanbul"));
}

TEST_CASE("to_upper", "[locale_utils]")
{
	auto l = icu::Locale();

	CHECK(L"" == to_upper(L"", l));
	CHECK(L"A" == to_upper(L"a", l));
	CHECK(L"A" == to_upper(L"A", l));
	CHECK(L"AA" == to_upper(L"aa", l));
	CHECK(L"AA" == to_upper(L"aA", l));
	CHECK(L"AA" == to_upper(L"Aa", l));
	CHECK(L"AA" == to_upper(L"AA", l));

	CHECK(L"TABLE" == to_upper(L"table", l));
	CHECK(L"TABLE" == to_upper(L"Table", l));
	CHECK(L"TABLE" == to_upper(L"tABLE", l));
	CHECK(L"TABLE" == to_upper(L"TABLE", l));

	// Note that i is converted to I, not İ
	CHECK_FALSE(L"İSTANBUL" == to_upper(L"istanbul", l));

	l = icu::Locale("tr_TR");
	CHECK(L"İSTANBUL" == to_upper(L"istanbul", l));
	// Note that I remains and is not converted to İ
	CHECK_FALSE(L"İSTANBUL" == to_upper(L"Istanbul", l));
	CHECK(L"DİYARBAKIR" == to_upper(L"Diyarbakır", l));

	l = icu::Locale("de_DE");
	// Note that lower case ü is not converted to upper case Ü.
	// Note that lower case ß is converted to double SS.
	// CHECK(L"GRüSSEN" == to_upper(L"grüßen", l));
	CHECK(L"GRÜSSEN" == to_upper(L"GRÜßEN", l));
	// Note that upper case ẞ is kept in upper case.
	CHECK(L"GRÜẞEN" == to_upper(L"GRÜẞEN", l));

	l = icu::Locale("nl_NL");
	CHECK(L"ÉÉN" == to_upper(L"één", l));
	CHECK(L"ÉÉN" == to_upper(L"Één", l));
	CHECK(L"IJSSELMEER" == to_upper(L"ijsselmeer", l));
	CHECK(L"IJSSELMEER" == to_upper(L"IJsselmeer", l));
	CHECK(L"IJSSELMEER" == to_upper(L"IJSSELMEER", l));
	CHECK(L"ĲSSELMEER" == to_upper(L"ĳsselmeer", l));
	CHECK(L"ĲSSELMEER" == to_upper(L"Ĳsselmeer", l));
	CHECK(L"ĲSSELMEER" == to_upper(L"ĲSSELMEER", l));
}

TEST_CASE("to_lower", "[locale_utils]")
{
	auto l = icu::Locale("en_US");

	CHECK(L"" == to_lower(L"", l));
	CHECK(L"a" == to_lower(L"A", l));
	CHECK(L"a" == to_lower(L"a", l));
	CHECK(L"aa" == to_lower(L"aa", l));
	CHECK(L"aa" == to_lower(L"aA", l));
	CHECK(L"aa" == to_lower(L"Aa", l));
	CHECK(L"aa" == to_lower(L"AA", l));

	CHECK(L"table" == to_lower(L"table", l));
	CHECK(L"table" == to_lower(L"Table", l));
	CHECK(L"table" == to_lower(L"TABLE", l));

	// Note that İ is converted to i followed by COMBINING DOT ABOVE U+0307
	CHECK_FALSE(L"istanbul" == to_lower(L"İSTANBUL", l));
	// Note that İ is converted to i followed by COMBINING DOT ABOVE U+0307
	CHECK_FALSE(L"istanbul" == to_lower(L"İstanbul", l));

	l = icu::Locale("tr_TR");
	CHECK(L"istanbul" == to_lower(L"İSTANBUL", l));
	CHECK(L"istanbul" == to_lower(L"İstanbul", l));
	CHECK(L"diyarbakır" == to_lower(L"Diyarbakır", l));

	l = icu::Locale("el_GR");
	CHECK(L"ελλάδα" == to_lower(L"ελλάδα", l));
	CHECK(L"ελλάδα" == to_lower(L"Ελλάδα", l));
	CHECK(L"ελλάδα" == to_lower(L"ΕΛΛΆΔΑ", l));

	l = icu::Locale("de_DE");
	CHECK(L"grüßen" == to_lower(L"grüßen", l));
	CHECK(L"grüssen" == to_lower(L"grüssen", l));
	// Note that double SS is not converted to lower case ß.
	CHECK(L"grüssen" == to_lower(L"GRÜSSEN", l));
	// Note that upper case ẞ is converted to lower case ß.
	// this assert fails on windows with icu 62
	// CHECK(L"grüßen" == to_lower(L"GRÜẞEN", l));

	l = icu::Locale("nl_NL");
	CHECK(L"één" == to_lower(L"Één", l));
	CHECK(L"één" == to_lower(L"ÉÉN", l));
	CHECK(L"ijsselmeer" == to_lower(L"ijsselmeer", l));
	CHECK(L"ijsselmeer" == to_lower(L"IJsselmeer", l));
	CHECK(L"ijsselmeer" == to_lower(L"IJSSELMEER", l));
	CHECK(L"ĳsselmeer" == to_lower(L"Ĳsselmeer", l));
	CHECK(L"ĳsselmeer" == to_lower(L"ĲSSELMEER", l));
	CHECK(L"ĳsselmeer" == to_lower(L"Ĳsselmeer", l));
}

TEST_CASE("to_title", "[locale_utils]")
{
	auto l = icu::Locale("en_US");
	CHECK(L"" == to_title(L"", l));
	CHECK(L"A" == to_title(L"a", l));
	CHECK(L"A" == to_title(L"A", l));
	CHECK(L"Aa" == to_title(L"aa", l));
	CHECK(L"Aa" == to_title(L"Aa", l));
	CHECK(L"Aa" == to_title(L"aA", l));
	CHECK(L"Aa" == to_title(L"AA", l));

	CHECK(L"Table" == to_title(L"table", l));
	CHECK(L"Table" == to_title(L"Table", l));
	CHECK(L"Table" == to_title(L"tABLE", l));
	CHECK(L"Table" == to_title(L"TABLE", l));

	// Note that i is converted to I, not İ
	CHECK_FALSE(L"İstanbul" == to_title(L"istanbul", l));
	// Note that i is converted to I, not İ
	CHECK_FALSE(L"İstanbul" == to_title(L"iSTANBUL", l));
	CHECK(L"İstanbul" == to_title(L"İSTANBUL", l));
	CHECK(L"Istanbul" == to_title(L"ISTANBUL", l));

	CHECK(to_title(L"ß", l) == L"Ss");

	l = icu::Locale("tr_TR");
	CHECK(L"İstanbul" == to_title(L"istanbul", l));
	CHECK(L"İstanbul" == to_title(L"iSTANBUL", l));
	CHECK(L"İstanbul" == to_title(L"İSTANBUL", l));
	CHECK(L"Istanbul" == to_title(L"ISTANBUL", l));
	CHECK(L"Diyarbakır" == to_title(L"diyarbakır", l));
	l = icu::Locale("tr_CY");
	CHECK(L"İstanbul" == to_title(L"istanbul", l));
	l = icu::Locale("crh_UA");
	// Note that lower case i is not converted to upper case İ, bug?
	CHECK(L"Istanbul" == to_title(L"istanbul", l));
	l = icu::Locale("az_AZ");
	CHECK(L"İstanbul" == to_title(L"istanbul", l));
	l = icu::Locale("az_IR");
	CHECK(L"İstanbul" == to_title(L"istanbul", l));

	l = icu::Locale("el_GR");
	CHECK(L"Ελλάδα" == to_title(L"ελλάδα", l));
	CHECK(L"Ελλάδα" == to_title(L"Ελλάδα", l));
	CHECK(L"Ελλάδα" == to_title(L"ΕΛΛΆΔΑ", l));
	CHECK(L"Σίγμα" == to_title(L"Σίγμα", l));
	CHECK(L"Σίγμα" == to_title(L"σίγμα", l));
	// Use of ς where σ is expected, should convert to upper case Σ.
	CHECK(L"Σίγμα" == to_title(L"ςίγμα", l));

	l = icu::Locale("de_DE");
	CHECK(L"Grüßen" == to_title(L"grüßen", l));
	CHECK(L"Grüßen" == to_title(L"GRÜßEN", l));
	// Use of upper case ẞ where lower case ß is expected.
	// this assert fails on windows with icu 62
	// CHECK(L"Grüßen" == to_title(L"GRÜẞEN", l));

	l = icu::Locale("nl_NL");
	CHECK(L"Één" == to_title(L"één", l));
	CHECK(L"Één" == to_title(L"ÉÉN", l));
	CHECK(L"IJsselmeer" == to_title(L"ijsselmeer", l));
	CHECK(L"IJsselmeer" == to_title(L"Ijsselmeer", l));
	CHECK(L"IJsselmeer" == to_title(L"iJsselmeer", l));
	CHECK(L"IJsselmeer" == to_title(L"IJsselmeer", l));
	CHECK(L"IJsselmeer" == to_title(L"IJSSELMEER", l));
	CHECK(L"Ĳsselmeer" == to_title(L"ĳsselmeer", l));
	CHECK(L"Ĳsselmeer" == to_title(L"Ĳsselmeer", l));
	CHECK(L"Ĳsselmeer" == to_title(L"ĲSSELMEER", l));
}

TEST_CASE("split_on_any_of", "[string_utils]")
{
	auto in = string("^abc;.qwe/zxc/");
	auto exp = vector<string>{"", "abc", "", "qwe", "zxc", ""};
	auto out = vector<string>();
	split_on_any_of(in, ".;^/", out);
	CHECK(exp == out);
}

TEST_CASE("is_number", "[string_utils]")
{
	CHECK_FALSE(is_number(L""));
	CHECK_FALSE(is_number(L"a"));
	CHECK_FALSE(is_number(L"1a"));
	CHECK_FALSE(is_number(L"a1"));
	CHECK_FALSE(is_number(L".a"));
	CHECK_FALSE(is_number(L"a."));
	CHECK_FALSE(is_number(L",a"));
	CHECK_FALSE(is_number(L"a,"));
	CHECK_FALSE(is_number(L"-a"));
	CHECK_FALSE(is_number(L"a-"));

	CHECK_FALSE(is_number(L"1..1"));
	CHECK_FALSE(is_number(L"1.,1"));
	CHECK_FALSE(is_number(L"1.-1"));
	CHECK_FALSE(is_number(L"1,.1"));
	CHECK_FALSE(is_number(L"1,,1"));
	CHECK_FALSE(is_number(L"1,-1"));
	CHECK_FALSE(is_number(L"1-.1"));
	CHECK_FALSE(is_number(L"1-,1"));
	CHECK_FALSE(is_number(L"1--1"));

	CHECK(is_number(L"1,1111"));
	CHECK(is_number(L"-1,1111"));
	CHECK(is_number(L"1,1111.00"));
	CHECK(is_number(L"-1,1111.00"));
	CHECK(is_number(L"1.1111"));
	CHECK(is_number(L"-1.1111"));
	CHECK(is_number(L"1.1111,00"));
	CHECK(is_number(L"-1.1111,00"));

	// below needs extra review

	CHECK(is_number(L"1"));
	CHECK(is_number(L"-1"));
	CHECK_FALSE(is_number(L"1-"));

	CHECK_FALSE(is_number(L"1."));
	CHECK_FALSE(is_number(L"-1."));
	CHECK_FALSE(is_number(L"1.-"));

	CHECK_FALSE(is_number(L"1,"));
	CHECK_FALSE(is_number(L"-1,"));
	CHECK_FALSE(is_number(L"1,-"));

	CHECK(is_number(L"1.1"));
	CHECK(is_number(L"-1.1"));
	CHECK_FALSE(is_number(L"1.1-"));

	CHECK(is_number(L"1,1"));
	CHECK(is_number(L"-1,1"));
	CHECK_FALSE(is_number(L"1,1-"));

	CHECK_FALSE(is_number(L".1"));
	CHECK_FALSE(is_number(L"-.1"));
	CHECK_FALSE(is_number(L".1-"));

	CHECK_FALSE(is_number(L",1"));
	CHECK_FALSE(is_number(L"-,1"));
	CHECK_FALSE(is_number(L",1-"));
}
