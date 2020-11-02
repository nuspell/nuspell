/* Copyright 2016-2020 Dimitrij Mijoski, Sander van Geloven
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

#include "dictionary.hxx"
#include "finder.hxx"

#include <fstream>
#include <iomanip>
#include <iostream>

#include <boost/locale.hpp>

// manually define if not supplied by the build system
#ifndef PROJECT_VERSION
#define PROJECT_VERSION "unknown.version"
#endif
#define PACKAGE_STRING "nuspell " PROJECT_VERSION

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
	MISSPELLED_WORDS_MODE /**< printing only misspelled words */,
	MISSPELLED_LINES_MODE /**< printing only lines with misspelled word(s)*/
	,
	CORRECT_WORDS_MODE /**< printing only correct words */,
	CORRECT_LINES_MODE /**< printing only fully correct lines */,
	LINES_MODE, /**< intermediate mode used while parsing command line
	               arguments, otherwise unused */
	LIST_DICTIONARIES_MODE /**< printing available dictionaries */,
	HELP_MODE /**< printing help information */,
	VERSION_MODE /**< printing version information */,
	ERROR_MODE
};

struct Args_t {
	Mode mode = DEFAULT_MODE;
	bool whitespace_segmentation = false;
	string program_name = "nuspell";
	string dictionary;
	string encoding;
	vector<string> other_dicts;
	vector<string> files;

	Args_t() = default;
	Args_t(int argc, char* argv[]) { parse_args(argc, argv); }
	auto parse_args(int argc, char* argv[]) -> void;
};

/**
 * @brief Parses command line arguments.
 *
 * @param argc command-line argument count.
 * @param argv command-line argument vector.
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
	const char* shortopts = ":d:i:aDGLslhv";
	const struct option longopts[] = {
	    {"version", 0, nullptr, 'v'},
	    {"help", 0, nullptr, 'h'},
	    {nullptr, 0, nullptr, 0},
	};
	while ((c = getopt_long(argc, argv, shortopts, longopts, nullptr)) !=
	       -1) {
		switch (c) {
		case 'a':
			// ispell pipe mode, same as default mode
			if (mode != DEFAULT_MODE)
				mode = ERROR_MODE;
			break;
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
		case 's':
			whitespace_segmentation = true;

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
 * @brief Prints help information to standard output.
 *
 * @param program_name pass argv[0] here.
 */
auto print_help(const string& program_name) -> void
{
	auto& p = program_name;
	auto& o = cout;
	o << "Usage:\n"
	     "\n";
	o << p << " [-s] [-d dict_NAME] [-i enc] [file_name]...\n";
	o << p << " -l|-G [-L] [-s] [-d dict_NAME] [-i enc] [file_name]...\n";
	o << p << " -D|-h|--help|-v|--version\n";
	o << "\n"
	     "Check spelling of each FILE. Without FILE, check standard "
	     "input.\n"
	     "\n"
	     "  -d di_CT      use di_CT dictionary. Only one dictionary at a\n"
	     "                time is currently supported\n"
	     "  -D            print search paths and available dictionaries\n"
	     "                and exit\n"
	     "  -i enc        input/output encoding, default is active locale\n"
	     "  -l            print only misspelled words or lines\n"
	     "  -G            print only correct words or lines\n"
	     "  -L            lines mode\n"
	     "  -s            use simple whitespace text segmentation to\n"
	     "                extract words instead of the default Unicode\n"
	     "                text segmentation. It is not recommended to use\n"
	     "                this.\n"
	     "  -h, --help    print this help and exit\n"
	     "  -v, --version print version number and exit\n"
	     "\n";
	o << "Example: " << p << " -d en_US file.txt\n";
	o << "\n"
	     "Bug reports: <https://github.com/nuspell/nuspell/issues>\n"
	     "Full documentation: "
	     "<https://github.com/nuspell/nuspell/wiki>\n"
	     "Home page: <http://nuspell.github.io/>\n";
}

/**
 * @brief Prints the version number to standard output.
 */
auto print_version() -> void
{
	cout << PACKAGE_STRING
	    "\n"
	    "Copyright (C) 2016-2020 Dimitrij Mijoski and Sander van Geloven\n"
	    "License LGPLv3+: GNU LGPL version 3 or later "
	    "<http://gnu.org/licenses/lgpl.html>.\n"
	    "This is free software: you are free to change and "
	    "redistribute it.\n"
	    "There is NO WARRANTY, to the extent permitted by law.\n"
	    "\n"
	    "Written by Dimitrij Mijoski, Sander van Geloven and others,\n"
	    "see https://github.com/nuspell/nuspell/blob/master/AUTHORS\n";
}

/**
 * @brief Lists dictionary paths and available dictionaries.
 *
 * @param f a finder for search paths and located dictionary.
 */
auto list_dictionaries(const Finder& f) -> void
{
	if (f.get_dir_paths().empty()) {
		cout << "No search paths available" << '\n';
	}
	else {
		cout << "Search paths:" << '\n';
		for (auto& p : f.get_dir_paths()) {
			cout << p << '\n';
		}
	}

	// Even if no search paths are available, still report on available
	// dictionaries.
	if (f.get_dictionaries().empty()) {
		cout << "No dictionaries available\n";
	}
	else {
		cout << "Available dictionaries:\n";
		for (auto& d : f.get_dictionaries()) {
			cout << left << setw(15) << d.first << ' ' << d.second
			     << '\n';
		}
	}
}

auto process_word(
    Mode mode, const Dictionary& dic, const string& line,
    string::const_iterator b, string::const_iterator c, string& word,
    vector<pair<string::const_iterator, string::const_iterator>>& wrong_words,
    vector<string>& suggestions, ostream& out)
{
	word.assign(b, c);
	auto correct = dic.spell(word);
	switch (mode) {
	case DEFAULT_MODE: {
		if (correct) {
			out << "*\n";
			break;
		}
		dic.suggest(word, suggestions);
		auto pos_word = b - begin(line);
		if (suggestions.empty()) {
			out << "# " << word << ' ' << pos_word << '\n';
			break;
		}
		out << "& " << word << ' ' << suggestions.size() << ' '
		    << pos_word << ": ";
		out << suggestions[0];
		for_each(begin(suggestions) + 1, end(suggestions),
		         [&](auto& sug) { out << ", " << sug; });
		out << '\n';
		break;
	}
	case MISSPELLED_WORDS_MODE:
		if (!correct)
			out << word << '\n';
		break;
	case CORRECT_WORDS_MODE:
		if (correct)
			out << word << '\n';
		break;
	case MISSPELLED_LINES_MODE:
	case CORRECT_LINES_MODE:
		if (!correct)
			wrong_words.emplace_back(b, c);
		break;
	default:
		break;
	}
}

auto process_line(
    Mode mode, const string& line,
    const vector<pair<string::const_iterator, string::const_iterator>>&
        wrong_words,
    ostream& out)
{
	switch (mode) {
	case DEFAULT_MODE:
		out << '\n';
		break;
	case MISSPELLED_LINES_MODE:
		if (!wrong_words.empty())
			out << line << '\n';
		break;
	case CORRECT_LINES_MODE:
		if (wrong_words.empty())
			out << line << '\n';
		break;
	default:
		break;
	}
}

auto whitespace_segmentation_loop(istream& in, ostream& out,
                                  const Dictionary& dic, Mode mode)
{
	auto line = string();
	auto word = string();
	auto suggestions = vector<string>();
	using Str_Iter = string::const_iterator;
	auto wrong_words = vector<pair<Str_Iter, Str_Iter>>();
	auto loc = in.getloc();
	auto line_num = size_t(0);
	auto& facet = use_facet<ctype<char>>(loc);
	auto isspace = [&](char c) { return facet.is(facet.space, c); };
	while (getline(in, line)) {
		++line_num;
		wrong_words.clear();
		for (auto a = begin(line); a != end(line);) {
			auto b = find_if_not(a, end(line), isspace);
			if (b == end(line))
				break;
			auto c = find_if(b, end(line), isspace);

			process_word(mode, dic, line, b, c, word, wrong_words,
			             suggestions, out);

			a = c;
		}
		process_line(mode, line, wrong_words, out);
	}
}

auto unicode_segentation_loop(istream& in, ostream& out, const Dictionary& dic,
                              Mode mode)
{
	namespace b = boost::locale::boundary;
	auto line = string();
	auto word = string();
	auto suggestions = vector<string>();
	using Str_Iter = string::const_iterator;
	auto wrong_words = vector<pair<Str_Iter, Str_Iter>>();
	auto loc = in.getloc();
	auto line_num = size_t(0);
	auto index = b::ssegment_index();
	index.rule(b::word_any);
	auto line_stream = istringstream();
	while (getline(in, line)) {
		++line_num;
		index.map(b::word, begin(line), end(line), loc);
		wrong_words.clear();
		auto a = cbegin(line);
		for (auto& segment : index) {
			auto b = begin(segment);
			auto c = end(segment);

			process_word(mode, dic, line, b, c, word, wrong_words,
			             suggestions, out);

			a = c;
		}
		process_line(mode, line, wrong_words, out);
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
	auto loc = std::locale();
	try {
		if (args.encoding.empty())
			loc = gen("");
		else
			loc = gen("en_US." + args.encoding);
	}
	catch (const boost::locale::conv::invalid_charset_error& e) {
		cerr << e.what() << '\n';
#ifdef _POSIX_VERSION
		cerr << "Nuspell error: see `locale -m` for supported "
		        "encodings.\n";
#endif
		return 1;
	}
	cin.imbue(loc);
	cout.imbue(loc);

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
	clog << "INFO: I/O  locale " << loc << '\n';

	auto f = Finder::search_all_dirs_for_dicts();

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
	if (args.dictionary.empty()) {
		cerr << "No dictionary provided and can not infer from OS "
		        "locale\n";
	}
	auto filename = f.get_dictionary_path(args.dictionary);
	if (filename.empty()) {
		cerr << "Dictionary " << args.dictionary << " not found\n";
		return 1;
	}
	clog << "INFO: Pointed dictionary " << filename << ".{dic,aff}\n";
	auto dic = Dictionary();
	try {
		dic = Dictionary::load_from_path(filename);
	}
	catch (const Dictionary_Loading_Error& e) {
		cerr << e.what() << '\n';
		return 1;
	}
	if (!use_facet<boost::locale::info>(loc).utf8())
		dic.imbue(loc);
	auto loop_function = unicode_segentation_loop;
	if (args.whitespace_segmentation)
		loop_function = whitespace_segmentation_loop;
	if (args.files.empty()) {
		loop_function(cin, cout, dic, args.mode);
	}
	else {
		for (auto& file_name : args.files) {
			ifstream in(file_name);
			if (!in.is_open()) {
				cerr << "Can't open " << file_name << '\n';
				return 1;
			}
			in.imbue(loc);
			loop_function(in, cout, dic, args.mode);
		}
	}
	return 0;
}
