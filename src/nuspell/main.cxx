/* Copyright 2016-2018 Dimitrij Mijoski
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

/**
 * @file main.cxx
 * Command line spell checking.
 */

#include "dictionary.hxx"
#include "finder.hxx"
#include "string_utils.hxx"
#include <clocale>
#include <fstream>
#include <iostream>
#include <locale>
#include <string>
#include <unordered_map>

#include <boost/locale.hpp>

#ifdef HAVE_CONFIG_H
#include "../../config.h"
#else
// manually define
#ifndef PACKAGE_STRING
#define PACKAGE_STRING "nuspell 2.0.0"
#endif
#ifndef PACKAGE
#define PACKAGE "nuspell"
#endif
#endif

#if defined(__MINGW32__) || defined(__unix__) || defined(__unix) ||            \
    (defined(__APPLE__) && defined(__MACH__))
#include <getopt.h>
#include <unistd.h>
#endif

using namespace std;
using namespace nuspell;

enum Mode {
	DEFAULT_MODE /**< printing correct and misspelled words with
	                suggestions */
	,
	NO_SUGGEST_MODE /**< printing correct and misspelled words without
	                   suggestions */
	,
	MISSPELLED_WORDS_MODE /**< printing only misspelled words */,
	MISSPELLED_LINES_MODE /**< printing only misspelled lines */,
	CORRECT_WORDS_MODE /**< printing only correct words */,
	CORRECT_LINES_MODE /**< printing only correct lines */,
	LINES_MODE /**< where correctness is based on entire line */,
	LIST_DICTIONARIES_MODE /**< printing available dictionaries */,
	HELP_MODE /**< printing help information */,
	VERSION_MODE /**< printing version information */,
	ERROR_MODE /**< where the arguments used caused an error */
};

struct Args_t {
	Mode mode = DEFAULT_MODE;
	string program_name = PACKAGE;
	string dictionary;
	string encoding;
	vector<string> other_dicts;
	vector<string> files;

	Args_t() {}
	Args_t(int argc, char* argv[]) { parse_args(argc, argv); }
	auto parse_args(int argc, char* argv[]) -> void;
};

/**
 * Parses command line arguments and result is stored in mode, dictionary,
 * other_dicts and files.
 *
 * @param argc the total number of command line arguments.
 * @param argv all the individual command linen arguments.
 */
auto Args_t::parse_args(int argc, char* argv[]) -> void
{
	if (argc != 0 && argv[0] && argv[0][0] != '\0')
		program_name = argv[0];
// See POSIX Utility argument syntax
// http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap12.html
#if defined(_POSIX_VERSION) || defined(__MINGW32__)
	int c;
	// The program can run in various modes depending on the
	// command line options. mode is FSM state, this while loop is FSM.
	const char* shortopts = ":d:i:DGLUlhv";
	const struct option longopts[] = {
	    {"version", 0, 0, 'v'},
	    {"help", 0, 0, 'h'},
	    {nullptr, 0, 0, 0},
	};
	while ((c = getopt_long(argc, argv, shortopts, longopts, nullptr)) !=
	       -1) {
		switch (c) {
		case 'd':
			if (dictionary.empty())
				dictionary = optarg;
			else
				cerr << "WARNING: Detected not yet supported "
				        "other dictionary "
				     << optarg << '\n';
			other_dicts.emplace_back(optarg);

			break;
		case 'i':
			encoding = optarg;

			break;
		case 'D':
			if (mode == DEFAULT_MODE)
				mode = LIST_DICTIONARIES_MODE;
			else
				mode = ERROR_MODE;

			break;
		case 'G':
			if (mode == DEFAULT_MODE)
				mode = CORRECT_WORDS_MODE;
			else if (mode == LINES_MODE)
				mode = CORRECT_LINES_MODE;
			else
				mode = ERROR_MODE;

			break;
		case 'l':
			if (mode == DEFAULT_MODE)
				mode = MISSPELLED_WORDS_MODE;
			else if (mode == LINES_MODE)
				mode = MISSPELLED_LINES_MODE;
			else
				mode = ERROR_MODE;

			break;
		case 'L':
			if (mode == DEFAULT_MODE)
				mode = LINES_MODE;
			else if (mode == MISSPELLED_WORDS_MODE)
				mode = MISSPELLED_LINES_MODE;
			else if (mode == CORRECT_WORDS_MODE)
				mode = CORRECT_LINES_MODE;
			else
				mode = ERROR_MODE;

			break;
		case 'U':
			if (mode == DEFAULT_MODE)
				mode = NO_SUGGEST_MODE;
			else
				mode = ERROR_MODE;

			break;
		case 'h':
			if (mode == DEFAULT_MODE)
				mode = HELP_MODE;
			else
				mode = ERROR_MODE;

			break;
		case 'v':
			if (mode == DEFAULT_MODE)
				mode = VERSION_MODE;
			else
				mode = ERROR_MODE;

			break;
		case ':':
			cerr << "Option -" << static_cast<char>(optopt)
			     << " requires an operand\n";
			mode = ERROR_MODE;
			break;
		case '?':
			cerr << "Unrecognized option: '-"
			     << static_cast<char>(optopt) << "'\n";
			mode = ERROR_MODE;
			break;
		}
	}
	files.insert(files.end(), argv + optind, argv + argc);
	if (mode == LINES_MODE) {
		// in v1 this defaults to MISSPELLED_LINES_MODE
		// we will make it error here
		mode = ERROR_MODE;
	}
#endif
}

/**
 * Prints help information.
 * @param program_name pass argv[0] here.
 * @param out stream, standard output by default.
 */
auto print_help(const string& program_name) -> void
{
	auto& p = program_name;
	auto& o = cout;
	o << "Usage:\n"
	     "\n";
	o << p << " [-d dict_NAME] [-i enc] [file_name]...\n";
	o << p << " -l|-G [-L] [-U] [-d dict_NAME] [-i enc] [file_name]...\n";
	o << p << " -D|-h|--help|-v|--version\n";
	o << "\n"
	     "Check spelling of each FILE. Without FILE, check standard "
	     "input.\n"
	     "\n"
	     "  -d di_CT      use di_CT dictionary. Only one dictionary is\n"
	     "                currently supported\n"
	     "  -D            show available dictionaries and exit\n"
	     "  -i enc        input encoding, default is active locale\n"
	     "  -l            print only misspelled words or lines\n"
	     "  -G            print only correct words or lines\n"
	     "  -L            lines mode\n"
	     "  -U            do not suggest, increases performance\n"
	     "  -h, --help    display this help and exit\n"
	     "  -v, --version print version number and exit\n"
	     "\n";
	o << "Example: " << p << " -d en_US file.txt\n";
	o << "\n"
	     "Bug reports: <https://github.com/hunspell/nuspell/issues>\n"
	     "Full documentation: "
	     "<https://github.com/hunspell/hunspell/wiki>\n"
	     "Home page: <http://hunspell.github.io/>\n";
}

/**
 * Prints the version number to standard output.
 */
auto print_version() -> void
{
	cout << PACKAGE_STRING
	    "\n"
	    "Copyright (C) 2017-2018 Dimitrij Mijoski and Sander van Geloven\n"
	    "License LGPLv3+: GNU LGPL version 3 or later "
	    "<http://gnu.org/licenses/lgpl.html>.\n"
	    "This is free software: you are free to change and "
	    "redistribute it.\n"
	    "There is NO WARRANTY, to the extent permitted by law.\n"
	    "\n"
	    "Written by Dimitrij Mijoski, Sander van Geloven and others,\n"
	    "see https://github.com/hunspell/nuspell/blob/master/AUTHORS\n";
}

/**
 * Lists dictionary paths and available dictionaries on the system to standard
 * output.
 *
 * @param f a finder for search paths and located dictionary.
 */
auto list_dictionaries(const Finder& f) -> void
{
	if (f.get_all_paths().empty()) {
		cout << "No search paths available" << '\n';
	}
	else {
		cout << "Search paths:" << '\n';
		for (auto& p : f.get_all_paths()) {
			cout << p << '\n';
		}
	}

	// Even if no search paths are available, still report on available
	// dictionaries.
	if (f.get_all_dictionaries().empty()) {
		cout << "No dictionaries available\n";
	}
	else {
		cout << "Available dictionaries:\n";
		for (auto& d : f.get_all_dictionaries()) {
			cout << d.first;
			// The longest basenames of .dic and .aff files are
			// "ca_ES-valencia" with 14 characters and "de_DE_frami"
			// with 11 characters. Most basenames are 2 or 5
			// characters long, such as "fr" and "en_US". This
			// variation in length proves a challenge in aligning
			// the output with tab characters. Hence, a minimal
			// typical columns width of 16 characters is used with
			// always including minimally 1 space character as
			// separater. This specific width has been chosen to
			// allow plenty of space and to support tab width
			// preferences of 2, 4 and 8.
			for (size_t i = d.first.size(); i < 15; ++i)
				cout << ' ';
			cout << ' ' << d.second << '\n';
		}
	}
}

/**
 * Normal loop, tokenize and check spelling.
 *
 * Tokenizes words from @p in by whitespace, checks spelling and outputs
 * result to @p out.
 *
 * @param in the input stream to check spelling for with a word on each line.
 * @param out the output stream to report spelling correctness on the respective
 * lines.
 * @param dic the dictionary to use.
 */
auto normal_loop(istream& in, ostream& out, Dictionary& dic)
{
	auto word = string();
	while (in >> word) {
		auto res = dic.spell(word, in.getloc());
		switch (res) {
		case BAD_WORD:
			out << '&' << '\n';
			break;
		case GOOD_WORD:
			out << '*' << '\n';
			break;
		case AFFIXED_GOOD_WORD:
			out << '+' << '\n';
			break;
		case COMPOUND_GOOD_WORD:
			out << '-' << '\n';
			break;
		}
	}
}

/**
 * Prints misspelled words from an input stream to an output stream.
 *
 * @param in the input stream with a word on each line.
 * @param out the output stream with on each line only misspelled words.
 * @param dic the dictionary to use.
 */
auto misspelled_word_loop(istream& in, ostream& out, Dictionary& dic)
{
	auto word = string();
	while (in >> word) {
		auto res = dic.spell(word, in.getloc());
		if (res == BAD_WORD)
			out << word << '\n';
	}
}

auto correct_word_loop(istream& in, ostream& out, Dictionary& dic)
{
	auto word = string();
	while (in >> word) {
		auto res = dic.spell(word, in.getloc());
		if (res != BAD_WORD)
			out << word << '\n';
	}
}

auto misspelled_line_loop(istream& in, ostream& out, Dictionary& dic)
{
	auto line = string();
	auto words = vector<string>();
	while (getline(in, line)) {
		auto print = false;
		split_on_whitespace_v(line, words, in.getloc());
		for (auto& word : words) {
			auto res = dic.spell(word, in.getloc());
			if (res == BAD_WORD) {
				print = true;
				break;
			}
		}
		if (print)
			out << line << '\n';
	}
}

auto correct_line_loop(istream& in, ostream& out, Dictionary& dic)
{
	auto line = string();
	auto words = vector<string>();
	while (getline(in, line)) {
		auto print = true;
		split_on_whitespace_v(line, words, in.getloc());
		for (auto& word : words) {
			auto res = dic.spell(word, in.getloc());
			if (res == BAD_WORD) {
				print = false;
				break;
			}
		}
		if (print)
			out << line << '\n';
	}
}

namespace std {
ostream& operator<<(ostream& out, const locale& loc)
{
	if (has_facet<boost::locale::info>(loc)) {
		auto& f = use_facet<boost::locale::info>(loc);
		out << "name=" << f.name() << ", lang=" << f.language()
		    << ", country=" << f.country() << ", enc=" << f.encoding();
	}
	else {
		out << loc.name();
	}
	return out;
}
} // namespace std

int main(int argc, char* argv[])
{
	// May speed up I/O. After this, don't use C printf, scanf etc.
	ios_base::sync_with_stdio(false);

	auto args = Args_t(argc, argv);
	if (args.mode == ERROR_MODE) {
		cerr << "Invalid (combination of) arguments, try '"
		     << args.program_name << " --help' for more information\n";
		return 1;
	}
	boost::locale::generator gen;
	auto loc = gen("");
	install_ctype_facets_inplace(loc);
	if (args.encoding.empty()) {
		cin.imbue(loc);
	}
	else {
		try {
			auto loc2 = gen("en_US." + args.encoding);
			install_ctype_facets_inplace(loc2);
			cin.imbue(loc2);
		}
		catch (const boost::locale::conv::invalid_charset_error& e) {
			cerr << e.what() << '\n';
#ifdef _POSIX_VERSION
			cerr << "Nuspell error: see `locale -m` for supported "
			        "encodings.\n";
#endif
			return 1;
		}
	}
	cout.imbue(loc);
	cerr.imbue(loc);
	clog.imbue(loc);
	setlocale(LC_CTYPE, "");

	switch (args.mode) {
	case HELP_MODE:
		print_help(args.program_name);
		return 0;
	case VERSION_MODE:
		print_version();
		return 0;
	default:
		break;
	}
	clog << "INFO: Input  locale " << cin.getloc() << '\n';
	clog << "INFO: Output locale " << cout.getloc() << '\n';

	auto f = Finder();
	f.add_default_paths();
	f.add_libreoffice_paths();
	f.add_mozilla_paths();
	f.add_apacheopenoffice_paths();
	f.search_dictionaries();

	if (args.mode == LIST_DICTIONARIES_MODE) {
		list_dictionaries(f);
		return 0;
	}
	if (args.dictionary.empty()) {
		// infer dictionary from locale
		auto& info = use_facet<boost::locale::info>(loc);
		args.dictionary = info.language();
		auto c = info.country();
		if (!c.empty()) {
			args.dictionary += '_';
			args.dictionary += c;
		}
	}
	auto filename = f.get_dictionary(args.dictionary);
	if (filename.empty()) {
		if (args.dictionary.empty())
			cerr << "No dictionary provided\n";
		else
			cerr << "Dictionary " << args.dictionary
			     << " not found\n";

		return 1;
	}
	clog << "INFO: Pointed dictionary " << filename << ".{dic,aff}\n";
	auto dic = Dictionary();
	try {
		dic = Dictionary::load_from_aff_dic(filename);
	}
	catch (const std::ios_base::failure& e) {
		cerr << e.what() << '\n';
		return 1;
	}
	auto loop_function = normal_loop;
	switch (args.mode) {
	case DEFAULT_MODE:
		// loop_function = normal_loop;
		break;
	case NO_SUGGEST_MODE:
		// This will differ from normal loop once suggestion has been
		// implemented. loop_function = normal_loop;
		break;
	case MISSPELLED_WORDS_MODE:
		loop_function = misspelled_word_loop;
		break;
	case MISSPELLED_LINES_MODE:
		loop_function = misspelled_line_loop;
		break;
	case CORRECT_WORDS_MODE:
		loop_function = correct_word_loop;
		break;
	case CORRECT_LINES_MODE:
		loop_function = correct_line_loop;
		break;
	default:
		break;
	}

	if (args.files.empty()) {
		loop_function(cin, cout, dic);
	}
	else {
		for (auto& file_name : args.files) {
			ifstream in(file_name.c_str());
			if (!in.is_open()) {
				cerr << "Can't open " << file_name << '\n';
				return 1;
			}
			in.imbue(cin.getloc());
			loop_function(in, cout, dic);
		}
	}
	return 0;
}
