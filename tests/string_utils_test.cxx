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
	auto in = string("");
	auto out = string();
	out = to_upper(in);
	CHECK(in == out);

	in = string("a");
	out = to_upper(in);
	CHECK(string("A") == out);
	CHECK(string("a") == in); // Check that input has not been affected.
	in = string("aA");
	out = to_upper(in);
	CHECK(string("AA") == out);
	in = string("Aa");
	out = to_upper(in);
	CHECK(string("AA") == out);
	in = string("AA");
	out = to_upper(in);
	CHECK(string("AA") == out);

	in = string("ελλάδα");
	out = to_upper(in);
	// FIXME CHECK(string("ΕΛΛΆΔΑ") == out);

	in = string("grüßen");
	out = to_upper(in);
	// FIXME CHECK(string("GRÜẞEN") == out);

	in = string("ijsselmeer");
	out = to_upper(in);
	CHECK(string("IJSSELMEER") == out);
	in = string("IJsselmeer");
	out = to_upper(in);
	CHECK(string("IJSSELMEER") == out);

	in = string("ĳsselmeer");
	out = to_upper(in);
	// FIXME CHECK(string("ĲSSELMEER") == out);
	in = string("Ĳsselmeer");
	out = to_upper(in);
	CHECK(string("ĲSSELMEER") == out);

	// TODO Add some Arabic and Hebrew examples.
}

TEST_CASE("method capitalize", "[string_utils]")
{
	auto in = string("");
	auto out = string();
	out = capitalize(in);
	CHECK(string("") == out);

	in = string("a");
	out = capitalize(in);
	CHECK(string("A") == out);
	CHECK(string("a") == in); // Check that input has not been affected.
	in = string("A");
	out = capitalize(in);
	CHECK(string("A") == out);
	in = string("Aa");
	out = capitalize(in);
	CHECK(string("Aa") == out);
	in = string("aA");
	out = capitalize(in);
	CHECK(string("AA") == out);
	in = string("AA");
	out = capitalize(in);
	CHECK(string("AA") == out);

	in = string("ελλάδα");
	out = capitalize(in);
	// FIXME CHECK(string("Ελλάδα") == out);
	in = string("Ελλάδα");
	out = capitalize(in);
	CHECK(string("Ελλάδα") == out);
	in = string("ΕΛΛΆΔΑ");
	out = capitalize(in);
	CHECK(string("ΕΛΛΆΔΑ") == out);

	in = string("σίγμα");
	out = capitalize(in);
	// FIXME CHECK(string("Σίγμα") == out);
	in = string("ςίγμα"); // Use of ς where σ is expected.
	out = capitalize(in);
	// FIXME CHECK(string("Σίγμα") == out);
	in = string("Σίγμα");
	out = capitalize(in);
	CHECK(string("Σίγμα") == out);

	in = string("ijsselmeer");
	out = capitalize(in);
	CHECK(string("Ijsselmeer") == out);
	in = string("Ijsselmeer");
	out = capitalize(in);
	CHECK(string("Ijsselmeer") == out);
	in = string("iJsselmeer");
	out = capitalize(in);
	CHECK(string("IJsselmeer") == out);
	in = string("IJsselmeer");
	out = capitalize(in);
	CHECK(string("IJsselmeer") == out);

	in = string("ijsselmeer");
	out = capitalize(in, true);
	CHECK(string("IJsselmeer") == out);
	in = string("Ijsselmeer");
	out = capitalize(in, true);
	CHECK(string("IJsselmeer") == out);
	in = string("iJsselmeer");
	out = capitalize(in, true);
	CHECK(string("IJsselmeer") == out);
	in = string("IJsselmeer");
	out = capitalize(in, true);
	CHECK(string("IJsselmeer") == out);

	in = string("ĳsselmeer");
	out = capitalize(in);
	// FIXME CHECK(string("Ĳsselmeer") == out);
	in = string("Ĳsselmeer");
	out = capitalize(in);
	CHECK(string("Ĳsselmeer") == out);

	in = string("ĳsselmeer");
	out = capitalize(in, true);
	// FIXME CHECK(string("Ĳsselmeer") == out);
	in = string("Ĳsselmeer");
	out = capitalize(in, true);
	CHECK(string("Ĳsselmeer") == out);

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
