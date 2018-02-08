/* Copyright 2016-2017 Dimitrij Mijoski
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

#include "aff_data.hxx"

#include "locale_utils.hxx"
#include "string_utils.hxx"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <unordered_map>

#include <fstream> // Only here for logging.
#include <iomanip> // Only here for logging.

#include <boost/algorithm/string.hpp>
#include <boost/locale.hpp>

/*
 * Aff_Data class and the method parse() should be structured in the following
 * way. The data members of the class should be data structures that are
 * useful for reading and searching operations, but are not necessarely
 * good for dynamic updates. They should be designed toward their later use
 * while spellchecking.
 *
 * On the other hand, the method parse() should fill the data in intermediate
 * data structures that are good for dynamic updates.
 *
 * E.g. In the class: it can store the affixes in a sorted vector by the key
 * on which will be searched, the adding affix. (aka flat_map)
 *
 * In method parse(): it will fill the affixes in simple unsorted vector.
 *
 * For simple stiff like simple int, flag, or for data structures that are
 * equally good for updating and reading, just parse directly into the class
 * data members.
 *
 * class Aff_data {
 *	Affix_Table affix_table;     // wraps a vector and keeps invariant
 *	                                that the vector is always sorted
 *	Substring_Replace_Table rep_table;
 * };
 *
 * parse(istream in) {
 *	vector<aff_entry> affix_table_intermediate;
 *	vector<rep_entry> rep_table_intermediate;
 *	while (getline) {
 *	...
 *		affix_table_intermediate.push_back(entry)
 *	...
 *	}
 *	At the end
 *	affix_table = std::move(affix_table_intermediate);
 * }
 */

namespace hunspell {

using namespace std;

Encoding::Encoding(const std::string& e, const std::locale& ascii_loc) : name(e)
{
	boost::algorithm::to_upper(name, ascii_loc);
	if (name == "UTF8")
		name = "UTF-8";
}

void reset_failbit_istream(std::istream& in)
{
	in.clear(in.rdstate() & ~in.failbit);
}

bool read_to_slash_or_space(std::istream& in, std::string& out)
{
	in >> std::ws;
	int c;
	bool readSomething = false;
	while ((c = in.get()) != std::istream::traits_type::eof() &&
	       !isspace((char)c, in.getloc()) && c != '/') {
		out.push_back(c);
		readSomething = true;
	}
	bool slash = c == '/';
	if (readSomething || slash) {
		reset_failbit_istream(in);
	}
	return slash;
}

template <class T, class Func>
auto parse_vector_of_T(/* in */ istream& in, /* in */ size_t line_num,
                       /* in */ const string& command,
                       /* in out */ unordered_map<string, int>& counts,
                       /* in out */ vector<T>& vec,
                       /* in */ Func parseLineFunc) -> void
{
	auto dat = counts.find(command);
	if (dat == counts.end()) {
		// first line
		size_t a;
		in >> a;
		if (in.fail()) {
			a = 0; // err
			cerr << "Hunspell error: a vector command (series of "
			        "of similar commands) has no count. Ignoring "
			        "all of them.\n";
		}
		counts[command] = a;
	}
	else if (dat->second) {
		vec.emplace_back();
		parseLineFunc(in, vec.back());
		if (in.fail()) {
			vec.pop_back();
			cerr << "Hunspell error: single entry of a vector "
			        "command (series of "
			        "of similar commands) is invalid.\n";
		}
		dat->second--;
	}
	else {
		cerr << "Hunspell warning: extra entries of " << command
		     << '\n';
		cerr << "Hunspell warning in line " << line_num << endl;
	}
}

// Expects that there are flags in the stream.
// If there are no flags in the stream (eg, stream is at eof)
// or if the format of the flags is incorrect the stream failbit will be set.
auto decode_flags(/* in */ istream& in, /* in */ size_t line_num,
                  /* in */ Flag_Type t, /* in */ const Encoding& enc)
    -> u16string
{
	string s;
	u16string ret;
	const auto err_message = "Hunspell warning: bytes above 127 in UTF-8 "
	                         "stream should not be treated alone as "
	                         "flags. Please update dictionary to use "
	                         "FLAG UTF-8 and make the file valid UTF-8.";
	switch (t) {
	case FLAG_SINGLE_CHAR:
		in >> s;
		if (in.fail()) {
			// err no flag at all
			cerr << "Hunspell error: missing flag\n";
			break;
		}
		if (enc.is_utf8() && !is_all_ascii(s)) {
			cerr << err_message << '\n';
			cerr << "Hunspell warning in line " << line_num
			     << "\n\n";
			// This error will be triggered in Hungarian.
			// Version 1 passed this, it just read a
			// single byte even if the stream utf-8.
			// Hungarian dictionary explited this
			// bug/feature, resulting it's file to be
			// mixed utf-8 and latin2.
			// In v2 this will eventually work, with
			// a warning.
		}
		ret = latin1_to_ucs2(s);
		break;
	case FLAG_DOUBLE_CHAR: {
		in >> s;
		if (in.fail()) {
			// err no flag at all
			cerr << "Hunspell error: missing flag\n";
			break;
		}
		if (enc.is_utf8() && !is_all_ascii(s)) {
			cerr << err_message << endl;
			cerr << "Hunspell warning in line " << line_num << endl;
		}
		auto i = s.begin();
		auto e = s.end();
		if (s.size() & 1) {
			--e;
		}
		for (; i != e; i += 2) {
			char16_t c1 = (unsigned char)*i;
			char16_t c2 = (unsigned char)*(i + 1);
			ret.push_back((c1 << 8) | c2);
		}
		if (i != s.end()) {
			ret.push_back((unsigned char)*i);
		}
		break;
	}
	case FLAG_NUMBER:
		unsigned short flag;
		in >> flag;
		if (in.fail()) {
			// err no flag at all
			cerr << "Hunspell error: missing flag\n";
			break;
		}
		ret.push_back(flag);
		// peek can set failbit
		while (in.good() && in.peek() == ',') {
			in.get();
			if (in >> flag) {
				ret.push_back(flag);
			}
			else {
				// err, comma and no number after that
				cerr << "Hunspell error: long flag, no number "
				        "after comma\n";
				break;
			}
		}
		break;
	case FLAG_UTF8: {
		in >> s;
		if (!enc.is_utf8()) {
			// err
			cerr << "Hunspell error: File encoding is not UTF-8, "
			        "yet flags are\n";
		}
		if (in.fail()) {
			// err no flag at all
			cerr << "Hunspell error: missing flag\n";
			break;
		}
		auto u32flags = decode_utf8(s);
		if (!is_all_bmp(u32flags)) {
			cerr << "Hunspell warning: flags must be in BMP. "
			        "Skipping non-BMP\n"
			     << "Hunspell warning in line " << line_num << endl;
		}
		ret = u32_to_ucs2_skip_non_bmp(u32flags);
		break;
	}
	}
	return ret;
}

auto decode_single_flag(/* in */ istream& in, /* in */ size_t line_num,
                        /* in */ Flag_Type t, /* in */ Encoding enc) -> char16_t
{
	auto flags = decode_flags(in, line_num, t, enc);
	if (flags.size()) {
		return flags.front();
	}
	return 0;
}

auto parse_affix(/* in */ istream& in, /* in */ size_t line_num,
                 /* in out */ string& command,
                 /* in */ Flag_Type t, /* in */ Encoding enc,
                 /* in out */ vector<Affix>& vec,
                 /* in out */ unordered_map<string, pair<bool, int>>& cmd_affix)
    -> void
{
	char16_t f = decode_single_flag(in, line_num, t, enc);
	if (f == 0) {
		// err
		return;
	}
	char f1 = f & 0xff;
	char f2 = (f >> 8) & 0xff;
	command.push_back(f1);
	command.push_back(f2);
	auto dat = cmd_affix.find(command);
	// note: the current affix parser does not allow the same flag
	// to be used once with cross product and again witohut
	// one flag is tied to one cross product value
	if (dat == cmd_affix.end()) {
		char cross_char; // 'Y' or 'N'
		size_t cnt;
		in >> cross_char >> cnt;
		bool cross = cross_char == 'Y';
		if (in.fail()) {
			cnt = 0; // err
			cerr << "Hunspell error: a SFX/PFX header command is "
			        "invalid. Missing count or cross product\n";
		}
		cmd_affix[command] = make_pair(cross, cnt);
	}
	else if (dat->second.second) {
		vec.emplace_back();
		auto& elem = vec.back();
		elem.flag = f;
		elem.cross_product = dat->second.first;
		in >> elem.stripping;
		if (read_to_slash_or_space(in, elem.affix)) {
			elem.new_flags = decode_flags(in, line_num, t, enc);
		}
		in >> elem.condition;
		if (in.fail()) {
			vec.pop_back();
		}
		else {
			parse_morhological_fields(in,
			                          elem.morphological_fields);
		}
		dat->second.second--;
	}
	else {
		cerr << "Hunspell warning: extra entries of "
		     << command.substr(0, 3) << '\n'
		     << "Hunspell warning in line " << line_num << endl;
	}
}

auto parse_flag_type(/* in */ istream& in, /* in */ size_t line_num,
                     /* in out */ Flag_Type& flag_type) -> void
{
	(void)line_num;
	string p;
	in >> p;
	boost::algorithm::to_upper(p, in.getloc());
	if (p == "LONG")
		flag_type = FLAG_DOUBLE_CHAR;
	else if (p == "NUM")
		flag_type = FLAG_NUMBER;
	else if (p == "UTF-8")
		flag_type = FLAG_UTF8;
	else
		cerr << "Hunspell error: unknown FLAG type\n";
}

auto parse_morhological_fields(/* in */ istream& in,
                               /* in out */ vector<string>& vecOut) -> void
{
	if (!in.good()) {
		return;
	}

	std::string morph;
	while (in >> morph) {
		vecOut.push_back(morph);
	}
	reset_failbit_istream(in);
}

auto Aff_Data::decode_flags(istream& in, size_t line_num) const -> u16string
{
	return hunspell::decode_flags(in, line_num, flag_type, encoding);
}

auto Aff_Data::decode_single_flag(istream& in, size_t line_num) const
    -> char16_t
{
	return hunspell::decode_single_flag(in, line_num, flag_type, encoding);
}

auto get_locale_name(string lang, string enc, const string& filename) -> string
{
	if (enc.empty())
		enc = "ISO8859-1";
	if (lang.empty()) {
		if (filename.empty()) {
			lang = "en_US";
		}
	}
	return lang + "." + enc;
}

auto Aff_Data::parse(istream& in) -> bool
{
	unordered_map<string, string*> command_strings = {
	    {"LANG", &language_code},
	    {"IGNORE", &ignore_chars},

	    {"KEY", &keyboard_layout},
	    {"TRY", &try_chars},

	    {"WORDCHARS", &wordchars}};

	unordered_map<string, bool*> command_bools = {
	    {"COMPLEXPREFIXES", &complex_prefixes},

	    {"ONLYMAXDIFF", &only_max_diff},
	    {"NOSPLITSUGS", &no_split_suggestions},
	    {"SUGSWITHDOTS", &suggest_with_dots},
	    {"FORBIDWARN", &forbid_warn},

	    {"COMPOUNDMORESUFFIXES", &compound_more_suffixes},
	    {"CHECKCOMPOUNDDUP", &compound_check_up},
	    {"CHECKCOMPOUNDREP", &compound_check_rep},
	    {"CHECKCOMPOUNDCASE", &compound_check_case},
	    {"CHECKCOMPOUNDTRIPLE", &compound_check_triple},
	    {"SIMPLIFIEDTRIPLE", &compound_simplified_triple},

	    {"FULLSTRIP", &fullstrip},
	    {"CHECKSHARPS", &checksharps}};

	unordered_map<string, vector<string>*> command_vec_str = {
	    {"BREAK", &break_patterns},
	    {"MAP", &map_related_chars}, // maybe add special parsing code
	    {"COMPOUNDRULE", &compound_rules}};

	unordered_map<string, short*> command_shorts = {
	    {"MAXCPDSUGS", &max_compound_suggestions},
	    {"MAXNGRAMSUGS", &max_ngram_suggestions},
	    {"MAXDIFF", &max_diff_factor},

	    {"COMPOUNDMIN", &compound_minimum},
	    {"COMPOUNDWORDMAX", &compound_word_max}};

	unordered_map<string, vector<pair<string, string>>*> command_vec_pair =
	    {{"REP", &replacements},
	     {"PHONE", &phonetic_replacements},
	     {"ICONV", &input_conversion},
	     {"OCONV", &output_conversion}};

	unordered_map<string, char16_t*> command_flag = {
	    {"NOSUGGEST", &nosuggest_flag},
	    {"WARN", &warn_flag},

	    {"COMPOUNDFLAG", &compound_flag},
	    {"COMPOUNDBEGIN", &compound_begin_flag},
	    {"COMPOUNDLAST", &compound_last_flag},
	    {"COMPOUNDMIDDLE", &compound_middle_flag},
	    {"ONLYINCOMPOUND", &compound_onlyin_flag},
	    {"COMPOUNDPERMITFLAG", &compound_permit_flag},
	    {"COMPOUNDFORBIDFLAG", &compound_forbid_flag},
	    {"COMPOUNDROOT", &compound_root_flag},
	    {"FORCEUCASE", &compound_force_uppercase},

	    {"CIRCUMFIX", &circumfix_flag},
	    {"FORBIDDENWORD", &forbiddenword_flag},
	    {"KEEPCASE", &keepcase_flag},
	    {"NEEDAFFIX", &need_affix_flag},
	    {"SUBSTANDARD", &substandard_flag}};

	// keeps count for each vector
	auto cmd_with_vec_cnt = unordered_map<string, int>();
	auto cmd_affix = unordered_map<string, pair<bool, int>>();
	auto line = string();
	auto command = string();
	size_t line_num = 0;
	auto ss = istringstream();

	boost::locale::generator locale_generator;
	// auto loc = locale_generator("en_US.us-ascii");
	auto loc = locale::classic();
	// while parsing, the streams must have plain ascii locale without
	// any special number separator otherwise istream >> int might fail
	// due to thousands separator.
	// "C" locale can be used assuming it is US-ASCII,
	// but also boost en_US.us-ascii will do, avoiding any assumptions
	in.imbue(loc);
	ss.imbue(loc);

	flag_type = FLAG_SINGLE_CHAR;
	while (getline(in, line)) {
		line_num++;

		if (encoding.is_utf8() && !validate_utf8(line)) {
			cerr << "Hunspell warning: invalid utf in aff file\n";
			// Hungarian will triger this, contains mixed
			// utf-8 and latin2. See note in decode_flags().
		}

		ss.str(line);
		ss.clear();
		ss >> ws;
		if (ss.eof() || ss.peek() == '#') {
			continue; // skip comment or empty lines
		}
		ss >> command;
		boost::algorithm::to_upper(command, ss.getloc());
		ss >> ws;
		if (command == "PFX" || command == "SFX") {
			auto& vec = command[0] == 'P' ? prefixes : suffixes;
			parse_affix(ss, line_num, command, flag_type, encoding,
			            vec, cmd_affix);
		}
		else if (command_strings.count(command)) {
			auto& str = *command_strings[command];
			if (str.empty())
				ss >> str;
			else
				cerr << "Hunspell warning: "
				        "Setting "
				     << command
				     << " more than once. Ignoring.\n"
				     << "Hunspell warning in line " << line_num
				     << '\n';
		}
		else if (command_bools.count(command)) {
			*command_bools[command] = true;
		}
		else if (command_shorts.count(command)) {
			ss >> *command_shorts[command];
		}
		else if (command_flag.count(command)) {
			*command_flag[command] =
			    decode_single_flag(ss, line_num);
		}
		else if (command_vec_str.count(command)) {
			auto& vec = *command_vec_str[command];
			auto func = [&](istream& in, string& p) { in >> p; };
			parse_vector_of_T(ss, line_num, command,
			                  cmd_with_vec_cnt, vec, func);
		}
		else if (command_vec_pair.count(command)) {
			auto& vec = *command_vec_pair[command];
			auto func = [&](istream& in, pair<string, string>& p) {
				in >> p.first >> p.second;
			};
			parse_vector_of_T(ss, line_num, command,
			                  cmd_with_vec_cnt, vec, func);
		}
		else if (command == "SET") {
			if (encoding.empty()) {
				auto str = string();
				ss >> str;
				encoding = str;
			}
			else {
				cerr << "Hunspell warning: "
				        "Setting "
				     << command
				     << " more than once. Ignoring.\n"
				     << "Hunspell warning in line " << line_num
				     << '\n';
			}
		}
		else if (command == "FLAG") {
			parse_flag_type(ss, line_num, flag_type);
		}
		else if (command == "AF") {
			auto& vec = flag_aliases;
			auto func = [&](istream& inn, Flag_Set& p) {
				p = decode_flags(inn, line_num);
			};
			parse_vector_of_T(ss, line_num, command,
			                  cmd_with_vec_cnt, vec, func);
		}
		else if (command == "AM") {
			auto& vec = morphological_aliases;
			parse_vector_of_T(ss, line_num, command,
			                  cmd_with_vec_cnt, vec,
			                  parse_morhological_fields);
		}
		else if (command == "CHECKCOMPOUNDPATTERN") {
			auto& vec = compound_check_patterns;
			auto func = [&](istream& in,
			                Compound_Check_Pattern& p) {
				if (read_to_slash_or_space(in, p.end_chars)) {
					p.end_flag =
					    decode_single_flag(in, line_num);
				}
				if (read_to_slash_or_space(in, p.begin_chars)) {
					p.begin_flag =
					    decode_single_flag(in, line_num);
				}
				if (in.fail()) {
					return;
				}
				in >> p.replacement;
				reset_failbit_istream(in);
			};
			parse_vector_of_T(ss, line_num, command,
			                  cmd_with_vec_cnt, vec, func);
		}
		else if (command == "COMPOUNDSYLLABLE") {
			ss >> compound_syllable_max >> compound_syllable_vowels;
		}
		else if (command == "SYLLABLENUM") {
			compound_syllable_num = decode_flags(ss, line_num);
		}
		if (ss.fail()) {
			cerr << "Hunspell aff error in line " << line_num
			     << ": " << line << endl;
		}
	}
	locale_aff = locale_generator(get_locale_name(language_code, encoding));
	cerr.flush();

	// default BREAK definition
	if (!break_patterns.size()) {
		break_patterns.push_back("-");
		break_patterns.push_back("^-");
		break_patterns.push_back("-$");
	}

	return in.eof(); // success when eof is reached
}

void Aff_Data::log(const string& affpath)
{
	std::ofstream log_file;
	auto log_name = std::string(".am2.log"); // 1: Hunspell, 2: Nuspell
	log_name.insert(0, affpath);
	if (log_name.substr(0, 2) == "./")
		log_name.erase(0, 2);
	log_name.insert(0, "../v1cmdline/"); // prevent logging somewhere else
	log_file.open(log_name, std::ios_base::out);
	fprintf(stderr, "log file %s\n", log_name.c_str());
	if (!log_file.is_open()) {
		return;
	}
	log_file << "affpath/affpath\t"
	         << log_name.erase(log_name.size() - 8, 8) << std::endl;
	//	log_file << "key\tTODO" << std::endl;
	log_file << "AFTER parse" << std::endl;

	log_file << std::endl << "BASIC" << std::endl;
	// The contents of alldic and pHMgr are logged by a
	// seprate log
	// method in the hash manager.

	// log_file << "encoding/encoding.value\t\"" <<
	// encoding.value()
	//	 << "\"" << std::endl;
	// TODO flag_type
	log_file << "complexprefixes/complex_prefixes\t" << complex_prefixes
	         << std::endl;
	log_file << "lang/language_code\t\"" << language_code << "\""
	         << std::endl;
	log_file << "ignorechars/ignore_chars\t\"" << ignore_chars << "\""
	         << std::endl;
	// TODO flag_aliases
	// TODO morphological_aliases

	// TODO locale_aff

	log_file << std::endl << "SUGGESTION OPTIONS" << std::endl;
	log_file << "keystring/keyboard_layout\t\"" << keyboard_layout << "\""
	         << std::endl;
	log_file << "trystring/try_chars\t\"" << try_chars << "\"" << std::endl;
	log_file << "nosuggest/nosuggest_flag\t" << nosuggest_flag << std::endl;
	log_file << "maxcpdsugs/max_compound_suggestions\t"
	         << max_compound_suggestions << std::endl;
	log_file << "maxngramsugs/max_ngram_suggestions\t"
	         << max_ngram_suggestions << std::endl;
	log_file << "maxdiff/max_diff_factor\t" << max_diff_factor << std::endl;
	log_file << "onlymaxdiff/only_max_diff;\t" << only_max_diff
	         << std::endl;
	log_file << "nosplitsugs/no_split_suggestions\t" << no_split_suggestions
	         << std::endl;
	log_file << "sugswithdots/suggest_with_dots\t" << suggest_with_dots
	         << std::endl;
	for (std::vector<pair<string, string>>::const_iterator i =
	         replacements.begin();
	     i != replacements.end(); ++i) {
		log_file << "reptable/replacements_" << std::setw(3)
		         << std::setfill('0') << i - replacements.begin() + 1
		         << "\t\"" << i->first << "\"\t\"" << i->second << "\""
		         << std::endl;
	}
	for (std::vector<std::string>::const_iterator i =
	         map_related_chars.begin();
	     i != map_related_chars.end(); ++i) {
		log_file << "maptable_" << std::setw(3) << std::setfill('0')
		         << i - map_related_chars.begin() + 1 << "\t\"" << *i
		         << "\"" << std::endl;
	}
	for (std::vector<pair<string, string>>::const_iterator i =
	         phonetic_replacements.begin();
	     i != phonetic_replacements.end(); ++i) {
		log_file << "phone.rules/phonetic_replacements_" << std::setw(3)
		         << std::setfill('0')
		         << i - phonetic_replacements.begin() << "\t\""
		         << i->first << "\"\t\"" << i->second << "\""
		         << std::endl;
		// TODO phone->hash
		// TODO phone->utf8
	}
	log_file << "warn/warn_flag\t" << warn_flag << std::endl;
	log_file << "forbidwarn/forbid_warn\t" << forbid_warn << std::endl;

	log_file << std::endl << "COMPOUNDING OPTIONS" << std::endl;
	for (std::vector<std::string>::const_iterator i =
	         break_patterns.begin();
	     i != break_patterns.end(); ++i) {
		log_file << "breaktable/break_patterns_" << std::setw(3)
		         << std::setfill('0') << i - break_patterns.begin() + 1
		         << "\t\"" << *i << "\"" << std::endl;
	}
	// TODO compound_rules
	log_file << "cpdmin/compound_minimum\t" << compound_minimum
	         << std::endl;
	log_file << "compoundflag/compound_flag\t" << compound_flag
	         << std::endl;
	log_file << "compoundbegin/compound_begin_flag\t" << compound_begin_flag
	         << std::endl;
	log_file << "compoundend/compound_last_flag\t" << compound_last_flag
	         << std::endl;
	log_file << "compoundmiddle/compound_middle_flag\t"
	         << compound_middle_flag << std::endl;
	log_file << "onlyincompound/compound_onlyin_flag\t"
	         << compound_onlyin_flag << std::endl;
	log_file << "compoundpermitflag/compound_permit_flag\t"
	         << compound_permit_flag << std::endl;
	log_file << "compoundforbidflag/compound_forbid_flag\t"
	         << compound_forbid_flag << std::endl;
	log_file << "compoundmoresuffixes/compound_more_suffixes\t"
	         << compound_more_suffixes << std::endl;
	log_file << "compoundroot/compound_root_flag\t" << compound_root_flag
	         << std::endl;
	log_file << "cpdwordmax/compound_word_max\t" << compound_word_max
	         << std::endl;
	log_file << "checkcompounddup/compound_check_up\t" << compound_check_up
	         << std::endl;
	log_file << "checkcompoundrep/compound_check_rep\t"
	         << compound_check_rep << std::endl;
	log_file << "checkcompoundcase/compound_check_case\t"
	         << compound_check_case << std::endl;
	log_file << "checkcompoundtriple/compound_check_triple\t"
	         << compound_check_triple << std::endl;
	log_file << "simplifiedtriple/compound_simplified_triple\t"
	         << compound_simplified_triple << std::endl;
	for (std::vector<Compound_Check_Pattern>::const_iterator i =
	         compound_check_patterns.begin();
	     i != compound_check_patterns.end(); ++i) {
		log_file << "checkcpdtable/compound_check_patterns_"
		         << std::setw(3) << std::setfill('0')
		         << i - compound_check_patterns.begin() << "\t\""
		         << i->end_chars << "\"" << std::endl;
		log_file << "checkcpdtable/compound_check_patterns_"
		         << std::setw(3) << std::setfill('0')
		         << i - compound_check_patterns.begin() << "\t\""
		         << i->begin_chars << "\"" << std::endl;
		log_file << "checkcpdtable/compound_check_patterns_"
		         << std::setw(3) << std::setfill('0')
		         << i - compound_check_patterns.begin() << "\t\""
		         << i->replacement << "\"" << std::endl;
		// TODO cond and cond2
	}
	log_file << "forceucase/compound_force_uppercase\t"
	         << compound_force_uppercase << std::endl;
	log_file << "cpdmaxsyllable/compound_syllable_max\t"
	         << compound_syllable_max << std::endl;
	log_file << "cpdvowels/compound_syllable_vowels\t\""
	         << compound_syllable_vowels << "\"" << std::endl;
	log_file << "cpdsyllablenum.length/"
	            "compound_syllable_num.size\t"
	         << compound_syllable_num.size() << std::endl;
	for (std::vector<Affix>::const_iterator i = prefixes.begin();
	     i != prefixes.end(); ++i) {
		log_file << "pfx/prefixes_" << std::setw(3) << std::setfill('0')
		         << i - prefixes.begin() + 1 << ".appnd/affix\t\""
		         << i->affix << "\"" << std::endl;
		log_file << "pfx/prefixes_" << std::setw(3) << std::setfill('0')
		         << i - prefixes.begin() + 1 << ".aflag/flag\t"
		         << i->flag << std::endl;
		//		log_file << "pfx/prefixes_" <<
		// std::setw(3) <<
		// std::setfill('0')
		//		         << i - prefixes.begin() +
		// 1 <<
		//".contclasslen\t"
		//		         << i->affix <<
		// std::endl;
	}
	for (std::vector<Affix>::const_iterator i = suffixes.begin();
	     i != suffixes.end(); ++i) {
		log_file << "sfx/suffixes_" << std::setw(3) << std::setfill('0')
		         << i - suffixes.begin() + 1 << ".appnd/affix\t\""
		         << i->affix << "\"" << std::endl;
		log_file << "sfx/suffixes_" << std::setw(3) << std::setfill('0')
		         << i - suffixes.begin() + 1 << ".aflag/flag\t"
		         << i->flag << std::endl;
		log_file << "sfx/suffixes_" << std::setw(3) << std::setfill('0')
		         << i - suffixes.begin() << ".cross()/cross_product\t"
		         << i->cross_product << std::endl;
		//		log_gile << "sfx/suffixes_" <<
		// std::setw(3) <<
		// std::setfill('0')
		//			 << i - suffixes.begin() + 1
		//<<
		//".contclasslen\t"
		//			 << i->affix <<
		// std::endl;
		log_file << "sfx/suffixes_" << std::setw(3) << std::setfill('0')
		         << i - suffixes.begin() + 1 << ".strip/stripping\t\""
		         << i->stripping << "\"" << std::endl;
	}

	log_file << std::endl << "OTHERS" << std::endl;
	log_file << "circumfix/circumfix_flag\t" << circumfix_flag << std::endl;
	log_file << "forbiddenword/forbiddenword_flag\t" << forbiddenword_flag
	         << std::endl;
	log_file << "fullstrip/fullstrip\t" << fullstrip << std::endl;
	log_file << "keepcase/keepcase_flag\t" << keepcase_flag << std::endl;
	for (std::vector<pair<string, string>>::const_iterator i =
	         input_conversion.begin();
	     i != input_conversion.end(); ++i) {
		log_file << "iconvtable/input_conversion_" << std::setw(3)
		         << std::setfill('0')
		         << i - input_conversion.begin() + 1 << "\t\""
		         << i->first << "\"\t\"" << i->second << "\""
		         << std::endl;
	}
	for (std::vector<pair<string, string>>::const_iterator i =
	         output_conversion.begin();
	     i != output_conversion.end(); ++i) {
		log_file << "oconvtable/output_conversion_" << std::setw(3)
		         << std::setfill('0')
		         << i - output_conversion.begin() + 1 << "\t\""
		         << i->first << "\"\t\"" << i->second << "\""
		         << std::endl;
	}
	log_file << "needaffix/need_affix_flag\t" << need_affix_flag
	         << std::endl;
	log_file << "substandard/substandard_flag\t" << substandard_flag
	         << std::endl;
	log_file << "wordchars/wordchars\t\"" << wordchars << "\"" << std::endl;
	log_file << "checksharps/checksharps\t" << checksharps << std::endl;

	log_file << "END" << std::endl;
}
}
