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

#include "../src/nuspell/string_utils.hxx"
#include "../src/nuspell/structures.hxx"

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
	exp = vector<string>{};
	// out = vector<string>();
	split_on_whitespace_v(in, out);
	CHECK(exp == out);
}

#if 0
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
	// explicitely be provided withw a locale.

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

	l = g("el_GR.UTF-8");
	CHECK("ΕΛΛΆΔΑ"s == to_upper("ελλάδα"s, l));
	CHECK("ΕΛΛΆΔΑ"s == to_upper("Ελλάδα"s, l));
	CHECK("ΕΛΛΆΔΑ"s == to_upper("ΕΛΛΆΔΑ"s, l));
	CHECK("ΣΊΓΜΑ"s == to_upper("Σίγμα"s, l));
	CHECK("ΣΊΓΜΑ"s == to_upper("σίγμα"s, l));
	// Use of ς where σ is expected, should convert to upper case Σ.
	CHECK("ΣΊΓΜΑ"s == to_upper("ςίγμα"s, l));

	l = g("de_DE.UTF-8");
	// Note that lower case ü is not converted to upper case Ü.
	// Note that lower case ß is converted to double SS.
	// CHECK("GRüSSEN"s == to_upper("grüßen"s, l));
	CHECK("GRÜSSEN"s == to_upper("GRÜßEN"s, l));
	// Note that upper case ẞ is kept in upper case.
	CHECK("GRÜẞEN"s == to_upper("GRÜẞEN"s, l));

	l = g("nl_NL.UTF-8");
	CHECK("IJSSELMEER"s == to_upper("ijsselmeer"s, l));
	CHECK("IJSSELMEER"s == to_upper("IJsselmeer"s, l));
	CHECK("IJSSELMEER"s == to_upper("IJSSELMEER"s, l));
	CHECK("ĲSSELMEER"s == to_upper("ĳsselmeer"s, l));
	CHECK("ĲSSELMEER"s == to_upper("Ĳsselmeer"s, l));
	CHECK("ĲSSELMEER"s == to_upper("ĲSSELMEER"s, l));

	// TODO Add some Arabic and Hebrew examples.
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
	// explicitely be provided withw a locale.

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
	CHECK("ijsselmeer"s == to_lower("ijsselmeer"s, l));
	CHECK("ijsselmeer"s == to_lower("IJsselmeer"s, l));
	CHECK("ijsselmeer"s == to_lower("IJSSELMEER"s, l));
	CHECK("ĳsselmeer"s == to_lower("Ĳsselmeer"s, l));
	CHECK("ĳsselmeer"s == to_lower("ĲSSELMEER"s, l));
	CHECK("ĳsselmeer"s == to_lower("Ĳsselmeer"s, l));

	// TODO Add some Arabic and Hebrew examples.
}

TEST_CASE("method to_title", "[string_utils]")
{
	// Here locale::to_upper is tested.
	//
	// As the active locale may vary from machine to machine, each test must
	// explicitely be provided withw a locale.

	boost::locale::generator g;
	using boost::locale::to_title;
	// Also std::locale can be the starting point in the code.
	// Note that any std::locale used, should first be made available on the
	// machine the test is run with
	//     $ sudo dpkg-reconfigure locales
	// This is not needed for any of the boost::locale used in the
	// non-English tests below.
	auto l = g(std::locale("en_US.UTF-8").name());
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
	CHECK("IJsselmeer"s == to_title("ijsselmeer"s, l));
	CHECK("IJsselmeer"s == to_title("Ijsselmeer"s, l));
	CHECK("IJsselmeer"s == to_title("iJsselmeer"s, l));
	CHECK("IJsselmeer"s == to_title("IJsselmeer"s, l));
	CHECK("IJsselmeer"s == to_title("IJSSELMEER"s, l));
	CHECK("Ĳsselmeer"s == to_title("ĳsselmeer"s, l));
	CHECK("Ĳsselmeer"s == to_title("Ĳsselmeer"s, l));
	CHECK("Ĳsselmeer"s == to_title("ĲSSELMEER"s, l));

	// TODO Add some Arabic and Hebrew examples.
}
#endif

TEST_CASE("method classify_casing", "[string_utils]")
{
	CHECK(Casing::SMALL == classify_casing("alllowercase"s));
	CHECK(Casing::SMALL == classify_casing("alllowercase3"s));
	CHECK(Casing::INIT_CAPITAL == classify_casing("Initandlowercase"s));
	CHECK(Casing::INIT_CAPITAL == classify_casing("Initandlowercase_"s));
	CHECK(Casing::ALL_CAPITAL == classify_casing("ALLUPPERCASE"s));
	CHECK(Casing::ALL_CAPITAL == classify_casing("ALLUPPERCASE."s));
	CHECK(Casing::CAMEL == classify_casing("iCamelCase"s));
	CHECK(Casing::CAMEL == classify_casing("iCamelCase@"s));
	CHECK(Casing::PASCAL == classify_casing("InitCamelCase"s));
	CHECK(Casing::PASCAL == classify_casing("InitCamelCase "s));
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
	CHECK(false == is_number("1,,1"s));
	CHECK(false == is_number("1--1"s));

	CHECK(false == is_number("1.,1"s));
	CHECK(false == is_number("1.-1"s));
	CHECK(false == is_number("1,.1"s));
	CHECK(false == is_number("1,-1"s));
	CHECK(false == is_number("1-.1"s));
	CHECK(false == is_number("1-,1"s));

	// should this be true for wider mathcing? old code results in false
	CHECK(false == is_number("-.1"s));
	CHECK(false == is_number("1."s));
	CHECK(false == is_number("1,"s));

	CHECK(false == is_number("1-"s));
	CHECK(false == is_number("1.1-"s));
	CHECK(false == is_number("1,1-"s));

	CHECK(true == is_number("1"s));
	CHECK(true == is_number("-1"s));
	CHECK(true == is_number("1.1"s));
	CHECK(true == is_number("-1.1"s));
	CHECK(true == is_number("1,1"s));
	CHECK(true == is_number("-1,1"s));

	CHECK(true == is_number("1,1111"s));
	CHECK(true == is_number("-1,1111"s));
	CHECK(true == is_number("1,1111.00"s));
	CHECK(true == is_number("-1,1111.00"s));
	CHECK(true == is_number("1.1111"s));
	CHECK(true == is_number("-1.1111"s));
	CHECK(true == is_number("1.1111,00"s));
	CHECK(true == is_number("-1.1111,00"s));
}
