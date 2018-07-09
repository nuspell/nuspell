/* Copyright 2017 Sander van Geloven
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

#include <boost/locale.hpp>
#include <iostream>

#include "../src/nuspell/locale_utils.hxx"
#include "../src/nuspell/string_utils.hxx"

using namespace std;
using namespace std::literals::string_literals;
using namespace nuspell;

TEST_CASE("method split_on_any_of", "[string_utils]")
{
	auto in = "^abc;.qwe/zxc/"s;
	auto exp = vector<string>{"", "abc", "", "qwe", "zxc", ""};
	auto out = vector<string>();
	split_on_any_of(in, ".;^/"s, back_inserter(out));
	CHECK(exp == out);
}

TEST_CASE("method split", "[string_utils]")
{
	// tests split(), also tests split_v

	auto in = ";abc;;qwe;zxc;"s;
	auto exp = vector<string>{"", "abc", "", "qwe", "zxc", ""};
	auto out = vector<string>();
	split_v(in, ';', out);
	CHECK(exp == out);

	in = "<>1<>234<>qwe<==><><>";
	exp = {"", "1", "234", "qwe<==>", "", ""};
	split_v(in, "<>", out);
	CHECK(exp == out);
}

TEST_CASE("method split_first", "[string_utils]")
{
	auto in = "first\tsecond"s;
	auto first = "first"s;
	auto out = split_first(in, '\t');
	CHECK(first == out);

	in = "first"s;
	out = split_first(in, '\t');
	CHECK(first == out);

	in = "\tsecond"s;
	first = ""s;
	out = split_first(in, '\t');
	CHECK(first == out);

	in = ""s;
	first = ""s;
	out = split_first(in, '\t');
	CHECK(first == out);
}

TEST_CASE("method split_on_whitespace", "[string_utils]")
{
	// also tests _v variant

	auto in = "   qwe ert  \tasd "s;
	auto exp = vector<string>{"qwe", "ert", "asd"};
	auto out = vector<string>();
	split_on_whitespace_v(in, out);
	CHECK(exp == out);

	in = "   \t\r\n  "s;
	exp = vector<string>();
	split_on_whitespace_v(in, out);
	CHECK(exp == out);

#if 0
	// https://en.wikipedia.org/wiki/Whitespace_character
	in =
	    "2000 2001 2002 2003 2004 2005 2006 2007 2008 2009 200a 202f 205f 3000　end"s;
	// contains:
	// U+1680 ogham space mark
	// U+2000 en quad
	// U+2001 em quad
	// U+2002 en space
	// U+2003 em space
	// U+2004 zero width non-joiner
	// U+2005 four-per-em space
	// U+2006 six-per-em space
	// U+2007 figure space
	// U+2008 punctuation space
	// U+2009 thin space
	// U+200a hair space
	// U+202f narrow no-break space
	// U+205f medium mathematical space
	// U+3000 ideographic space
	exp = vector<string>{
	    "2000", "2001", "2002", "2003", "2004", "2005",
	    "2006", "2007", "2008", "2009", "200a", "202f",
	    "205f", "3000", "end",
	};
	split_on_whitespace_v(in, out);
	CHECK(exp == out);
#endif
}

TEST_CASE("method to_upper", "[string_utils]")
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

	boost::locale::generator g;
	using boost::locale::to_upper;
	// Also std::locale can be the starting point in the code.
	// Note that any std::locale used, should first be made available on the
	// machine the test is run with
	//     $ sudo dpkg-reconfigure locales
	// This is not needed for any of the boost::locale used in the
	// non-English tests below.
	auto l = g(std::locale("en_US.UTF-8").name());

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
	l = g("tr_TR.UTF-8");
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

	l = g("de_DE.UTF-8");
	// Note that lower case ü is not converted to upper case Ü.
	// Note that lower case ß is converted to double SS.
	// CHECK("GRüSSEN"s == to_upper("grüßen"s, l));
	CHECK("GRÜSSEN"s == to_upper("GRÜßEN"s, l));
	// Note that upper case ẞ is kept in upper case.
	CHECK("GRÜẞEN"s == to_upper("GRÜẞEN"s, l));

	l = g("nl_NL.UTF-8");
	CHECK("ÉÉN"s == to_upper("één"s, l));
	CHECK("ÉÉN"s == to_upper("Één"s, l));
	CHECK("IJSSELMEER"s == to_upper("ijsselmeer"s, l));
	CHECK("IJSSELMEER"s == to_upper("IJsselmeer"s, l));
	CHECK("IJSSELMEER"s == to_upper("IJSSELMEER"s, l));
	CHECK("ĲSSELMEER"s == to_upper("ĳsselmeer"s, l));
	CHECK("ĲSSELMEER"s == to_upper("Ĳsselmeer"s, l));
	CHECK("ĲSSELMEER"s == to_upper("ĲSSELMEER"s, l));
}

TEST_CASE("method to_lower", "[string_utils]")
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

	boost::locale::generator g;
	using boost::locale::to_lower;
	// Also std::locale can be the starting point in the code.
	// Note that any std::locale used, should first be made available on the
	// machine the test is run with
	//     $ sudo dpkg-reconfigure locales
	// This is not needed for any of the boost::locale used in the
	// non-English tests below.
	auto l = g(std::locale("en_US.UTF-8").name());

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

	l = g("tr_TR.UTF-8");
	CHECK("istanbul"s == to_lower("İSTANBUL"s, l));
	CHECK("istanbul"s == to_lower("İstanbul"s, l));
	CHECK("diyarbakır"s == to_lower("Diyarbakır"s, l));

	l = g("el_GR.UTF-8");
	CHECK("ελλάδα"s == to_lower("ελλάδα"s, l));
	CHECK("ελλάδα"s == to_lower("Ελλάδα"s, l));
	CHECK("ελλάδα"s == to_lower("ΕΛΛΆΔΑ"s, l));

	l = g("de_DE.UTF-8");
	CHECK("grüßen"s == to_lower("grüßen"s, l));
	CHECK("grüssen"s == to_lower("grüssen"s, l));
	// Note that double SS is not converted to lower case ß.
	CHECK("grüssen"s == to_lower("GRÜSSEN"s, l));
	// Note that upper case ẞ is converted to lower case ß.
	CHECK("grüßen"s == to_lower("GRÜẞEN"s, l));

	l = g("nl_NL.UTF-8");
	CHECK("één"s == to_lower("Één"s, l));
	CHECK("één"s == to_lower("ÉÉN"s, l));
	CHECK("ijsselmeer"s == to_lower("ijsselmeer"s, l));
	CHECK("ijsselmeer"s == to_lower("IJsselmeer"s, l));
	CHECK("ijsselmeer"s == to_lower("IJSSELMEER"s, l));
	CHECK("ĳsselmeer"s == to_lower("Ĳsselmeer"s, l));
	CHECK("ĳsselmeer"s == to_lower("ĲSSELMEER"s, l));
	CHECK("ĳsselmeer"s == to_lower("Ĳsselmeer"s, l));
}

TEST_CASE("method to_title", "[string_utils]")
{
	// Here locale::to_upper is tested.
	//
	// As the active locale may vary from machine to machine, each test must
	// explicitely be provided with a locale.

	boost::locale::generator g;
	using boost::locale::to_title;
	// Also std::locale can be the starting point in the code.
	// Note that any std::locale used, should first be made available on the
	// machine the test is run with
	//     $ sudo dpkg-reconfigure locales
	// This is not needed for any of the boost::locale used in the
	// non-English tests below.
	auto l = g("en_US.UTF-8");
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

	l = g("tr_TR.UTF-8");
	CHECK("İstanbul"s == to_title("istanbul"s, l));
	CHECK("İstanbul"s == to_title("iSTANBUL"s, l));
	CHECK("İstanbul"s == to_title("İSTANBUL"s, l));
	CHECK("Istanbul"s == to_title("ISTANBUL"s, l));
	CHECK("Diyarbakır"s == to_title("diyarbakır"s, l));
	l = g("tr_CY.UTF-8");
	CHECK("İstanbul"s == to_title("istanbul"s, l));
	l = g("crh_UA.UTF-8");
	// Note that lower case i is not converted to upper case İ, bug?
	CHECK("Istanbul"s == to_title("istanbul"s, l));
	l = g("az_AZ.UTF-8");
	CHECK("İstanbul"s == to_title("istanbul"s, l));
	l = g("az_IR.UTF-8");
	CHECK("İstanbul"s == to_title("istanbul"s, l));

	l = g("el_GR.UTF-8");
	CHECK("Ελλάδα"s == to_title("ελλάδα"s, l));
	CHECK("Ελλάδα"s == to_title("Ελλάδα"s, l));
	CHECK("Ελλάδα"s == to_title("ΕΛΛΆΔΑ"s, l));
	CHECK("Σίγμα"s == to_title("Σίγμα"s, l));
	CHECK("Σίγμα"s == to_title("σίγμα"s, l));
	// Use of ς where σ is expected, should convert to upper case Σ.
	CHECK("Σίγμα"s == to_title("ςίγμα"s, l));

	l = g("de_DE.UTF-8");
	CHECK("Grüßen"s == to_title("grüßen"s, l));
	CHECK("Grüßen"s == to_title("GRÜßEN"s, l));
	// Use of upper case ẞ where lower case ß is expected.
	CHECK("Grüßen"s == to_title("GRÜẞEN"s, l));

	l = g("nl_NL.UTF-8");
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

TEST_CASE("method classify_casing", "[string_utils]")
{
	boost::locale::generator g;
	auto loc = g("en_US.utf-8");
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
	loc = g("tr_TR.UTF-8");
	install_ctype_facets_inplace(loc);
	CHECK_FALSE(Casing::INIT_CAPITAL == classify_casing("İstanbul"s, loc));
	CHECK(Casing::INIT_CAPITAL == classify_casing(L"İstanbul"s, loc));
}

TEST_CASE("method is_number", "[string_utils]")
{
	CHECK(false == is_number(""s));
	CHECK(false == is_number("a"s));
	CHECK(false == is_number("1a"s));
	CHECK(false == is_number("a1"s));
	CHECK(false == is_number(".a"s));
	CHECK(false == is_number("a."s));
	CHECK(false == is_number(",a"s));
	CHECK(false == is_number("a,"s));
	CHECK(false == is_number("-a"s));
	CHECK(false == is_number("a-"s));

	CHECK(false == is_number("1..1"s));
	CHECK(false == is_number("1.,1"s));
	CHECK(false == is_number("1.-1"s));
	CHECK(false == is_number("1,.1"s));
	CHECK(false == is_number("1,,1"s));
	CHECK(false == is_number("1,-1"s));
	CHECK(false == is_number("1-.1"s));
	CHECK(false == is_number("1-,1"s));
	CHECK(false == is_number("1--1"s));

	CHECK(true == is_number("1,1111"s));
	CHECK(true == is_number("-1,1111"s));
	CHECK(true == is_number("1,1111.00"s));
	CHECK(true == is_number("-1,1111.00"s));
	CHECK(true == is_number("1.1111"s));
	CHECK(true == is_number("-1.1111"s));
	CHECK(true == is_number("1.1111,00"s));
	CHECK(true == is_number("-1.1111,00"s));

	// below needs extra review

	CHECK(true == is_number("1"s));
	CHECK(true == is_number("-1"s));
	CHECK(false == is_number("1-"s));

	CHECK(false == is_number("1."s));
	CHECK(false == is_number("-1."s));
	CHECK(false == is_number("1.-"s));

	CHECK(false == is_number("1,"s));
	CHECK(false == is_number("-1,"s));
	CHECK(false == is_number("1,-"s));

	CHECK(true == is_number("1.1"s));
	CHECK(true == is_number("-1.1"s));
	CHECK(false == is_number("1.1-"s));

	CHECK(true == is_number("1,1"s));
	CHECK(true == is_number("-1,1"s));
	CHECK(false == is_number("1,1-"s));

	CHECK(false == is_number(".1"s));
	CHECK(false == is_number("-.1"s));
	CHECK(false == is_number(".1-"s));

	CHECK(false == is_number(",1"s));
	CHECK(false == is_number("-,1"s));
	CHECK(false == is_number(",1-"s));
}
