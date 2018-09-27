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

#include <boost/locale.hpp>

#include "../src/nuspell/dictionary.hxx"
#include "../src/nuspell/structures.hxx"

using namespace std;
using namespace std::literals::string_literals;
using namespace nuspell;

TEST_CASE("simple", "[dictionary]")
{
	boost::locale::generator gen;
	auto d = Dict_Base();
	d.set_encoding_and_language("UTF-8");

	auto words = {"table", "chair", "book", "fóóáár", "áárfóóĳ"};
	for (auto& x : words)
		d.words.insert({x, {}});

	auto good = {L"table", L"chair", L"book", L"fóóáár", L"áárfóóĳ"};
	for (auto& g : good)
		CHECK(d.spell_priv<wchar_t>(g) == true);

	auto wrong = {L"tabel", L"chiar",    L"boko", L"xyyz",  L"fooxy",
	              L"xyfoo", L"fooxybar", L"ééőő", L"fóóéé", L"őőáár"};
	for (auto& w : wrong)
		CHECK(d.spell_priv<wchar_t>(w) == false);
}

TEST_CASE("suffixes", "[dictionary]")
{
	boost::locale::generator gen;
	auto d = Dict_Base();
	d.set_encoding_and_language("UTF-8");

	d.words.emplace("berry", u"T");
	d.words.emplace("May", u"T");
	d.words.emplace("vary", u"");

	d.wide_structures.suffixes.emplace(u'T', true, L"y"s, L"ies"s,
	                                   Flag_Set(), L".[^aeiou]y"s);

	auto good = {L"berry", L"Berry", L"berries", L"BERRIES",
	             L"May",   L"MAY",   L"vary"};
	for (auto& g : good)
		CHECK(d.spell_priv<wchar_t>(g) == true);

	auto wrong = {L"beRRies", L"Maies", L"MAIES", L"maies", L"varies"};
	for (auto& w : wrong)
		CHECK(d.spell_priv<wchar_t>(w) == false);
}

TEST_CASE("break_pattern", "[dictionary]")
{
	boost::locale::generator gen;
	auto d = Dict_Base();
	d.set_encoding_and_language("UTF-8");

	d.words.emplace("user", u"");
	d.words.emplace("interface", u"");

	d.wide_structures.break_table = {L"-"};

	auto good = {L"user",           L"interface", L"user-interface",
	             L"interface-user", L"user-user", L"interface-interface"};
	for (auto& g : good)
		CHECK(d.spell_priv<wchar_t>(g) == true);

	auto wrong = {L"user--interface", L"user interface",
	              L"user - interface"};
	for (auto& w : wrong)
		CHECK(d.spell_priv<wchar_t>(w) == false);
}

TEST_CASE("rep_suggest", "[dictionary]") {}

TEST_CASE("extra_char_suggest", "[dictionary]")
{
	boost::locale::generator gen;
	auto d = Dict_Base();
	d.set_encoding_and_language("UTF-8");

	auto good = L"abcd";
	d.wide_structures.try_chars = {good};
	d.words.emplace("abcd", u"");
	CHECK(d.spell_priv<wchar_t>(good) == true);

	auto w = wstring(L"abxcd");
	CHECK(d.spell_priv<wchar_t>(w) == false);

	auto out = List_Strings<wchar_t>();
	auto sug = List_Strings<wchar_t>();
	sug.push_back(good);
	d.extra_char_suggest(w, back_inserter(out));
	CHECK(out == sug);
}

TEST_CASE("map_suggest", "[dictionary]") {}

TEST_CASE("keyboard_suggest", "[dictionary]")
{
	boost::locale::generator gen;
	auto d = Dict_Base();
	d.set_encoding_and_language("UTF-8");

	auto good = L"abcd";
	d.words.emplace("abcd", u"");
	d.wide_structures.keyboard_closeness = {L"uiop|df|nm"};
	CHECK(d.spell_priv<wchar_t>(good) == true);

	auto w = wstring(L"abcf");
	CHECK(d.spell_priv<wchar_t>(w) == false);

	auto out = List_Strings<wchar_t>();
	auto sug = List_Strings<wchar_t>();
	sug.push_back(good);
	d.keyboard_suggest(w, back_inserter(out));
	CHECK(out == sug);
}

TEST_CASE("bad_char_suggest", "[dictionary]")
{
	boost::locale::generator gen;
	auto d = Dict_Base();
	d.set_encoding_and_language("UTF-8");

	auto good = L"abcd";
	d.words.emplace("abcd", u"");
	d.wide_structures.try_chars = {good};
	CHECK(d.spell_priv<wchar_t>(good) == true);

	auto w = wstring(L"abce");
	CHECK(d.spell_priv<wchar_t>(w) == false);

	auto out = List_Strings<wchar_t>();
	auto sug = List_Strings<wchar_t>();
	sug.push_back(good);
	d.bad_char_suggest(w, back_inserter(out));
	CHECK(out == sug);
}

TEST_CASE("forgotten_char_suggest", "[dictionary]")
{
	boost::locale::generator gen;
	auto d = Dict_Base();
	d.set_encoding_and_language("UTF-8");

	auto good = L"abcd";
	d.words.emplace("abcd", u"");
	d.wide_structures.try_chars = {good};
	CHECK(d.spell_priv<wchar_t>(good) == true);

	auto w = wstring(L"abd");
	CHECK(d.spell_priv<wchar_t>(w) == false);

	auto out = List_Strings<wchar_t>();
	auto sug = List_Strings<wchar_t>();
	sug.push_back(good);
	d.forgotten_char_suggest(w, back_inserter(out));
	CHECK(out == sug);
}

TEST_CASE("phonetic_suggest", "[dictionary]") {}
