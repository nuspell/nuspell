/* Copyright 2018 Sander van Geloven
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

#include "../src/nuspell/dictionary.hxx"
#include "../src/nuspell/structures.hxx"
#include <boost/locale.hpp>

using namespace std;
using namespace std::literals::string_literals;
using namespace nuspell;

TEST_CASE("simple", "[dictionary]")
{
	boost::locale::generator gen;
	auto d = Dictionary();
	d.set_encoding_and_language("UTF-8");

	auto words = {"table", "chair", "book", "fóóáár", "áárfóóĳ"};
	for (auto& x : words)
		d.words.insert({x, {}});

	auto good = {L"table", L"chair", L"book", L"fóóáár", L"áárfóóĳ"};
	for (auto& g : good)
		CHECK(d.spell_priv<wchar_t>(g) == GOOD_WORD);

	auto wrong = {L"tabel", L"chiar",    L"boko", L"xyyz",  L"fooxy",
	              L"xyfoo", L"fooxybar", L"ééőő", L"fóóéé", L"őőáár"};
	for (auto& w : wrong)
		CHECK(d.spell_priv<wchar_t>(w) == BAD_WORD);
}

TEST_CASE("suffixes", "[dictionary]")
{
	boost::locale::generator gen;
	auto d = Dictionary();
	d.set_encoding_and_language("UTF-8");

	d.words.emplace("berry", u"T");
	d.words.emplace("May", u"T");
	d.words.emplace("vary", u"");

	d.structures.suffixes.emplace(u'T', true, "y"s, "ies"s, Flag_Set(),
	                              ".[^aeiou]y"s);

	auto good = {"berry", "Berry", "berries", "BERRIES",
	             "May",   "MAY",   "vary"};
	for (auto& g : good)
		CHECK(d.spell_priv<char>(g) == GOOD_WORD);

	auto wrong = {"beRRies", "Maies", "MAIES", "maies", "varies"};
	for (auto& w : wrong)
		CHECK(d.spell_priv<char>(w) == BAD_WORD);
}

TEST_CASE("break_pattern", "[dictionary]")
{
	boost::locale::generator gen;
	auto d = Dictionary();
	d.set_encoding_and_language("UTF-8");

	d.words.emplace("user", u"");
	d.words.emplace("interface", u"");

	d.structures.break_table = {"-"};

	auto good = {"user",           "interface", "user-interface",
	             "interface-user", "user-user", "interface-interface"};
	for (auto& g : good)
		CHECK(d.spell_priv<char>(g) == GOOD_WORD);

	auto wrong = {"user--interface", "user interface", "user - interface"};
	for (auto& w : wrong)
		CHECK(d.spell_priv<char>(w) == BAD_WORD);
}
