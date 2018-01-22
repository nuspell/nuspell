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
 * but WITHOUT ANY WARRANTY; withto_upper( even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Nuspell.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "catch.hpp"

#include <iostream>

#include "../src/nuspell/string_utils.hxx"

using namespace std;
using namespace std::literals;
using namespace hunspell;

TEST_CASE("method split_on_any_of", "[string_utils]")
{
	auto in = string("^abc;.qwe/zxc/");
	auto exp = vector<string>{"", "abc", "", "qwe", "zxc", ""};
	auto out = vector<string>();
	split_on_any_of(in, string(".;^/"), back_inserter(out));
	CHECK(exp == out);
}

TEST_CASE("method split", "[string_utils]")
{
	// tests split(), also tests split_v

	auto in = string(";abc;;qwe;zxc;");
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
	auto in = string("first\tsecond");
	auto first = string("first");
	auto out = split_first(in, '\t');
	CHECK(first == out);

	in = string("first");
	out = split_first(in, '\t');
	CHECK(first == out);

	in = string("\tsecond");
	first = string("");
	out = split_first(in, '\t');
	CHECK(first == out);

	in = string("");
	first = string("");
	out = split_first(in, '\t');
	CHECK(first == out);
}

TEST_CASE("method split_on_whitespace", "[string_utils]")
{
	// also tests _v variant

	auto in = string("   qwe ert  \tasd ");
	auto exp = vector<string>{"qwe", "ert", "asd"};
	auto out = vector<string>();
	split_on_whitespace_v(in, out);
	CHECK(exp == out);

	in = string("   \t\r\n  ");
	exp = vector<string>{};
	// out = vector<string>();
	split_on_whitespace_v(in, out);
	CHECK(exp == out);
}

TEST_CASE("method to_upper", "[string_utils]")
{
	CHECK(string("") == to_upper(string("")));
	CHECK(string("A") == to_upper(string("a")));
	CHECK(string("AA") == to_upper(string("aA")));
	CHECK(string("AA") == to_upper(string("Aa")));
	CHECK(string("AA") == to_upper(string("AA")));

	boost::locale::generator g;
	auto l = g("en_US.UTF-8");
	CHECK(string("TABLE") == to_upper(string("table"), l));
	CHECK(string("TABLE") == to_upper(string("Table"), l));
	CHECK(string("TABLE") == to_upper(string("tABLE"), l));
	CHECK(string("TABLE") == to_upper(string("TABLE"), l));

	l = g("el_GR.UTF-8");
	CHECK(string("ΕΛΛΆΔΑ") == to_upper(string("ελλάδα"), l));
	CHECK(string("ΕΛΛΆΔΑ") == to_upper(string("Ελλάδα"), l));
	CHECK(string("ΕΛΛΆΔΑ") == to_upper(string("ΕΛΛΆΔΑ"), l));
	CHECK(string("ΣΊΓΜΑ") == to_upper(string("Σίγμα"), l));
	CHECK(string("ΣΊΓΜΑ") == to_upper(string("σίγμα"), l));
	// Use of ς where σ is expected, should convert to upper case Σ.
	CHECK(string("ΣΊΓΜΑ") == to_upper(string("ςίγμα"), l));

	l = g("de_DE.UTF-8");
	// Note that lower case ü is not converted to upper case Ü.
	// Note that lower case ß is not converted to upper case ẞ.
	CHECK(string("GRüßEN") == to_upper(string("grüßen"), l));
	// Note that upper case Ü trigger a result in all upper case.
	CHECK(string("GRÜßEN") == to_upper(string("GRÜßEN"), l));
	// Note that upper case ẞ is kept in upper case.
	CHECK(string("GRÜẞEN") == to_upper(string("GRÜẞEN"), l));

	l = g("nl_NL.UTF-8");
	CHECK(string("IJSSELMEER") == to_upper(string("ijsselmeer"), l));
	CHECK(string("IJSSELMEER") == to_upper(string("IJsselmeer"), l));
	CHECK(string("IJSSELMEER") == to_upper(string("IJSSELMEER"), l));
	CHECK(string("ĲSSELMEER") == to_upper(string("ĳsselmeer"), l));
	CHECK(string("ĲSSELMEER") == to_upper(string("Ĳsselmeer"), l));
	CHECK(string("ĲSSELMEER") == to_upper(string("ĲSSELMEER"), l));

	// TODO Add some Arabic and Hebrew examples.
}

TEST_CASE("method capitalize", "[string_utils]")
{
	// FIXME The block below should work without locale.
	// It throws an unexpected exception with message:
	// std::bad_cast
	// If the to_upper is changed from:
	// return boost::to_upper_copy(s, loc);
	// to
	// return boost::locale::to_upper(s, loc);
	// the errors appear.
	// However, a there is no way to use
	// return boost::to_title(s, loc)
	// as it does not exist. If it was existing, it would fix the default
	// locale problem.
	CHECK(string("") == capitalize(string("")));
	CHECK(string("A") == capitalize(string("a")));
	CHECK(string("A") == capitalize(string("A")));
	CHECK(string("Aa") == capitalize(string("Aa")));
	CHECK(string("Aa") == capitalize(string("aA")));
	CHECK(string("Aa") == capitalize(string("AA")));

	boost::locale::generator g;
	auto l = g("en_US.UTF-8");
	CHECK(string("Table") == capitalize(string("table"), l));
	CHECK(string("Table") == capitalize(string("Table"), l));
	CHECK(string("Table") == capitalize(string("tABLE"), l));
	CHECK(string("Table") == capitalize(string("TABLE"), l));

	l = g("el_GR.UTF-8");
	CHECK(string("Ελλάδα") == capitalize(string("ελλάδα"), l));
	CHECK(string("Ελλάδα") == capitalize(string("Ελλάδα"), l));
	CHECK(string("Ελλάδα") == capitalize(string("ΕΛΛΆΔΑ"), l));
	CHECK(string("Σίγμα") == capitalize(string("Σίγμα"), l));
	CHECK(string("Σίγμα") == capitalize(string("σίγμα"), l));
	// Use of ς where σ is expected, should convert to upper case Σ.
	CHECK(string("Σίγμα") == capitalize(string("ςίγμα"), l));

	l = g("de_DE.UTF-8");
	CHECK(string("Grüßen") == capitalize(string("grüßen"), l));
	CHECK(string("Grüßen") == capitalize(string("GRÜßEN"), l));
	// Use of upper case ẞ where lower case ß is expected.
	CHECK(string("Grüßen") == capitalize(string("GRÜẞEN"), l));

	l = g("nl_NL.UTF-8");
	CHECK(string("IJsselmeer") == capitalize(string("ijsselmeer"), l));
	CHECK(string("IJsselmeer") == capitalize(string("Ijsselmeer"), l));
	CHECK(string("IJsselmeer") == capitalize(string("iJsselmeer"), l));
	CHECK(string("IJsselmeer") == capitalize(string("IJsselmeer"), l));
	CHECK(string("IJsselmeer") == capitalize(string("IJSSELMEER"), l));
	CHECK(string("Ĳsselmeer") == capitalize(string("ĳsselmeer"), l));
	CHECK(string("Ĳsselmeer") == capitalize(string("Ĳsselmeer"), l));
	CHECK(string("Ĳsselmeer") == capitalize(string("ĲSSELMEER"), l));

	// TODO Add some Arabic and Hebrew examples.
}

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
