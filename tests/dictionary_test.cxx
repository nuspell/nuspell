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
	auto spell_priv(std::string&& s) { return Dict_Base::spell_priv(s); }
};

TEST_CASE("Dictionary::load_from_path", "[dictionary]")
{
	CHECK_THROWS_AS(Dictionary::load_from_path(""),
	                Dictionary_Loading_Error);
}
TEST_CASE("Dictionary::spell_priv simple", "[dictionary]")
{
	auto d = Dict_Test();

	auto words = {"table", "chair", "book", "fóóáár", "áárfóóĳ"};
	for (auto& x : words)
		d.words.emplace(x, u"");

	auto good = {"",      ".",    "..",     "table",
	             "chair", "book", "fóóáár", "áárfóóĳ"};
	for (auto& g : good)
		CHECK(d.spell_priv(g) == true);

	auto wrong = {"tabel", "chiar",    "boko", "xyyz",  "fooxy",
	              "xyfoo", "fooxybar", "ééőő", "fóóéé", "őőáár"};
	for (auto& w : wrong)
		CHECK(d.spell_priv(w) == false);
}

TEST_CASE("Dictionary::spell_priv suffixes", "[dictionary]")
{
	auto d = Dict_Test();

	d.words.emplace("berry", u"T");
	d.words.emplace("May", u"T");
	d.words.emplace("vary", u"");

	d.suffixes = {{u'T', true, "y", "ies", Flag_Set(),
	               Condition<char>(".[^aeiou]y")}};

	auto good = {"berry", "Berry", "berries", "BERRIES",
	             "May",   "MAY",   "vary"};
	for (auto& g : good)
		CHECK(d.spell_priv(g) == true);

	auto wrong = {"beRRies", "Maies", "MAIES", "maies", "varies"};
	for (auto& w : wrong)
		CHECK(d.spell_priv(w) == false);
}

TEST_CASE("Dictionary::spell_priv complex_prefixes", "[dictionary]")
{
	auto d = Dict_Test();

	d.words.emplace("drink", u"X");
	d.suffixes = {
	    {u'Y', true, "", "s", Flag_Set(), Condition<char>(".")},
	    {u'X', true, "", "able", Flag_Set(u"Y"), Condition<char>(".")}};

	auto good = {"drink", "drinkable", "drinkables"};
	for (auto& g : good)
		CHECK(d.spell_priv(g) == true);

	auto wrong = {"drinks"};
	for (auto& w : wrong)
		CHECK(d.spell_priv(w) == false);
}

TEST_CASE("Dictionary::spell_priv extra_stripping", "[dictionary]")
{
	auto d = Dict_Test();

	d.complex_prefixes = true;

	d.words.emplace("aa", u"ABC");
	d.words.emplace("bb", u"XYZ");

	d.prefixes = {
	    {u'A', true, "", "W", Flag_Set(u"B"), Condition<char>("aa")},
	    {u'B', true, "", "Q", Flag_Set(u"C"), Condition<char>("Wa")},
	    {u'X', true, "b", "1", Flag_Set(u"Y"), Condition<char>("b")},
	    {u'Z', true, "", "3", Flag_Set(), Condition<char>("1")}};
	d.suffixes = {
	    {u'C', true, "", "E", Flag_Set(), Condition<char>("a")},
	    {u'Y', true, "", "2", Flag_Set(u"Z"), Condition<char>("b")}};
	// complex strip suffix prefix prefix
	CHECK(d.spell_priv("QWaaE") == true);
	// complex strip prefix suffix prefix
	CHECK(d.spell_priv("31b2") == true);
}

TEST_CASE("Dictionary::spell_priv break_pattern", "[dictionary]")
{
	auto d = Dict_Test();

	d.forbid_warn = true;
	d.warn_flag = 'W';

	d.words.emplace("user", u"");
	d.words.emplace("interface", u"");
	d.words.emplace("interface-interface", u"W");

	d.break_table = {"-", "++++++$"};

	auto good = {"user", "interface", "user-interface", "interface-user",
	             "user-user"};
	for (auto& g : good)
		CHECK(d.spell_priv(g) == true);

	auto wrong = {"user--interface", "user interface", "user - interface",
	              "interface-interface"};
	for (auto& w : wrong)
		CHECK(d.spell_priv(w) == false);
}

TEST_CASE("Dictionary::spell_priv spell_casing_upper", "[dictionary]")
{
	auto d = Dict_Test();

	d.words.emplace("Sant'Elia", u"");
	d.words.emplace("d'Osormort", u"");

	auto good = {"SANT'ELIA", "D'OSORMORT"};
	for (auto& g : good)
		CHECK(d.spell_priv(g) == true);
}

TEST_CASE("Dictionary::spell_priv compounding begin_last", "[dictionary]")
{
	auto d = Dict_Test();

	d.compound_begin_flag = 'B';
	d.compound_last_flag = 'L';

	d.compound_min_length = 4;
	d.words.emplace("car", u"B");
	d.words.emplace("cook", u"B");
	d.words.emplace("photo", u"B");
	d.words.emplace("book", u"L");

	auto good = {"cookbook", "photobook"};
	for (auto& g : good)
		CHECK(d.spell_priv(g) == true);
	auto wrong = {"carbook", "bookcook", "bookphoto", "cookphoto",
	              "photocook"};
	for (auto& w : wrong)
		CHECK(d.spell_priv(w) == false);
}

TEST_CASE("Dictionary::spell_priv compounding compound_middle", "[dictionary]")
{
	auto d = Dict_Test();
	d.compound_flag = 'C';
	d.compound_middle_flag = 'M';
	d.compound_check_duplicate = true;
	d.words.emplace("goederen", u"C");
	d.words.emplace("trein", u"M");
	d.words.emplace("wagon", u"C");

	auto good = {"goederentreinwagon", "wagontreingoederen",
	             "goederenwagon", "wagongoederen"};
	for (auto& g : good)
		CHECK(d.spell_priv(g) == true);
	auto wrong = {"goederentrein", "treingoederen", "treinwagon",
	              "wagontrein",    "treintrein",    "goederengoederen",
	              "wagonwagon"};
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
	d.words.emplace("schiff", u"B");
	d.words.emplace("fahrt", u"L");

	auto good = {"Schiffahrt", "schiffahrt"};
	for (auto& g : good)
		CHECK(d.spell_priv(g) == true);
	auto wrong = {"Schifffahrt", "schifffahrt", "SchiffFahrt",
	              "SchifFahrt",  "schiffFahrt", "schifFahrt"};
	for (auto& w : wrong)
		CHECK(d.spell_priv(w) == false);
}

TEST_CASE("Dictionary suggestions rep_suggest", "[dictionary]")
{
	auto d = Dict_Test();

	d.replacements = {{"ph", "f"},
	                  {"shun$", "tion"},
	                  {"^voo", "foo"},
	                  {"^alot$", "a lot"}};
	auto good = "fat";
	d.words.emplace("fat", u"");
	CHECK(d.spell_priv(good) == true);
	auto w = string("phat");
	CHECK(d.spell_priv(w) == false);
	auto out_sug = List_Strings();
	auto expected_sug = List_Strings{good};
	d.rep_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);
	w = string("fad phat");
	out_sug.clear();
	expected_sug.clear();
	d.rep_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	good = "station";
	d.words.emplace("station", u"");
	CHECK(d.spell_priv(good) == true);
	w = string("stashun");
	CHECK(d.spell_priv(w) == false);
	out_sug.clear();
	expected_sug = {good};
	d.rep_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	d.words.emplace("stations", u"");
	w = string("stashuns");
	CHECK(d.spell_priv(w) == false);
	out_sug.clear();
	expected_sug.clear();
	d.rep_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	good = "food";
	d.words.emplace("food", u"");
	CHECK(d.spell_priv(good) == true);
	w = string("vood");
	CHECK(d.spell_priv(w) == false);
	out_sug.clear();
	expected_sug = {good};
	d.rep_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	w = string("vvood");
	CHECK(d.spell_priv(w) == false);
	out_sug.clear();
	expected_sug.clear();
	d.rep_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	good = "a lot";
	d.words.emplace("a lot", u"");
	CHECK(d.spell_priv(good) == true);
	w = string("alot");
	CHECK(d.spell_priv(w) == false);
	out_sug.clear();
	expected_sug = {good};
	d.rep_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	w = string("aalot");
	CHECK(d.spell_priv(w) == false);
	out_sug.clear();
	expected_sug.clear();
	d.rep_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	w = string("alott");
	CHECK(d.spell_priv(w) == false);
	out_sug.clear();
	expected_sug.clear();
	d.rep_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);
}

TEST_CASE("Dictionary suggestions extra_char_suggest", "[dictionary]")
{
	auto d = Dict_Test();

	auto good = "table";
	d.try_chars = good;
	d.words.emplace("table", u"");
	CHECK(d.spell_priv(good) == true);

	auto w = string("tabble");
	CHECK(d.spell_priv(w) == false);

	auto out_sug = List_Strings();
	auto expected_sug = List_Strings{good, good};
	d.extra_char_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	d.forbid_warn = true;
	d.warn_flag = *u"W";
	d.words.emplace("late", u"W");
	w = string("laate");
	out_sug.clear();
	expected_sug.clear();
	d.extra_char_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);
}

TEST_CASE("Dictionary suggestions map_suggest", "[dictionary]")
{
	auto d = Dict_Test();

	auto good = "naïve";
	d.words.emplace("naïve", u"");
	d.similarities = {Similarity_Group<char>("iíìîï")};
	CHECK(d.spell_priv(good) == true);

	auto w = string("naive");
	CHECK(d.spell_priv(w) == false);

	auto out_sug = List_Strings();
	auto expected_sug = List_Strings{good};
	d.map_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	d.words.emplace("æon", u"");
	d.similarities.push_back(Similarity_Group<char>("æ(ae)"));
	good = "æon";
	CHECK(d.spell_priv(good) == true);
	w = string("aeon");
	CHECK(d.spell_priv(w) == false);
	out_sug.clear();
	expected_sug = {good};
	d.map_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	d.words.emplace("zijn", u"");
	d.similarities.push_back(Similarity_Group<char>("(ij)ĳ"));
	good = "zijn";
	CHECK(d.spell_priv(good) == true);
	w = string("zĳn");
	CHECK(d.spell_priv(w) == false);
	out_sug.clear();
	expected_sug = {good};
	d.map_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	d.words.emplace("hear", u"");
	d.similarities.push_back(Similarity_Group<char>("(ae)(ea)"));
	good = "hear";
	CHECK(d.spell_priv(good) == true);
	w = string("haer");
	CHECK(d.spell_priv(w) == false);
	out_sug.clear();
	expected_sug = {good};
	d.map_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);
}

TEST_CASE("Dictionary suggestions keyboard_suggest", "[dictionary]")
{
	auto d = Dict_Test();

	auto good1 = "abcd";
	auto good2 = "Abb";
	d.words.emplace("abcd", u"");
	d.words.emplace("Abb", u"");
	d.keyboard_closeness = "uiop|xdf|nm";
	CHECK(d.spell_priv(good1) == true);

	auto w = string("abcf");
	CHECK(d.spell_priv(w) == false);

	auto out_sug = List_Strings();
	auto expected_sug = List_Strings{good1};
	d.keyboard_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	w = string("abcx");
	CHECK(d.spell_priv(w) == false);
	out_sug.clear();
	expected_sug = {good1};
	d.keyboard_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	w = string("abcg");
	CHECK(d.spell_priv(w) == false);
	out_sug.clear();
	expected_sug.clear();
	d.keyboard_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);

	w = string("abb");
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

	auto good = "chair";
	d.words.emplace("chair", u"");
	d.try_chars = good;
	CHECK(d.spell_priv(good) == true);

	auto w = string("cháir");
	CHECK(d.spell_priv(w) == false);

	auto out_sug = List_Strings();
	auto expected_sug = List_Strings{good};
	d.bad_char_suggest(w, out_sug);
	CHECK(out_sug == expected_sug);
}

TEST_CASE("forgotten_char_suggest", "[dictionary]")
{
	auto d = Dict_Test();

	auto good = "abcd";
	d.words.emplace("abcd", u"");
	d.try_chars = good;
	CHECK(d.spell_priv(good) == true);

	auto w = string("abd");
	CHECK(d.spell_priv(w) == false);

	auto out_sug = List_Strings();
	auto expected_sug = List_Strings{good};
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

	d.try_chars = "ailrt";

	// extra char, bad char, bad char, forgotten char
	auto words = vector<string>{"tral", "traalt", "trial", "trail"};
	for (auto& x : words)
		d.words.emplace(x, u"");

	auto w = string("traal");
	auto out_sug = List_Strings();
	d.suggest_priv(w, out_sug);
	auto out_sug2 = out_sug.extract_sequence();
	CHECK(words == out_sug2);
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
