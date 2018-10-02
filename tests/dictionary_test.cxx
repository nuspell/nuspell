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

using namespace std;
using namespace std::literals::string_literals;
using namespace nuspell;

TEST_CASE("simple", "[dictionary]")
{
	auto d = Dict_Base();
	d.set_encoding_and_language("UTF-8");

	auto words = {"table", "chair", "book", "fóóáár", "áárfóóĳ"};
	for (auto& x : words)
		d.words.insert({x, {}});

	auto good = {L"",      L".",    L"..",     L"table",
	             L"chair", L"book", L"fóóáár", L"áárfóóĳ"};
	for (auto& g : good)
		CHECK(d.spell_priv<wchar_t>(g) == true);

	auto wrong = {L"tabel", L"chiar",    L"boko", L"xyyz",  L"fooxy",
	              L"xyfoo", L"fooxybar", L"ééőő", L"fóóéé", L"őőáár"};
	for (auto& w : wrong)
		CHECK(d.spell_priv<wchar_t>(w) == false);
}

TEST_CASE("suffixes", "[dictionary]")
{
	auto d = Dict_Base();
	d.set_encoding_and_language("UTF-8");

	d.words.emplace("berry", u"T");
	d.words.emplace("May", u"T");
	d.words.emplace("vary", u"");

	d.wide_structures.suffixes.emplace(u'T', true, L"y", L"ies", Flag_Set(),
	                                   L".[^aeiou]y");

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
	auto d = Dict_Base();
	d.set_encoding_and_language("UTF-8");
	d.forbid_warn = true;
	d.warn_flag = *u"W";

	d.words.emplace("user", u"");
	d.words.emplace("interface", u"");
	d.words.emplace("interface-interface", u"W");

	d.wide_structures.break_table = {L"-", L"++++++$"};

	auto good = {L"user", L"interface", L"user-interface",
	             L"interface-user", L"user-user"};
	for (auto& g : good)
		CHECK(d.spell_priv<wchar_t>(g) == true);

	auto wrong = {L"user--interface", L"user interface",
	              L"user - interface", L"interface-interface"};
	for (auto& w : wrong)
		CHECK(d.spell_priv<wchar_t>(w) == false);
}

TEST_CASE("spell_casing_upper", "[dictionary]")
{
	auto d = Dict_Base();
	d.set_encoding_and_language("UTF-8");
	d.words.emplace("Sant'Elia", u"");
	d.words.emplace("d'Osormort", u"");

	auto good = {L"SANT'ELIA", L"D'OSORMORT"};
	for (auto& g : good)
		CHECK(d.spell_priv<wchar_t>(g) == true);
}

#if 0
TEST_CASE("rep_suggest", "[dictionary]") {
	auto d = Dict_Base();
	d.set_encoding_and_language("UTF-8");

	auto good = L"äbcd";
	d.words.emplace("äbcd", u"");
	auto table = vector<pair<wstring, wstring>>();
	table.push_back(pair<wstring, wstring>(L"f", L"ph"));
	table.push_back(pair<wstring, wstring>(L"shun$", L"tion"));
	table.push_back(pair<wstring, wstring>(L"^alot$", L"a_lot"));
	table.push_back(pair<wstring, wstring>(L"^foo", L"bar"));
	d.wide_structures.replacements = Replacement_Table<wchar_t>(table);
	CHECK(d.spell_priv<wchar_t>(good) == true);

	auto w = wstring(L"abcd");
	CHECK(d.spell_priv<wchar_t>(w) == false);

	auto out = List_Strings<wchar_t>();
	auto sug = List_Strings<wchar_t>();
	sug.push_back(good);
	d.rep_suggest(w, out);
	CHECK(out == sug);
}
#endif

TEST_CASE("extra_char_suggest", "[dictionary]")
{
	auto d = Dict_Base();
	d.set_encoding_and_language("UTF-8");

	auto good = L"abcd";
	d.wide_structures.try_chars = good;
	d.words.emplace("abcd", u"");
	CHECK(d.spell_priv<wchar_t>(good) == true);

	auto w = wstring(L"abxcd");
	CHECK(d.spell_priv<wchar_t>(w) == false);

	auto out = List_Strings<wchar_t>();
	auto sug = List_Strings<wchar_t>();
	sug.push_back(good);
	d.extra_char_suggest(w, out);
	CHECK(out == sug);
}

TEST_CASE("map_suggest", "[dictionary]")
{
	auto d = Dict_Base();
	d.set_encoding_and_language("UTF-8");

	auto good = L"äbcd";
	d.words.emplace("äbcd", u"");
	d.wide_structures.similarities.push_back(
	    Similarity_Group<wchar_t>(L"aäâ"));
	CHECK(d.spell_priv<wchar_t>(good) == true);

	auto w = wstring(L"abcd");
	CHECK(d.spell_priv<wchar_t>(w) == false);

	auto out = List_Strings<wchar_t>();
	auto sug = List_Strings<wchar_t>();
	sug.push_back(good);
	d.map_suggest(w, out);
	CHECK(out == sug);
}

TEST_CASE("keyboard_suggest", "[dictionary]")
{
	auto d = Dict_Base();
	d.set_encoding_and_language("UTF-8");

	auto good1 = L"abcd";
	auto good2 = L"Abb";
	d.words.emplace("abcd", u"");
	d.words.emplace("Abb", u"");
	d.wide_structures.keyboard_closeness = L"uiop|xdf|nm";
	CHECK(d.spell_priv<wchar_t>(good1) == true);

	auto w = wstring(L"abcf");
	CHECK(d.spell_priv<wchar_t>(w) == false);

	auto out = List_Strings<wchar_t>();
	auto sug = List_Strings<wchar_t>();
	sug.push_back(good1);
	d.keyboard_suggest(w, out);
	CHECK(out == sug);

	w = wstring(L"abcx");
	CHECK(d.spell_priv<wchar_t>(w) == false);

	out.clear();
	sug.clear();
	sug.push_back(good1);
	d.keyboard_suggest(w, out);
	CHECK(out == sug);

	w = wstring(L"abcg");
	CHECK(d.spell_priv<wchar_t>(w) == false);

	out.clear();
	sug.clear();
	d.keyboard_suggest(w, out);
	CHECK(out == sug);

	w = wstring(L"abb");
	CHECK(d.spell_priv<wchar_t>(w) == false);

	out.clear();
	sug.clear();
	sug.push_back(good2);
	d.keyboard_suggest(w, out);
	CHECK(out == sug);
}

TEST_CASE("bad_char_suggest", "[dictionary]")
{
	auto d = Dict_Base();
	d.set_encoding_and_language("UTF-8");

	auto good = L"abcd";
	d.words.emplace("abcd", u"");
	d.wide_structures.try_chars = good;
	CHECK(d.spell_priv<wchar_t>(good) == true);

	auto w = wstring(L"abce");
	CHECK(d.spell_priv<wchar_t>(w) == false);

	auto out = List_Strings<wchar_t>();
	auto sug = List_Strings<wchar_t>();
	sug.push_back(good);
	d.bad_char_suggest(w, out);
	CHECK(out == sug);
}

TEST_CASE("forgotten_char_suggest", "[dictionary]")
{
	auto d = Dict_Base();
	d.set_encoding_and_language("UTF-8");

	auto good = L"abcd";
	d.words.emplace("abcd", u"");
	d.wide_structures.try_chars = good;
	CHECK(d.spell_priv<wchar_t>(good) == true);

	auto w = wstring(L"abd");
	CHECK(d.spell_priv<wchar_t>(w) == false);

	auto out = List_Strings<wchar_t>();
	auto sug = List_Strings<wchar_t>();
	sug.push_back(good);
	d.forgotten_char_suggest(w, out);
	CHECK(out == sug);
}

#if 0
TEST_CASE("phonetic_suggest", "[dictionary]") {}
#endif

#if 0
TEST_CASE("long word", "[dictionary]")
{
	auto d = Dict_Base();
	d.set_encoding_and_language("UTF-8");

	// 18 times abcdefghij (10 characters) = 180 characters
	auto good =
	    L"abcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcde"
	    L"fghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghij"
	    L"abcdefghijabcdefghijabcdefghijabcdefghijabcdefghij";
	// 18 times abcdefghij (10 characters) + THISISEXTRA = 191 characters
	auto toolong =
	    L"abcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcde"
	    L"fghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghij"
	    L"abcdefghijabcdefghijabcdefghijabcdefghijabcdefghijTHISISEXTRA";
	// 18 times abcdefghij (10 characters) = 180 characters
	d.words.emplace(
	    "abcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdef"
	    "ghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijab"
	    "cdefghijabcdefghijabcdefghijabcdefghijabcdefghij",
	    u"");
	CHECK(d.spell(good) == true);
	CHECK(d.spell(toolong) == true);

	auto out = List_Strings<wchar_t>();
	auto sug = List_Strings<wchar_t>();
	d.suggest(toolong, out);
	CHECK(out == sug);
}
#endif
