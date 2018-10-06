/* Copyright 2017-2018 Sander van Geloven
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

#include "catch.hpp"

#include <algorithm>
#include <iostream>

#include <boost/locale.hpp>

#include "../src/nuspell/locale_utils.hxx"

using namespace std;
using namespace std::literals::string_literals;
using namespace nuspell;

TEST_CASE("method utf8_to_32_alternative", "[locale_utils]")
{
	CHECK(U""s == utf8_to_32_alternative(""s));

	CHECK(U"abczĳß«абвњ\U0001FFFFерњеӤ\u0801\u0912日本にреѐ"s ==
	      utf8_to_32_alternative(
	          u8"abczĳß«абвњ\U0001FFFFерњеӤ\u0801\u0912日本にреѐ"s));
	CHECK(U"日  Ӥ" != utf8_to_32_alternative("Ӥ日本に"s));
	// need counter example too
}

TEST_CASE("method validate_utf8", "[locale_utils]")
{
	CHECK(validate_utf8(""s));
	CHECK(validate_utf8("the brown fox~"s));
	CHECK(validate_utf8("Ӥ日本に"s));
	// need counter example too
}

TEST_CASE("method is_ascii", "[locale_utils]")
{
	CHECK(is_ascii('a'));
	CHECK(is_ascii('\t'));

	CHECK_FALSE(is_ascii(char_traits<char>::to_char_type(128)));
}

TEST_CASE("method is_all_ascii", "[locale_utils]")
{
	CHECK(is_all_ascii(""s));
	CHECK(is_all_ascii("the brown fox~"s));
	CHECK_FALSE(is_all_ascii("brown foxĳӤ"s));
}

TEST_CASE("method latin1_to_ucs2", "[locale_utils]")
{
	CHECK(u"" == latin1_to_ucs2(""s));
	CHECK(u"abc\u0080" == latin1_to_ucs2("abc\x80"s));
	CHECK(u"²¿ýþÿ" != latin1_to_ucs2(u8"²¿ýþÿ"s));
	CHECK(u"Ӥ日本に" != latin1_to_ucs2(u8"Ӥ日本に"s));
}

TEST_CASE("method is_all_bmp", "[locale_utils]")
{
	CHECK(true == is_all_bmp(U"abcýþÿӤ"));
}

TEST_CASE("method u32_to_ucs2_skip_non_bmp", "[locale_utils]")
{
	CHECK(u" ABC" == u32_to_ucs2_skip_non_bmp(U"\U0010FFFF AB\U00010000C"));
}

TEST_CASE("to_wide", "[locale_utils]")
{
	auto in = u8"\U0010FFFF ß"s;
	boost::locale::generator g;
	auto loc = g("en_US.UTF-8");
	CHECK(L"\U0010FFFF ß" == to_wide(in, loc));

	in = "abcd\xDF";
	loc = g("en_US.ISO-8859-1");
	CHECK(L"abcdß" == to_wide(in, loc));
}

TEST_CASE("to_narrow", "[locale_utils]")
{
	auto in = L"\U0010FFFF ß"s;
	boost::locale::generator g;
	auto loc = g("en_US.UTF-8");
	CHECK(u8"\U0010FFFF ß" == to_narrow(in, loc));

	in = L"abcdß";
	loc = g("en_US.ISO-8859-1");
	CHECK("abcd\xDF" == to_narrow(in, loc));
}

TEST_CASE("icu ctype facets", "[locale_utils]")
{
	boost::locale::generator g;
	auto loc = g("en_US.UTF-8");
	install_ctype_facets_inplace(loc);

	// test narrow ctype
	CHECK(isupper('A', loc));
	CHECK_FALSE(isupper('a', loc));
	CHECK(islower('a', loc));
	CHECK(ispunct('.', loc));
	CHECK_FALSE(ispunct('a', loc));

	CHECK(tolower('I', loc) == 'i');
	CHECK(toupper('i', loc) == 'I');

	// test above ascii, shoud be false
	CHECK_FALSE(isupper('\xC0', loc));
	CHECK_FALSE(islower('\xC0', loc));
	CHECK_FALSE(isalnum('\xC0', loc));
	CHECK(tolower('\xC0', loc) == '\xC0');
	CHECK(toupper('\xC0', loc) == '\xC0');

	// test wide
	CHECK(isupper(L'A', loc));
	CHECK_FALSE(isupper(L'a', loc));
	CHECK(islower(L'a', loc));
	CHECK(ispunct(L'.', loc));
	CHECK_FALSE(ispunct(L'a', loc));

	CHECK(isupper(L'Ш', loc));
	CHECK_FALSE(isupper(L'ш', loc));
	CHECK(islower(L'ш', loc));
	CHECK(ispunct(L'¿', loc));
	CHECK_FALSE(ispunct(L'ш', loc));

	CHECK(tolower(L'I', loc) == L'i');
	CHECK(toupper(L'i', loc) == L'I');

	CHECK(toupper(L'г', loc) == L'Г');
	CHECK(tolower(L'Г', loc) == L'г');

	CHECK(toupper(L'У', loc) == L'У');
	CHECK(tolower(L'м', loc) == L'м');

	loc = g("ru_RU.ISO8859-5");
	install_ctype_facets_inplace(loc);

	CHECK(isupper('\xC8', loc));
	CHECK_FALSE(isupper('\xE8', loc));
	CHECK(islower('\xE8', loc));
	CHECK(ispunct(',', loc));
	CHECK_FALSE(ispunct('\xE8', loc));

	CHECK(tolower('\xC8', loc) == '\xE8'); // Ш to ш
	CHECK(tolower('\xE8', loc) == '\xE8'); // ш to ш

	CHECK(toupper('\xE8', loc) == '\xC8'); // ш to Ш
	CHECK(toupper('\xC8', loc) == '\xC8'); // Ш to Ш
}

TEST_CASE("method classify_casing", "[string_utils]")
{
	boost::locale::generator gen;
	auto loc = gen("en_US.utf-8");
	install_ctype_facets_inplace(loc);
	CHECK(Casing::SMALL == classify_casing(""s, loc));
	CHECK(Casing::SMALL == classify_casing("alllowercase"s, loc));
	CHECK(Casing::SMALL == classify_casing("alllowercase3"s, loc));
	CHECK(Casing::INIT_CAPITAL ==
	      classify_casing("Initandlowercase"s, loc));
	CHECK(Casing::INIT_CAPITAL ==
	      classify_casing("Initandlowercase_"s, loc));
	CHECK(Casing::ALL_CAPITAL == classify_casing("ALLUPPERCASE"s, loc));
	CHECK(Casing::ALL_CAPITAL == classify_casing("ALLUPPERCASE."s, loc));
	CHECK(Casing::CAMEL == classify_casing("iCamelCase"s, loc));
	CHECK(Casing::CAMEL == classify_casing("iCamelCase@"s, loc));
	CHECK(Casing::PASCAL == classify_casing("InitCamelCase"s, loc));
	CHECK(Casing::PASCAL == classify_casing("InitCamelCase "s, loc));

	CHECK_FALSE(Casing::INIT_CAPITAL == classify_casing("İstanbul"s, loc));
	CHECK(Casing::INIT_CAPITAL == classify_casing(L"İstanbul"s, loc));
	loc = gen("tr_TR.UTF-8");
	install_ctype_facets_inplace(loc);
	CHECK_FALSE(Casing::INIT_CAPITAL == classify_casing("İstanbul"s, loc));
	CHECK(Casing::INIT_CAPITAL == classify_casing(L"İstanbul"s, loc));
}

TEST_CASE("boost locale has icu", "[locale_utils]")
{
	using lbm = boost::locale::localization_backend_manager;
	auto v = lbm::global().get_all_backends();
	CHECK(std::find(v.begin(), v.end(), "icu") != v.end());
}

TEST_CASE("boost locale to_upper", "[string_utils]")
{
	// Major note here. boost::locale::to_upper is different
	// from boost::algorithm::to_upper.
	//
	// The second works on char by char basis, so it can not be used with
	// multibyte encodings, utf-8 (but is OK with wide strings) and can not
	// handle sharp s to double S. Strictly char by char.
	//
	// Here locale::to_upper is tested.
	//
	// As the active locale may vary from machine to machine, each test must
	// explicitely be provided with a locale.

	boost::locale::generator gen;
	using boost::locale::to_upper;
	auto l = gen("en_US.UTF-8");

	CHECK(""s == to_upper(""s, l));
	CHECK("A"s == to_upper("a"s, l));
	CHECK("A"s == to_upper("A"s, l));
	CHECK("AA"s == to_upper("aa"s, l));
	CHECK("AA"s == to_upper("aA"s, l));
	CHECK("AA"s == to_upper("Aa"s, l));
	CHECK("AA"s == to_upper("AA"s, l));

	CHECK("TABLE"s == to_upper("table"s, l));
	CHECK("TABLE"s == to_upper("Table"s, l));
	CHECK("TABLE"s == to_upper("tABLE"s, l));
	CHECK("TABLE"s == to_upper("TABLE"s, l));

	// Note that i is converted to I, not İ
	CHECK_FALSE("İSTANBUL"s == to_upper("istanbul"s, l));
	l = gen("tr_TR.UTF-8");
	CHECK("İSTANBUL"s == to_upper("istanbul"s, l));
	// Note that I remains and is not converted to İ
	CHECK_FALSE("İSTANBUL"s == to_upper("Istanbul"s, l));
	CHECK("DİYARBAKIR"s == to_upper("Diyarbakır"s, l));

#if 0
	// 201805, currently the following six tests are failing on Travis
	l = g("el_GR.UTF-8");
	CHECK("ΕΛΛΑΔΑ"s == to_upper("ελλάδα"s, l)); // was ΕΛΛΆΔΑ up to
	CHECK("ΕΛΛΑΔΑ"s == to_upper("Ελλάδα"s, l)); // was ΕΛΛΆΔΑ up to
	CHECK("ΕΛΛΑΔΑ"s == to_upper("ΕΛΛΆΔΑ"s, l)); // was ΕΛΛΆΔΑ up to
	CHECK("ΣΙΓΜΑ"s == to_upper("Σίγμα"s, l));   // was ΣΊΓΜΑ up to
	CHECK("ΣΙΓΜΑ"s == to_upper("σίγμα"s, l));   // was ΣΊΓΜΑ up to

	// Use of ς where σ is expected, should convert to upper case Σ.
	CHECK("ΣΙΓΜΑ"s == to_upper("ςίγμα"s, l)); // was ΣΊΓΜΑ up to
#endif

	l = gen("de_DE.UTF-8");
	// Note that lower case ü is not converted to upper case Ü.
	// Note that lower case ß is converted to double SS.
	// CHECK("GRüSSEN"s == to_upper("grüßen"s, l));
	CHECK("GRÜSSEN"s == to_upper("GRÜßEN"s, l));
	// Note that upper case ẞ is kept in upper case.
	CHECK("GRÜẞEN"s == to_upper("GRÜẞEN"s, l));

	l = gen("nl_NL.UTF-8");
	CHECK("ÉÉN"s == to_upper("één"s, l));
	CHECK("ÉÉN"s == to_upper("Één"s, l));
	CHECK("IJSSELMEER"s == to_upper("ijsselmeer"s, l));
	CHECK("IJSSELMEER"s == to_upper("IJsselmeer"s, l));
	CHECK("IJSSELMEER"s == to_upper("IJSSELMEER"s, l));
	CHECK("ĲSSELMEER"s == to_upper("ĳsselmeer"s, l));
	CHECK("ĲSSELMEER"s == to_upper("Ĳsselmeer"s, l));
	CHECK("ĲSSELMEER"s == to_upper("ĲSSELMEER"s, l));
}

TEST_CASE("boost locale to_lower", "[string_utils]")
{
	// Major note here. boost::locale::to_lower is different
	// from boost::algorithm::to_lower.
	//
	// The second works on char by char basis, so it can not be used with
	// multibyte encodings, utf-8 (but is OK with wide strings) and can not
	// handle sharp s to double S. Strictly char by char.
	//
	// Here locale::to_lower is tested.
	//
	// As the active locale may vary from machine to machine, each test must
	// explicitely be provided with a locale.

	boost::locale::generator gen;
	using boost::locale::to_lower;
	auto l = gen("en_US.UTF-8");

	CHECK(""s == to_lower(""s, l));
	CHECK("a"s == to_lower("A"s, l));
	CHECK("a"s == to_lower("a"s, l));
	CHECK("aa"s == to_lower("aa"s, l));
	CHECK("aa"s == to_lower("aA"s, l));
	CHECK("aa"s == to_lower("Aa"s, l));
	CHECK("aa"s == to_lower("AA"s, l));

	CHECK("table"s == to_lower("table"s, l));
	CHECK("table"s == to_lower("Table"s, l));
	CHECK("table"s == to_lower("TABLE"s, l));

	// Note that İ is converted to i followed by COMBINING DOT ABOVE U+0307
	CHECK_FALSE("istanbul"s == to_lower("İSTANBUL"s, l));
	// Note that İ is converted to i followed by COMBINING DOT ABOVE U+0307
	CHECK_FALSE("istanbul"s == to_lower("İstanbul"s, l));

	l = gen("tr_TR.UTF-8");
	CHECK("istanbul"s == to_lower("İSTANBUL"s, l));
	CHECK("istanbul"s == to_lower("İstanbul"s, l));
	CHECK("diyarbakır"s == to_lower("Diyarbakır"s, l));

	l = gen("el_GR.UTF-8");
	CHECK("ελλάδα"s == to_lower("ελλάδα"s, l));
	CHECK("ελλάδα"s == to_lower("Ελλάδα"s, l));
	CHECK("ελλάδα"s == to_lower("ΕΛΛΆΔΑ"s, l));

	l = gen("de_DE.UTF-8");
	CHECK("grüßen"s == to_lower("grüßen"s, l));
	CHECK("grüssen"s == to_lower("grüssen"s, l));
	// Note that double SS is not converted to lower case ß.
	CHECK("grüssen"s == to_lower("GRÜSSEN"s, l));
	// Note that upper case ẞ is converted to lower case ß.
	CHECK("grüßen"s == to_lower("GRÜẞEN"s, l));

	l = gen("nl_NL.UTF-8");
	CHECK("één"s == to_lower("Één"s, l));
	CHECK("één"s == to_lower("ÉÉN"s, l));
	CHECK("ijsselmeer"s == to_lower("ijsselmeer"s, l));
	CHECK("ijsselmeer"s == to_lower("IJsselmeer"s, l));
	CHECK("ijsselmeer"s == to_lower("IJSSELMEER"s, l));
	CHECK("ĳsselmeer"s == to_lower("Ĳsselmeer"s, l));
	CHECK("ĳsselmeer"s == to_lower("ĲSSELMEER"s, l));
	CHECK("ĳsselmeer"s == to_lower("Ĳsselmeer"s, l));
}

TEST_CASE("boost locale to_title", "[string_utils]")
{
	// Here locale::to_upper is tested.
	//
	// As the active locale may vary from machine to machine, each test must
	// explicitely be provided with a locale.

	boost::locale::generator gen;
	using boost::locale::to_title;

	auto l = gen("en_US.UTF-8");
	CHECK(""s == to_title(""s, l));
	CHECK("A"s == to_title("a"s, l));
	CHECK("A"s == to_title("A"s, l));
	CHECK("Aa"s == to_title("aa"s, l));
	CHECK("Aa"s == to_title("Aa"s, l));
	CHECK("Aa"s == to_title("aA"s, l));
	CHECK("Aa"s == to_title("AA"s, l));

	CHECK("Table"s == to_title("table"s, l));
	CHECK("Table"s == to_title("Table"s, l));
	CHECK("Table"s == to_title("tABLE"s, l));
	CHECK("Table"s == to_title("TABLE"s, l));

	// Note that i is converted to I, not İ
	CHECK_FALSE("İstanbul"s == to_title("istanbul"s, l));
	// Note that i is converted to I, not İ
	CHECK_FALSE("İstanbul"s == to_title("iSTANBUL"s, l));
	CHECK("İstanbul"s == to_title("İSTANBUL"s, l));
	CHECK("Istanbul"s == to_title("ISTANBUL"s, l));

	l = gen("tr_TR.UTF-8");
	CHECK("İstanbul"s == to_title("istanbul"s, l));
	CHECK("İstanbul"s == to_title("iSTANBUL"s, l));
	CHECK("İstanbul"s == to_title("İSTANBUL"s, l));
	CHECK("Istanbul"s == to_title("ISTANBUL"s, l));
	CHECK("Diyarbakır"s == to_title("diyarbakır"s, l));
	l = gen("tr_CY.UTF-8");
	CHECK("İstanbul"s == to_title("istanbul"s, l));
	l = gen("crh_UA.UTF-8");
	// Note that lower case i is not converted to upper case İ, bug?
	CHECK("Istanbul"s == to_title("istanbul"s, l));
	l = gen("az_AZ.UTF-8");
	CHECK("İstanbul"s == to_title("istanbul"s, l));
	l = gen("az_IR.UTF-8");
	CHECK("İstanbul"s == to_title("istanbul"s, l));

	l = gen("el_GR.UTF-8");
	CHECK("Ελλάδα"s == to_title("ελλάδα"s, l));
	CHECK("Ελλάδα"s == to_title("Ελλάδα"s, l));
	CHECK("Ελλάδα"s == to_title("ΕΛΛΆΔΑ"s, l));
	CHECK("Σίγμα"s == to_title("Σίγμα"s, l));
	CHECK("Σίγμα"s == to_title("σίγμα"s, l));
	// Use of ς where σ is expected, should convert to upper case Σ.
	CHECK("Σίγμα"s == to_title("ςίγμα"s, l));

	l = gen("de_DE.UTF-8");
	CHECK("Grüßen"s == to_title("grüßen"s, l));
	CHECK("Grüßen"s == to_title("GRÜßEN"s, l));
	// Use of upper case ẞ where lower case ß is expected.
	CHECK("Grüßen"s == to_title("GRÜẞEN"s, l));

	l = gen("nl_NL.UTF-8");
	CHECK("Één"s == to_title("één"s, l));
	CHECK("Één"s == to_title("ÉÉN"s, l));
	CHECK("IJsselmeer"s == to_title("ijsselmeer"s, l));
	CHECK("IJsselmeer"s == to_title("Ijsselmeer"s, l));
	CHECK("IJsselmeer"s == to_title("iJsselmeer"s, l));
	CHECK("IJsselmeer"s == to_title("IJsselmeer"s, l));
	CHECK("IJsselmeer"s == to_title("IJSSELMEER"s, l));
	CHECK("Ĳsselmeer"s == to_title("ĳsselmeer"s, l));
	CHECK("Ĳsselmeer"s == to_title("Ĳsselmeer"s, l));
	CHECK("Ĳsselmeer"s == to_title("ĲSSELMEER"s, l));
}

TEST_CASE("Encoding", "[locale_utils]")
{
	auto e = Encoding();
	auto v = e.value_or_default();
	auto i = e.is_utf8();
	CHECK("ISO8859-1"s == v);
	CHECK(false == i);

	e = Encoding("UTF8");
	v = e.value();
	i = e.is_utf8();
	CHECK("UTF-8"s == v);
	CHECK(true == i);

	e = "MICROSOFT-CP1251"s;
	v = e.value();
	i = e.is_utf8();
	CHECK("CP1251"s == v);
	CHECK(false == i);
}
