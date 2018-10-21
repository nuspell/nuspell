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

#include <catch2/catch.hpp>

#include <iostream>

#include "../src/nuspell/dictionary.hxx"

using namespace std;
using namespace std::literals::string_literals;
using namespace nuspell;

struct Dict_Test : public nuspell::Dict_Base {
	using Dict_Base::spell_priv;
	template <class CharT>
	auto spell_priv(std::basic_string<CharT>&& s)
	{
		return Dict_Base::spell_priv(s);
	}
};

TEST_CASE("parse", "[dictionary]")
{
	CHECK_THROWS_AS(Dictionary::load_from_path(""), std::ios_base::failure);
}
TEST_CASE("simple", "[dictionary]")
{
	auto d = Dict_Test();
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
	auto d = Dict_Test();
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

TEST_CASE("complex_prefixes", "[dictionary]")
{
	auto d = Dict_Test();
	d.set_encoding_and_language("UTF-8");

	d.words.emplace("drink", u"X");
	d.wide_structures.suffixes.emplace(u'Y', true, L"", L"s", Flag_Set(),
	                                   L".");
	d.wide_structures.suffixes.emplace(u'X', true, L"", L"able",
	                                   Flag_Set(u"Y"), L".");

	auto good = {L"drink", L"drinkable", L"drinkables"};
	for (auto& g : good)
		CHECK(d.spell_priv<wchar_t>(g) == true);

	auto wrong = {L"drinks"};
	for (auto& w : wrong)
		CHECK(d.spell_priv<wchar_t>(w) == false);
}

TEST_CASE("extra_stripping", "[dictionary]")
{
	auto d = Dict_Test();
	d.set_encoding_and_language("UTF-8");
	d.complex_prefixes = true;

	d.words.emplace("aa", u"ABC");
	d.words.emplace("bb", u"XYZ");

	d.wide_structures.prefixes.emplace(u'A', true, L"", L"W",
	                                   Flag_Set(u"B"), L"aa");
	d.wide_structures.prefixes.emplace(u'B', true, L"", L"Q",
	                                   Flag_Set(u"C"), L"Wa");
	d.wide_structures.suffixes.emplace(u'C', true, L"", L"E", Flag_Set(),
	                                   L"a");
	d.wide_structures.prefixes.emplace(u'X', true, L"b", L"1",
	                                   Flag_Set(u"Y"), L"b");
	d.wide_structures.suffixes.emplace(u'Y', true, L"", L"2",
	                                   Flag_Set(u"Z"), L"b");
	d.wide_structures.prefixes.emplace(u'Z', true, L"", L"3", Flag_Set(),
	                                   L"1");
	// complex strip suffix prefix prefix
	CHECK(d.spell_priv<wchar_t>(L"QWaaE") == true);
	// complex strip prefix suffix prefix
	CHECK(d.spell_priv<wchar_t>(L"31b2") == true);
}

TEST_CASE("break_pattern", "[dictionary]")
{
	auto d = Dict_Test();
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
	auto d = Dict_Test();
	d.set_encoding_and_language("UTF-8");
	d.words.emplace("Sant'Elia", u"");
	d.words.emplace("d'Osormort", u"");

	auto good = {L"SANT'ELIA", L"D'OSORMORT"};
	for (auto& g : good)
		CHECK(d.spell_priv<wchar_t>(g) == true);
}

TEST_CASE("compounding", "[dictionary]")
{
	auto d = Dict_Test();
	d.set_encoding_and_language("UTF-8");
	d.compound_begin_flag = *u"B";
	d.compound_last_flag = *u"L";

	SECTION("begin_last")
	{
		d.compound_min_length = 4;
		d.words.emplace("car", u"B");
		d.words.emplace("cook", u"B");
		d.words.emplace("photo", u"B");
		d.words.emplace("book", u"L");

		auto good = {L"cookbook", L"photobook"};
		for (auto& g : good)
			CHECK(d.spell_priv<wchar_t>(g) == true);
		auto wrong = {L"carbook", L"bookcook", L"bookphoto",
		              L"cookphoto", L"photocook"};
		for (auto& w : wrong)
			CHECK(d.spell_priv<wchar_t>(w) == false);
	}

	SECTION("compound_middle")
	{
		d.compound_flag = *u"C";
		d.compound_middle_flag = *u"M";
		d.compound_check_duplicate = true;
		d.words.emplace("goederen", u"C");
		d.words.emplace("trein", u"M");
		d.words.emplace("wagon", u"C");

		auto good = {L"goederentreinwagon", L"wagontreingoederen",
		             L"goederenwagon", L"wagongoederen"};
		for (auto& g : good)
			CHECK(d.spell_priv<wchar_t>(g) == true);
		auto wrong = {L"goederentrein", L"treingoederen",
		              L"treinwagon",    L"wagontrein",
		              L"treintrein",    L"goederengoederen",
		              L"wagonwagon"};
		for (auto& w : wrong)
			CHECK(d.spell_priv<wchar_t>(w) == false);
	}

	SECTION("triple")
	{
		d.compound_check_triple = true;
		d.compound_simplified_triple = true;
		d.words.emplace("schiff", u"B");
		d.words.emplace("fahrt", u"L");

		auto good = {L"Schiffahrt", L"schiffahrt"};
		for (auto& g : good)
			CHECK(d.spell_priv<wchar_t>(g) == true);
		auto wrong = {L"Schifffahrt", L"schifffahrt", L"SchiffFahrt",
		              L"SchifFahrt",  L"schiffFahrt", L"schifFahrt"};
		for (auto& w : wrong)
			CHECK(d.spell_priv<wchar_t>(w) == false);
	}
}

TEST_CASE("rep_suggest", "[dictionary]")
{
	auto d = Dict_Test();
	d.set_encoding_and_language("UTF-8");
	d.wide_structures.replacements = {{L"ph", L"f"},
	                                  {L"shun$", L"tion"},
	                                  {L"^voo", L"foo"},
	                                  {L"^alot$", L"a lot"}};
	auto good = L"fat";
	d.words.emplace("fat", u"");
	CHECK(d.spell_priv<wchar_t>(good) == true);
	auto w = wstring(L"phat");
	CHECK(d.spell_priv<wchar_t>(w) == false);
	auto out_sug = List_WStrings();
	auto expected_sug = List_WStrings{good};
	d.rep_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);
	w = wstring(L"fad phat");
	out_sug.clear();
	expected_sug.clear();
	d.rep_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	good = L"station";
	d.words.emplace("station", u"");
	CHECK(d.spell_priv<wchar_t>(good) == true);
	w = wstring(L"stashun");
	CHECK(d.spell_priv<wchar_t>(w) == false);
	out_sug.clear();
	expected_sug = {good};
	d.rep_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	d.words.emplace("stations", u"");
	w = wstring(L"stashuns");
	CHECK(d.spell_priv<wchar_t>(w) == false);
	out_sug.clear();
	expected_sug.clear();
	d.rep_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	good = L"food";
	d.words.emplace("food", u"");
	CHECK(d.spell_priv<wchar_t>(good) == true);
	w = wstring(L"vood");
	CHECK(d.spell_priv<wchar_t>(w) == false);
	out_sug.clear();
	expected_sug = {good};
	d.rep_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	w = wstring(L"vvood");
	CHECK(d.spell_priv<wchar_t>(w) == false);
	out_sug.clear();
	expected_sug.clear();
	d.rep_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	good = L"a lot";
	d.words.emplace("a lot", u"");
	CHECK(d.spell_priv<wchar_t>(good) == true);
	w = wstring(L"alot");
	CHECK(d.spell_priv<wchar_t>(w) == false);
	out_sug.clear();
	expected_sug = {good};
	d.rep_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	w = wstring(L"aalot");
	CHECK(d.spell_priv<wchar_t>(w) == false);
	out_sug.clear();
	expected_sug.clear();
	d.rep_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	w = wstring(L"alott");
	CHECK(d.spell_priv<wchar_t>(w) == false);
	out_sug.clear();
	expected_sug.clear();
	d.rep_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);
}

TEST_CASE("extra_char_suggest", "[dictionary]")
{
	auto d = Dict_Test();
	d.set_encoding_and_language("UTF-8");

	auto good = L"table";
	d.wide_structures.try_chars = good;
	d.words.emplace("table", u"");
	CHECK(d.spell_priv<wchar_t>(good) == true);

	auto w = wstring(L"tabble");
	CHECK(d.spell_priv<wchar_t>(w) == false);

	auto out_sug = List_WStrings();
	auto expected_sug = List_WStrings{good};
	d.extra_char_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	d.forbid_warn = true;
	d.warn_flag = *u"W";
	d.words.emplace("late", u"W");
	w = wstring(L"laate");
	out_sug.clear();
	expected_sug.clear();
	d.extra_char_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);
}

TEST_CASE("map_suggest", "[dictionary]")
{
	auto d = Dict_Test();
	d.set_encoding_and_language("UTF-8");

	auto good = L"naïve";
	d.words.emplace("naïve", u"");
	d.wide_structures.similarities = {Similarity_Group<wchar_t>(L"iíìîï")};
	CHECK(d.spell_priv<wchar_t>(good) == true);

	auto w = wstring(L"naive");
	CHECK(d.spell_priv<wchar_t>(w) == false);

	auto out_sug = List_WStrings();
	auto expected_sug = List_WStrings{good};
	d.map_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	d.words.emplace("æon", u"");
	d.wide_structures.similarities.push_back(
	    Similarity_Group<wchar_t>(L"æ(ae)"));
	good = L"æon";
	CHECK(d.spell_priv<wchar_t>(good) == true);
	w = wstring(L"aeon");
	CHECK(d.spell_priv<wchar_t>(w) == false);
	out_sug.clear();
	expected_sug = {good};
	d.map_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	d.words.emplace("zijn", u"");
	d.wide_structures.similarities.push_back(
	    Similarity_Group<wchar_t>(L"(ij)ĳ"));
	good = L"zijn";
	CHECK(d.spell_priv<wchar_t>(good) == true);
	w = wstring(L"zĳn");
	CHECK(d.spell_priv<wchar_t>(w) == false);
	out_sug.clear();
	expected_sug = {good};
	d.map_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	d.words.emplace("hear", u"");
	d.wide_structures.similarities.push_back(
	    Similarity_Group<wchar_t>(L"(ae)(ea)"));
	good = L"hear";
	CHECK(d.spell_priv<wchar_t>(good) == true);
	w = wstring(L"haer");
	CHECK(d.spell_priv<wchar_t>(w) == false);
	out_sug.clear();
	expected_sug = {good};
	d.map_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);
}

TEST_CASE("keyboard_suggest", "[dictionary]")
{
	auto d = Dict_Test();
	d.set_encoding_and_language("UTF-8");

	auto good1 = L"abcd";
	auto good2 = L"Abb";
	d.words.emplace("abcd", u"");
	d.words.emplace("Abb", u"");
	d.wide_structures.keyboard_closeness = L"uiop|xdf|nm";
	CHECK(d.spell_priv<wchar_t>(good1) == true);

	auto w = wstring(L"abcf");
	CHECK(d.spell_priv<wchar_t>(w) == false);

	auto out_sug = List_WStrings();
	auto expected_sug = List_WStrings{good1};
	d.keyboard_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	w = wstring(L"abcx");
	CHECK(d.spell_priv<wchar_t>(w) == false);
	out_sug.clear();
	expected_sug = {good1};
	d.keyboard_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	w = wstring(L"abcg");
	CHECK(d.spell_priv<wchar_t>(w) == false);
	out_sug.clear();
	expected_sug.clear();
	d.keyboard_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	w = wstring(L"abb");
	CHECK(d.spell_priv<wchar_t>(w) == false);
	out_sug.clear();
	expected_sug.clear();
	expected_sug = {good2};
	d.keyboard_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);
}

TEST_CASE("bad_char_suggest", "[dictionary]")
{
	auto d = Dict_Test();
	d.set_encoding_and_language("UTF-8");

	auto good = L"chair";
	d.words.emplace("chair", u"");
	d.wide_structures.try_chars = good;
	CHECK(d.spell_priv<wchar_t>(good) == true);

	auto w = wstring(L"cháir");
	CHECK(d.spell_priv<wchar_t>(w) == false);

	auto out_sug = List_WStrings();
	auto expected_sug = List_WStrings{good};
	d.bad_char_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);
}

TEST_CASE("forgotten_char_suggest", "[dictionary]")
{
	auto d = Dict_Test();
	d.set_encoding_and_language("UTF-8");

	auto good = L"abcd";
	d.words.emplace("abcd", u"");
	d.wide_structures.try_chars = good;
	CHECK(d.spell_priv<wchar_t>(good) == true);

	auto w = wstring(L"abd");
	CHECK(d.spell_priv<wchar_t>(w) == false);

	auto out_sug = List_WStrings();
	auto expected_sug = List_WStrings{good};
	d.forgotten_char_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);
}

#if 0
TEST_CASE("phonetic_suggest", "[dictionary]")
{
	auto d = Dict_Test();
	d.set_encoding_and_language("UTF-8");

	d.words.emplace("Brasilia", u"");
	d.words.emplace("brassily", u"");
	d.words.emplace("Brazilian", u"");
	d.words.emplace("brilliance", u"");
	d.words.emplace("brilliancy", u"");
	d.words.emplace("brilliant", u"");
	d.words.emplace("brain", u"");
	d.words.emplace("brass", u"");
	d.words.emplace("Churchillian", u"");
	d.words.emplace("xxxxxxxxxx", u""); // needs adding of ph:Brasilia to
	// its morph data, but this is pending enabling of
	// parse_morhological_fields when reading aff file.

	d.wide_structures.phonetic_table = {{L"AH(AEIOUY)-^", L"*H"},
	                                    {L"AR(AEIOUY)-^", L"*R"},
	                                    {L"A(HR)^", L"*"},
	                                    {L"A^", L"*"},
	                                    {L"AH(AEIOUY)-", L"H"},
	                                    {L"AR(AEIOUY)-", L"R"},
	                                    {L"A(HR)", L"_"},
	                                    {L"BB-", L"_"},
	                                    {L"B", L"B"},
	                                    {L"CQ-", L"_"},
	                                    {L"CIA", L"X"},
	                                    {L"CH", L"X"},
	                                    {L"C(EIY)-", L"S"},
	                                    {L"CK", L"K"},
	                                    {L"COUGH^", L"KF"},
	                                    {L"CC<", L"C"},
	                                    {L"C", L"K"},
	                                    {L"DG(EIY)", L"K"},
	                                    {L"DD-", L"_"},
	                                    {L"D", L"T"},
	                                    {L"É<", L"E"},
	                                    {L"EH(AEIOUY)-^", L"*H"},
	                                    {L"ER(AEIOUY)-^", L"*R"},
	                                    {L"E(HR)^", L"*"},
	                                    {L"ENOUGH^$", L"*NF"},
	                                    {L"E^", L"*"},
	                                    {L"EH(AEIOUY)-", L"H"},
	                                    {L"ER(AEIOUY)-", L"R"},
	                                    {L"E(HR)", L"_"},
	                                    {L"FF-", L"_"},
	                                    {L"F", L"F"},
	                                    {L"GN^", L"N"},
	                                    {L"GN$", L"N"},
	                                    {L"GNS$", L"NS"},
	                                    {L"GNED$", L"N"},
	                                    {L"GH(AEIOUY)-", L"K"},
	                                    {L"GH", L"_"},
	                                    {L"GG9", L"K"},
	                                    {L"G", L"K"},
	                                    {L"H", L"H"},
	                                    {L"IH(AEIOUY)-^", L"*H"},
	                                    {L"IR(AEIOUY)-^", L"*R"},
	                                    {L"I(HR)^", L"*"},
	                                    {L"I^", L"*"},
	                                    {L"ING6", L"N"},
	                                    {L"IH(AEIOUY)-", L"H"},
	                                    {L"IR(AEIOUY)-", L"R"},
	                                    {L"I(HR)", L"_"},
	                                    {L"J", L"K"},
	                                    {L"KN^", L"N"},
	                                    {L"KK-", L"_"},
	                                    {L"K", L"K"},
	                                    {L"LAUGH^", L"LF"},
	                                    {L"LL-", L"_"},
	                                    {L"L", L"L"},
	                                    {L"MB$", L"M"},
	                                    {L"MM", L"M"},
	                                    {L"M", L"M"},
	                                    {L"NN-", L"_"},
	                                    {L"N", L"N"},
	                                    {L"OH(AEIOUY)-^", L"*H"},
	                                    {L"OR(AEIOUY)-^", L"*R"},
	                                    {L"O(HR)^", L"*"},
	                                    {L"O^", L"*"},
	                                    {L"OH(AEIOUY)-", L"H"},
	                                    {L"OR(AEIOUY)-", L"R"},
	                                    {L"O(HR)", L"_"},
	                                    {L"PH", L"F"},
	                                    {L"PN^", L"N"},
	                                    {L"PP-", L"_"},
	                                    {L"P", L"P"},
	                                    {L"Q", L"K"},
	                                    {L"RH^", L"R"},
	                                    {L"ROUGH^", L"RF"},
	                                    {L"RR-", L"_"},
	                                    {L"R", L"R"},
	                                    {L"SCH(EOU)-", L"SK"},
	                                    {L"SC(IEY)-", L"S"},
	                                    {L"SH", L"X"},
	                                    {L"SI(AO)-", L"X"},
	                                    {L"SS-", L"_"},
	                                    {L"S", L"S"},
	                                    {L"TI(AO)-", L"X"},
	                                    {L"TH", L"@"},
	                                    {L"TCH--", L"_"},
	                                    {L"TOUGH^", L"TF"},
	                                    {L"TT-", L"_"},
	                                    {L"T", L"T"},
	                                    {L"UH(AEIOUY)-^", L"*H"},
	                                    {L"UR(AEIOUY)-^", L"*R"},
	                                    {L"U(HR)^", L"*"},
	                                    {L"U^", L"*"},
	                                    {L"UH(AEIOUY)-", L"H"},
	                                    {L"UR(AEIOUY)-", L"R"},
	                                    {L"U(HR)", L"_"},
	                                    {L"V^", L"W"},
	                                    {L"V", L"F"},
	                                    {L"WR^", L"R"},
	                                    {L"WH^", L"W"},
	                                    {L"W(AEIOU)-", L"W"},
	                                    {L"X^", L"S"},
	                                    {L"X", L"KS"},
	                                    {L"Y(AEIOU)-", L"Y"},
	                                    {L"ZZ-", L"_"},
	                                    {L"Z", L"S"}};

	auto w = wstring(L"Brasillian");
	CHECK(d.spell_priv<wchar_t>(w) == false);

	auto out_sug = List_WStrings();
	auto expected_sug = List_WStrings();
	auto sugs = {L"Brasilia",  L"Xxxxxxxxxx", L"Brilliant",
	             L"Brazilian", L"Brassily",   L"Brilliance"};
	for (auto& s : sugs)
		expected_sug.push_back(s);

	d.phonetic_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);
}
#endif

#if 0
TEST_CASE("long word", "[dictionary]")
{
	auto d = Dict_Test();
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

	auto out_sug = List_WStrings();
	auto expected_sug = List_WStrings();
	d.suggest(toolong, out_sug);
	CHECK(out_sug == expected_sug);
}
#endif

TEST_CASE("suggest_priv", "[dictionary]")
{
	auto d = Dict_Test();
	d.set_encoding_and_language("UTF-8");
	d.wide_structures.try_chars = L"ailrt";

	// extra char, bad char, bad char, forgotten char
	auto words = {"tral", "trial", "trail", "traalt"};
	for (auto& x : words)
		d.words.insert({x, {}});

	auto w = wstring(L"traal");
	auto out_sug = List_WStrings();
	d.suggest_priv(w, out_sug);
	CHECK(words.size() == out_sug.size());
}

#if 0
TEST_CASE("suggest_priv_max", "[dictionary]")
{
	auto d = Dict_Test();
	d.set_encoding_and_language("UTF-8");
	d.wide_structures.replacements = {
	    {L"x", L"a"}, {L"x", L"b"}, {L"x", L"c"}, {L"x", L"d"},
	    {L"x", L"e"}, {L"x", L"f"}, {L"x", L"g"}, {L"x", L"h"}};
	d.wide_structures.similarities = {
	    Similarity_Group<wchar_t>(L"xabcdefgh")};
	d.wide_structures.keyboard_closeness = L"axb|cxd|exf|gxh";
	d.wide_structures.try_chars = L"abcdefgh";

	auto chars = string("abcdefgh");
	auto word = string();
	for (auto& w : chars) {
		word.push_back(w);
		for (auto& x : chars) {
			word.push_back(x);
			for (auto& y : chars) {
				word.push_back(y);
				for (auto& z : chars) {
					word.push_back(z);
					d.words.insert({word, {}});
					word.pop_back();
				}
				word.pop_back();
			}
			word.pop_back();
		}
		word.pop_back();
	}

	auto w = wstring(L"xxxx");
	auto out_sug = List_WStrings();
	d.suggest_priv(w, out_sug);
	CHECK(d.words.size() == out_sug.size());
}
#endif
