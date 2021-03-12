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

TEST_CASE("wide_to_utf8", "[locale_utils]")
{
	CHECK("abгшß" == utf32_to_utf8(U"abгшß"));
	CHECK("\U0010FFFF" == utf32_to_utf8(U"\U0010FFFF"));
	CHECK("\U0010FFFF\U0010FF12" == utf32_to_utf8(U"\U0010FFFF\U0010FF12"));
	CHECK("\U0010FFFF ß" == utf32_to_utf8(U"\U0010FFFF ß"));

	auto in =
	    u32string(U"\U00011D59\U00011D59\U00011D59\U00011D59\U00011D59");
	auto out = string();
	utf32_to_utf8(in, out);
	CHECK("\U00011D59\U00011D59\U00011D59\U00011D59\U00011D59" == out);

	in.assign(256, U'a');
	in += U'\U0010FFFF';
	out.clear();
	out.shrink_to_fit();
	auto exp = string(256, 'a');
	exp += "\U0010FFFF";
	utf32_to_utf8(in, out);
	CHECK(exp == out);
}

TEST_CASE("classify_casing", "[locale_utils]")
{
	CHECK(Casing::SMALL == classify_casing(""));
	CHECK(Casing::SMALL == classify_casing("alllowercase"));
	CHECK(Casing::SMALL == classify_casing("alllowercase3"));
	CHECK(Casing::INIT_CAPITAL == classify_casing("Initandlowercase"));
	CHECK(Casing::INIT_CAPITAL == classify_casing("Initandlowercase_"));
	CHECK(Casing::ALL_CAPITAL == classify_casing("ALLUPPERCASE"));
	CHECK(Casing::ALL_CAPITAL == classify_casing("ALLUPPERCASE."));
	CHECK(Casing::CAMEL == classify_casing("iCamelCase"));
	CHECK(Casing::CAMEL == classify_casing("iCamelCase@"));
	CHECK(Casing::PASCAL == classify_casing("InitCamelCase"));
	CHECK(Casing::PASCAL == classify_casing("InitCamelCase "));
	CHECK(Casing::INIT_CAPITAL == classify_casing("İstanbul"));
}

TEST_CASE("to_upper", "[locale_utils]")
{
	auto l = icu::Locale();

	CHECK("" == to_upper("", l));
	CHECK("A" == to_upper("a", l));
	CHECK("A" == to_upper("A", l));
	CHECK("AA" == to_upper("aa", l));
	CHECK("AA" == to_upper("aA", l));
	CHECK("AA" == to_upper("Aa", l));
	CHECK("AA" == to_upper("AA", l));

	CHECK("TABLE" == to_upper("table", l));
	CHECK("TABLE" == to_upper("Table", l));
	CHECK("TABLE" == to_upper("tABLE", l));
	CHECK("TABLE" == to_upper("TABLE", l));

	// Note that i is converted to I, not İ
	CHECK_FALSE("İSTANBUL" == to_upper("istanbul", l));

	l = icu::Locale("tr_TR");
	CHECK("İSTANBUL" == to_upper("istanbul", l));
	// Note that I remains and is not converted to İ
	CHECK_FALSE("İSTANBUL" == to_upper("Istanbul", l));
	CHECK("DİYARBAKIR" == to_upper("Diyarbakır", l));

	l = icu::Locale("de_DE");
	// Note that lower case ü is not converted to upper case Ü.
	// Note that lower case ß is converted to double SS.
	// CHECK("GRüSSEN" == to_upper("grüßen", l));
	CHECK("GRÜSSEN" == to_upper("GRÜßEN", l));
	// Note that upper case ẞ is kept in upper case.
	CHECK("GRÜẞEN" == to_upper("GRÜẞEN", l));

	l = icu::Locale("nl_NL");
	CHECK("ÉÉN" == to_upper("één", l));
	CHECK("ÉÉN" == to_upper("Één", l));
	CHECK("IJSSELMEER" == to_upper("ijsselmeer", l));
	CHECK("IJSSELMEER" == to_upper("IJsselmeer", l));
	CHECK("IJSSELMEER" == to_upper("IJSSELMEER", l));
	CHECK("ĲSSELMEER" == to_upper("ĳsselmeer", l));
	CHECK("ĲSSELMEER" == to_upper("Ĳsselmeer", l));
	CHECK("ĲSSELMEER" == to_upper("ĲSSELMEER", l));
}

TEST_CASE("to_lower", "[locale_utils]")
{
	auto l = icu::Locale("en_US");

	CHECK("" == to_lower("", l));
	CHECK("a" == to_lower("A", l));
	CHECK("a" == to_lower("a", l));
	CHECK("aa" == to_lower("aa", l));
	CHECK("aa" == to_lower("aA", l));
	CHECK("aa" == to_lower("Aa", l));
	CHECK("aa" == to_lower("AA", l));

	CHECK("table" == to_lower("table", l));
	CHECK("table" == to_lower("Table", l));
	CHECK("table" == to_lower("TABLE", l));

	// Note that İ is converted to i followed by COMBINING DOT ABOVE U+0307
	CHECK_FALSE("istanbul" == to_lower("İSTANBUL", l));
	// Note that İ is converted to i followed by COMBINING DOT ABOVE U+0307
	CHECK_FALSE("istanbul" == to_lower("İstanbul", l));

	l = icu::Locale("tr_TR");
	CHECK("istanbul" == to_lower("İSTANBUL", l));
	CHECK("istanbul" == to_lower("İstanbul", l));
	CHECK("diyarbakır" == to_lower("Diyarbakır", l));

	l = icu::Locale("el_GR");
	CHECK("ελλάδα" == to_lower("ελλάδα", l));
	CHECK("ελλάδα" == to_lower("Ελλάδα", l));
	CHECK("ελλάδα" == to_lower("ΕΛΛΆΔΑ", l));

	l = icu::Locale("de_DE");
	CHECK("grüßen" == to_lower("grüßen", l));
	CHECK("grüssen" == to_lower("grüssen", l));
	// Note that double SS is not converted to lower case ß.
	CHECK("grüssen" == to_lower("GRÜSSEN", l));
	// Note that upper case ẞ is converted to lower case ß.
	// this assert fails on windows with icu 62
	// CHECK("grüßen" == to_lower("GRÜẞEN", l));

	l = icu::Locale("nl_NL");
	CHECK("één" == to_lower("Één", l));
	CHECK("één" == to_lower("ÉÉN", l));
	CHECK("ijsselmeer" == to_lower("ijsselmeer", l));
	CHECK("ijsselmeer" == to_lower("IJsselmeer", l));
	CHECK("ijsselmeer" == to_lower("IJSSELMEER", l));
	CHECK("ĳsselmeer" == to_lower("Ĳsselmeer", l));
	CHECK("ĳsselmeer" == to_lower("ĲSSELMEER", l));
	CHECK("ĳsselmeer" == to_lower("Ĳsselmeer", l));
}

TEST_CASE("to_title", "[locale_utils]")
{
	auto l = icu::Locale("en_US");
	CHECK("" == to_title("", l));
	CHECK("A" == to_title("a", l));
	CHECK("A" == to_title("A", l));
	CHECK("Aa" == to_title("aa", l));
	CHECK("Aa" == to_title("Aa", l));
	CHECK("Aa" == to_title("aA", l));
	CHECK("Aa" == to_title("AA", l));

	CHECK("Table" == to_title("table", l));
	CHECK("Table" == to_title("Table", l));
	CHECK("Table" == to_title("tABLE", l));
	CHECK("Table" == to_title("TABLE", l));

	// Note that i is converted to I, not İ
	CHECK_FALSE("İstanbul" == to_title("istanbul", l));
	// Note that i is converted to I, not İ
	CHECK_FALSE("İstanbul" == to_title("iSTANBUL", l));
	CHECK("İstanbul" == to_title("İSTANBUL", l));
	CHECK("Istanbul" == to_title("ISTANBUL", l));

	CHECK(to_title("ß", l) == "Ss");

	l = icu::Locale("tr_TR");
	CHECK("İstanbul" == to_title("istanbul", l));
	CHECK("İstanbul" == to_title("iSTANBUL", l));
	CHECK("İstanbul" == to_title("İSTANBUL", l));
	CHECK("Istanbul" == to_title("ISTANBUL", l));
	CHECK("Diyarbakır" == to_title("diyarbakır", l));
	l = icu::Locale("tr_CY");
	CHECK("İstanbul" == to_title("istanbul", l));
	l = icu::Locale("crh_UA");
	// Note that lower case i is not converted to upper case İ, bug?
	CHECK("Istanbul" == to_title("istanbul", l));
	l = icu::Locale("az_AZ");
	CHECK("İstanbul" == to_title("istanbul", l));
	l = icu::Locale("az_IR");
	CHECK("İstanbul" == to_title("istanbul", l));

	l = icu::Locale("el_GR");
	CHECK("Ελλάδα" == to_title("ελλάδα", l));
	CHECK("Ελλάδα" == to_title("Ελλάδα", l));
	CHECK("Ελλάδα" == to_title("ΕΛΛΆΔΑ", l));
	CHECK("Σίγμα" == to_title("Σίγμα", l));
	CHECK("Σίγμα" == to_title("σίγμα", l));
	// Use of ς where σ is expected, should convert to upper case Σ.
	CHECK("Σίγμα" == to_title("ςίγμα", l));

	l = icu::Locale("de_DE");
	CHECK("Grüßen" == to_title("grüßen", l));
	CHECK("Grüßen" == to_title("GRÜßEN", l));
	// Use of upper case ẞ where lower case ß is expected.
	// this assert fails on windows with icu 62
	// CHECK("Grüßen" == to_title("GRÜẞEN", l));

	l = icu::Locale("nl_NL");
	CHECK("Één" == to_title("één", l));
	CHECK("Één" == to_title("ÉÉN", l));
	CHECK("IJsselmeer" == to_title("ijsselmeer", l));
	CHECK("IJsselmeer" == to_title("Ijsselmeer", l));
	CHECK("IJsselmeer" == to_title("iJsselmeer", l));
	CHECK("IJsselmeer" == to_title("IJsselmeer", l));
	CHECK("IJsselmeer" == to_title("IJSSELMEER", l));
	CHECK("Ĳsselmeer" == to_title("ĳsselmeer", l));
	CHECK("Ĳsselmeer" == to_title("Ĳsselmeer", l));
	CHECK("Ĳsselmeer" == to_title("ĲSSELMEER", l));
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
	CHECK_FALSE(is_number(""));
	CHECK_FALSE(is_number("a"));
	CHECK_FALSE(is_number("1a"));
	CHECK_FALSE(is_number("a1"));
	CHECK_FALSE(is_number(".a"));
	CHECK_FALSE(is_number("a."));
	CHECK_FALSE(is_number(",a"));
	CHECK_FALSE(is_number("a,"));
	CHECK_FALSE(is_number("-a"));
	CHECK_FALSE(is_number("a-"));

	CHECK_FALSE(is_number("1..1"));
	CHECK_FALSE(is_number("1.,1"));
	CHECK_FALSE(is_number("1.-1"));
	CHECK_FALSE(is_number("1,.1"));
	CHECK_FALSE(is_number("1,,1"));
	CHECK_FALSE(is_number("1,-1"));
	CHECK_FALSE(is_number("1-.1"));
	CHECK_FALSE(is_number("1-,1"));
	CHECK_FALSE(is_number("1--1"));

	CHECK(is_number("1,1111"));
	CHECK(is_number("-1,1111"));
	CHECK(is_number("1,1111.00"));
	CHECK(is_number("-1,1111.00"));
	CHECK(is_number("1.1111"));
	CHECK(is_number("-1.1111"));
	CHECK(is_number("1.1111,00"));
	CHECK(is_number("-1.1111,00"));

	// below needs extra review

	CHECK(is_number("1"));
	CHECK(is_number("-1"));
	CHECK_FALSE(is_number("1-"));

	CHECK_FALSE(is_number("1."));
	CHECK_FALSE(is_number("-1."));
	CHECK_FALSE(is_number("1.-"));

	CHECK_FALSE(is_number("1,"));
	CHECK_FALSE(is_number("-1,"));
	CHECK_FALSE(is_number("1,-"));

	CHECK(is_number("1.1"));
	CHECK(is_number("-1.1"));
	CHECK_FALSE(is_number("1.1-"));

	CHECK(is_number("1,1"));
	CHECK(is_number("-1,1"));
	CHECK_FALSE(is_number("1,1-"));

	CHECK_FALSE(is_number(".1"));
	CHECK_FALSE(is_number("-.1"));
	CHECK_FALSE(is_number(".1-"));

	CHECK_FALSE(is_number(",1"));
	CHECK_FALSE(is_number("-,1"));
	CHECK_FALSE(is_number(",1-"));
}
