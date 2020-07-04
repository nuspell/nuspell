/* Copyright 2018-2020 Sander van Geloven, Dimitrij Mijoski
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

#include <nuspell/dictionary.hxx>

#include <catch2/catch.hpp>

using namespace std;
using namespace nuspell;

struct Dict_Test : public nuspell::Dict_Base {
	using Dict_Base::spell_priv;
	auto spell_priv(std::wstring&& s) { return Dict_Base::spell_priv(s); }
};

TEST_CASE("Dictionary::load_from_path", "[dictionary]")
{
	CHECK_THROWS_AS(Dictionary::load_from_path(""),
	                Dictionary_Loading_Error);
}
TEST_CASE("Dictionary::spell_priv simple", "[dictionary]")
{
	auto d = Dict_Test();

	auto words = {L"table", L"chair", L"book", L"fóóáár", L"áárfóóĳ"};
	for (auto& x : words)
		d.words.insert({x, {}});

	auto good = {L"",      L".",    L"..",     L"table",
	             L"chair", L"book", L"fóóáár", L"áárfóóĳ"};
	for (auto& g : good)
		CHECK(d.spell_priv(g) == true);

	auto wrong = {L"tabel", L"chiar",    L"boko", L"xyyz",  L"fooxy",
	              L"xyfoo", L"fooxybar", L"ééőő", L"fóóéé", L"őőáár"};
	for (auto& w : wrong)
		CHECK(d.spell_priv(w) == false);
}

TEST_CASE("Dictionary::spell_priv suffixes", "[dictionary]")
{
	auto d = Dict_Test();

	d.words.emplace(L"berry", u"T");
	d.words.emplace(L"May", u"T");
	d.words.emplace(L"vary", u"");

	d.suffixes = {{u'T', true, L"y", L"ies", Flag_Set(),
	               Condition<wchar_t>(L".[^aeiou]y")}};

	auto good = {L"berry", L"Berry", L"berries", L"BERRIES",
	             L"May",   L"MAY",   L"vary"};
	for (auto& g : good)
		CHECK(d.spell_priv(g) == true);

	auto wrong = {L"beRRies", L"Maies", L"MAIES", L"maies", L"varies"};
	for (auto& w : wrong)
		CHECK(d.spell_priv(w) == false);
}

TEST_CASE("Dictionary::spell_priv complex_prefixes", "[dictionary]")
{
	auto d = Dict_Test();

	d.words.emplace(L"drink", u"X");
	d.suffixes = {
	    {u'Y', true, L"", L"s", Flag_Set(), Condition<wchar_t>(L".")},
	    {u'X', true, L"", L"able", Flag_Set(u"Y"),
	     Condition<wchar_t>(L".")}};

	auto good = {L"drink", L"drinkable", L"drinkables"};
	for (auto& g : good)
		CHECK(d.spell_priv(g) == true);

	auto wrong = {L"drinks"};
	for (auto& w : wrong)
		CHECK(d.spell_priv(w) == false);
}

TEST_CASE("Dictionary::spell_priv extra_stripping", "[dictionary]")
{
	auto d = Dict_Test();

	d.complex_prefixes = true;

	d.words.emplace(L"aa", u"ABC");
	d.words.emplace(L"bb", u"XYZ");

	d.prefixes = {
	    {u'A', true, L"", L"W", Flag_Set(u"B"), Condition<wchar_t>(L"aa")},
	    {u'B', true, L"", L"Q", Flag_Set(u"C"), Condition<wchar_t>(L"Wa")},
	    {u'X', true, L"b", L"1", Flag_Set(u"Y"), Condition<wchar_t>(L"b")},
	    {u'Z', true, L"", L"3", Flag_Set(), Condition<wchar_t>(L"1")}};
	d.suffixes = {
	    {u'C', true, L"", L"E", Flag_Set(), Condition<wchar_t>(L"a")},
	    {u'Y', true, L"", L"2", Flag_Set(u"Z"), Condition<wchar_t>(L"b")}};
	// complex strip suffix prefix prefix
	CHECK(d.spell_priv(L"QWaaE") == true);
	// complex strip prefix suffix prefix
	CHECK(d.spell_priv(L"31b2") == true);
}

TEST_CASE("Dictionary::spell_priv break_pattern", "[dictionary]")
{
	auto d = Dict_Test();

	d.forbid_warn = true;
	d.warn_flag = 'W';

	d.words.emplace(L"user", u"");
	d.words.emplace(L"interface", u"");
	d.words.emplace(L"interface-interface", u"W");

	d.break_table = {L"-", L"++++++$"};

	auto good = {L"user", L"interface", L"user-interface",
	             L"interface-user", L"user-user"};
	for (auto& g : good)
		CHECK(d.spell_priv(g) == true);

	auto wrong = {L"user--interface", L"user interface",
	              L"user - interface", L"interface-interface"};
	for (auto& w : wrong)
		CHECK(d.spell_priv(w) == false);
}

TEST_CASE("Dictionary::spell_priv spell_casing_upper", "[dictionary]")
{
	auto d = Dict_Test();

	d.words.emplace(L"Sant'Elia", u"");
	d.words.emplace(L"d'Osormort", u"");

	auto good = {L"SANT'ELIA", L"D'OSORMORT"};
	for (auto& g : good)
		CHECK(d.spell_priv(g) == true);
}

TEST_CASE("Dictionary::spell_priv compounding begin_last", "[dictionary]")
{
	auto d = Dict_Test();

	d.compound_begin_flag = 'B';
	d.compound_last_flag = 'L';

	d.compound_min_length = 4;
	d.words.emplace(L"car", u"B");
	d.words.emplace(L"cook", u"B");
	d.words.emplace(L"photo", u"B");
	d.words.emplace(L"book", u"L");

	auto good = {L"cookbook", L"photobook"};
	for (auto& g : good)
		CHECK(d.spell_priv(g) == true);
	auto wrong = {L"carbook", L"bookcook", L"bookphoto", L"cookphoto",
	              L"photocook"};
	for (auto& w : wrong)
		CHECK(d.spell_priv(w) == false);
}

TEST_CASE("Dictionary::spell_priv compounding compound_middle", "[dictionary]")
{
	auto d = Dict_Test();
	d.compound_flag = 'C';
	d.compound_middle_flag = 'M';
	d.compound_check_duplicate = true;
	d.words.emplace(L"goederen", u"C");
	d.words.emplace(L"trein", u"M");
	d.words.emplace(L"wagon", u"C");

	auto good = {L"goederentreinwagon", L"wagontreingoederen",
	             L"goederenwagon", L"wagongoederen"};
	for (auto& g : good)
		CHECK(d.spell_priv(g) == true);
	auto wrong = {L"goederentrein", L"treingoederen", L"treinwagon",
	              L"wagontrein",    L"treintrein",    L"goederengoederen",
	              L"wagonwagon"};
	for (auto& w : wrong)
		CHECK(d.spell_priv(w) == false);
}

TEST_CASE("Dictionary::spell_priv compounding triple", "[dictionary]")
{
	auto d = Dict_Test();
	d.compound_begin_flag = 'B';
	d.compound_last_flag = 'L';
	d.compound_check_triple = true;
	d.compound_simplified_triple = true;
	d.words.emplace(L"schiff", u"B");
	d.words.emplace(L"fahrt", u"L");

	auto good = {L"Schiffahrt", L"schiffahrt"};
	for (auto& g : good)
		CHECK(d.spell_priv(g) == true);
	auto wrong = {L"Schifffahrt", L"schifffahrt", L"SchiffFahrt",
	              L"SchifFahrt",  L"schiffFahrt", L"schifFahrt"};
	for (auto& w : wrong)
		CHECK(d.spell_priv(w) == false);
}

TEST_CASE("Dictionary suggestions rep_suggest", "[dictionary]")
{
	auto d = Dict_Test();

	d.replacements = {{L"ph", L"f"},
	                  {L"shun$", L"tion"},
	                  {L"^voo", L"foo"},
	                  {L"^alot$", L"a lot"}};
	auto good = L"fat";
	d.words.emplace(L"fat", u"");
	CHECK(d.spell_priv(good) == true);
	auto w = wstring(L"phat");
	CHECK(d.spell_priv(w) == false);
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
	d.words.emplace(L"station", u"");
	CHECK(d.spell_priv(good) == true);
	w = wstring(L"stashun");
	CHECK(d.spell_priv(w) == false);
	out_sug.clear();
	expected_sug = {good};
	d.rep_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	d.words.emplace(L"stations", u"");
	w = wstring(L"stashuns");
	CHECK(d.spell_priv(w) == false);
	out_sug.clear();
	expected_sug.clear();
	d.rep_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	good = L"food";
	d.words.emplace(L"food", u"");
	CHECK(d.spell_priv(good) == true);
	w = wstring(L"vood");
	CHECK(d.spell_priv(w) == false);
	out_sug.clear();
	expected_sug = {good};
	d.rep_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	w = wstring(L"vvood");
	CHECK(d.spell_priv(w) == false);
	out_sug.clear();
	expected_sug.clear();
	d.rep_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	good = L"a lot";
	d.words.emplace(L"a lot", u"");
	CHECK(d.spell_priv(good) == true);
	w = wstring(L"alot");
	CHECK(d.spell_priv(w) == false);
	out_sug.clear();
	expected_sug = {good};
	d.rep_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	w = wstring(L"aalot");
	CHECK(d.spell_priv(w) == false);
	out_sug.clear();
	expected_sug.clear();
	d.rep_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	w = wstring(L"alott");
	CHECK(d.spell_priv(w) == false);
	out_sug.clear();
	expected_sug.clear();
	d.rep_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);
}

TEST_CASE("Dictionary suggestions extra_char_suggest", "[dictionary]")
{
	auto d = Dict_Test();

	auto good = L"table";
	d.try_chars = good;
	d.words.emplace(L"table", u"");
	CHECK(d.spell_priv(good) == true);

	auto w = wstring(L"tabble");
	CHECK(d.spell_priv(w) == false);

	auto out_sug = List_WStrings();
	auto expected_sug = List_WStrings{good, good};
	d.extra_char_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	d.forbid_warn = true;
	d.warn_flag = *u"W";
	d.words.emplace(L"late", u"W");
	w = wstring(L"laate");
	out_sug.clear();
	expected_sug.clear();
	d.extra_char_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);
}

TEST_CASE("Dictionary suggestions map_suggest", "[dictionary]")
{
	auto d = Dict_Test();

	auto good = L"naïve";
	d.words.emplace(L"naïve", u"");
	d.similarities = {Similarity_Group<wchar_t>(L"iíìîï")};
	CHECK(d.spell_priv(good) == true);

	auto w = wstring(L"naive");
	CHECK(d.spell_priv(w) == false);

	auto out_sug = List_WStrings();
	auto expected_sug = List_WStrings{good};
	d.map_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	d.words.emplace(L"æon", u"");
	d.similarities.push_back(Similarity_Group<wchar_t>(L"æ(ae)"));
	good = L"æon";
	CHECK(d.spell_priv(good) == true);
	w = wstring(L"aeon");
	CHECK(d.spell_priv(w) == false);
	out_sug.clear();
	expected_sug = {good};
	d.map_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	d.words.emplace(L"zijn", u"");
	d.similarities.push_back(Similarity_Group<wchar_t>(L"(ij)ĳ"));
	good = L"zijn";
	CHECK(d.spell_priv(good) == true);
	w = wstring(L"zĳn");
	CHECK(d.spell_priv(w) == false);
	out_sug.clear();
	expected_sug = {good};
	d.map_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	d.words.emplace(L"hear", u"");
	d.similarities.push_back(Similarity_Group<wchar_t>(L"(ae)(ea)"));
	good = L"hear";
	CHECK(d.spell_priv(good) == true);
	w = wstring(L"haer");
	CHECK(d.spell_priv(w) == false);
	out_sug.clear();
	expected_sug = {good};
	d.map_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);
}

TEST_CASE("Dictionary suggestions keyboard_suggest", "[dictionary]")
{
	auto d = Dict_Test();

	auto good1 = L"abcd";
	auto good2 = L"Abb";
	d.words.emplace(L"abcd", u"");
	d.words.emplace(L"Abb", u"");
	d.keyboard_closeness = L"uiop|xdf|nm";
	CHECK(d.spell_priv(good1) == true);

	auto w = wstring(L"abcf");
	CHECK(d.spell_priv(w) == false);

	auto out_sug = List_WStrings();
	auto expected_sug = List_WStrings{good1};
	d.keyboard_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	w = wstring(L"abcx");
	CHECK(d.spell_priv(w) == false);
	out_sug.clear();
	expected_sug = {good1};
	d.keyboard_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	w = wstring(L"abcg");
	CHECK(d.spell_priv(w) == false);
	out_sug.clear();
	expected_sug.clear();
	d.keyboard_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	w = wstring(L"abb");
	CHECK(d.spell_priv(w) == false);
	out_sug.clear();
	expected_sug.clear();
	expected_sug = {good2};
	d.keyboard_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);
}

TEST_CASE("Dictionary suggestions bad_char_suggest", "[dictionary]")
{
	auto d = Dict_Test();

	auto good = L"chair";
	d.words.emplace(L"chair", u"");
	d.try_chars = good;
	CHECK(d.spell_priv(good) == true);

	auto w = wstring(L"cháir");
	CHECK(d.spell_priv(w) == false);

	auto out_sug = List_WStrings();
	auto expected_sug = List_WStrings{good};
	d.bad_char_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);
}

TEST_CASE("forgotten_char_suggest", "[dictionary]")
{
	auto d = Dict_Test();

	auto good = L"abcd";
	d.words.emplace(L"abcd", u"");
	d.try_chars = good;
	CHECK(d.spell_priv(good) == true);

	auto w = wstring(L"abd");
	CHECK(d.spell_priv(w) == false);

	auto out_sug = List_WStrings();
	auto expected_sug = List_WStrings{good};
	d.forgotten_char_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);
}

#if 0
TEST_CASE("Dictionary suggestions phonetic_suggest", "[dictionary]")
{
	auto d = Dict_Test();


	d.words.emplace(L"Brasilia", u"");
	d.words.emplace(L"brassily", u"");
	d.words.emplace(L"Brazilian", u"");
	d.words.emplace(L"brilliance", u"");
	d.words.emplace(L"brilliancy", u"");
	d.words.emplace(L"brilliant", u"");
	d.words.emplace(L"brain", u"");
	d.words.emplace(L"brass", u"");
	d.words.emplace(L"Churchillian", u"");
	d.words.emplace(L"xxxxxxxxxx", u""); // needs adding of ph:Brasilia to
	// its morph data, but this is pending enabling of
	// parse_morhological_fields when reading aff file.

	d.phonetic_table = {{L"AH(AEIOUY)-^", L"*H"},
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
	CHECK(d.spell_priv(w) == false);

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

TEST_CASE("Dictionary suggestions suggest_priv", "[dictionary]")
{
	auto d = Dict_Test();

	d.try_chars = L"ailrt";

	// extra char, bad char, bad char, forgotten char
	auto words = {L"tral", L"trial", L"trail", L"traalt"};
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

	d.replacements = {
	    {L"x", L"a"}, {L"x", L"b"}, {L"x", L"c"}, {L"x", L"d"},
	    {L"x", L"e"}, {L"x", L"f"}, {L"x", L"g"}, {L"x", L"h"}};
	d.similarities = {
	    Similarity_Group<wchar_t>(L"xabcdefgh")};
	d.keyboard_closeness = L"axb|cxd|exf|gxh";
	d.try_chars = L"abcdefgh";

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
