/* Copyright 2018-2020 Dimitrij Mijoski, Sander van Geloven
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
#include <nuspell/finder.hxx>
#include <nuspell/utils.hxx>

#include <chrono>
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

#include <hunspell/hunspell.hxx>

using namespace std;
using namespace nuspell;

enum Mode {
	DEFAULT_MODE /**< verification test */,
	HELP_MODE /**< printing help information */,
	VERSION_MODE /**< printing version information */,
	ERROR_MODE /**< where the arguments used caused an error */
};

struct Args_t {
	Mode mode = DEFAULT_MODE;
	string program_name = "verify";
	string dictionary;
	string encoding;
	vector<string> other_dicts;
	vector<string> files;
	bool print_false = false;
	bool sugs = false;

	Args_t() = default;
	Args_t(int argc, char* argv[]) { parse_args(argc, argv); }
	auto parse_args(int argc, char* argv[]) -> void;
};

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
	const char* shortopts = ":d:i:fshv";
	const struct option longopts[] = {
	    {"version", 0, nullptr, 'v'},
	    {"help", 0, nullptr, 'h'},
	    {nullptr, 0, nullptr, 0},
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
		case 'f':
			print_false = true;

			break;
		case 's':
			sugs = true;
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
	o << p << " [-d dict_NAME] [-i enc] [-f] [-s] [file_name]...\n";
	o << p << " -h|--help|-v|--version\n";
	o << "\n"
	     "Verification testing spell check of each FILE.\n"
	     "Without FILE, check standard input.\n"
	     "For simple test, use /usr/share/dict/american-english for FILE.\n"
	     "\n"
	     "  -d di_CT      use di_CT dictionary. Only one dictionary is\n"
	     "                currently supported\n"
	     "  -i enc        input encoding, default is active locale\n"
	     "  -f            print false negative and false positive words\n"
	     "  -s            also test suggestions (usable only in debugger)\n"
	     "  -h, --help    print this help and exit\n"
	     "  -v, --version print version number and exit\n"
	     "\n";
	o << "Example: " << p << " -d en_US file.txt\n";
	o << "\n"
	     "All word for which results differ with Hunspell are printed to"
	     "standard output.\n"
	     "Then some statistics for correctness and "
	     "performance are printed to standard output, being:\n"
	     "  Total Words\n"
	     "  True Positives\n"
	     "  True Negatives\n"
	     "  False Positives\n"
	     "  False Negatives\n"
	     "  Accuracy\n"
	     "  Precision\n"
	     "  Duration Nuspell (type varies, but usually in nanoseconds)\n"
	     "  Duration Hunspell (type varies, but usually in nanoseconds)\n"
	     "  Speedup Rate\n"
	     "All durations are highly machine and platform dependent.\n"
	     "Even on the same machine it varies a lot in the second decimal!\n"
	     "If speedup is 1.60, Nuspell is 1.60 times faster as Hunspell.\n"
	     "Use only executable from production build with optimizations.\n"
	     "\n"
	     "Please note, messages containing:\n"
	     "  This UTF-8 encoding can't convert to UTF-16:"
	     "are caused by Hunspell and can be ignored.\n";
}

/**
 * @brief Prints the version number to standard output.
 */
auto print_version() -> void
{
	cout << PACKAGE_STRING
	    "\n"
	    "Copyright (C) 2018-2020 Dimitrij Mijoski and Sander van Geloven\n"
	    "License LGPLv3+: GNU LGPL version 3 or later "
	    "<http://gnu.org/licenses/lgpl.html>.\n"
	    "This is free software: you are free to change and "
	    "redistribute it.\n"
	    "There is NO WARRANTY, to the extent permitted by law.\n"
	    "\n"
	    "Written by Dimitrij Mijoski, Sander van Geloven and others,\n"
	    "see https://github.com/nuspell/nuspell/blob/master/AUTHORS\n";
}

auto normal_loop(istream& in, ostream& out, Dictionary& dic, Hunspell& hun,
                 locale& hloc, bool print_false = false, bool test_sugs = false)
{
	auto word = string();
	auto wide_word = wstring();
	auto narrow_word = string();
	// total number of words
	auto total = 0;
	// total number of words with identical spelling correctness
	auto true_pos = 0;
	auto true_neg = 0;
	auto false_pos = 0;
	auto false_neg = 0;
	// store cpu time for Hunspell and Nuspell
	auto duration_hun = chrono::high_resolution_clock::duration();
	auto duration_nu = duration_hun;
	auto in_loc = in.getloc();
	// need to take entine line here, not `in >> word`
	while (getline(in, word)) {
		auto tick_a = chrono::high_resolution_clock::now();
		auto res_nu = dic.spell(word);
		auto tick_b = chrono::high_resolution_clock::now();
		to_wide(word, in_loc, wide_word);
		to_narrow(wide_word, narrow_word, hloc);
		auto res_hun = hun.spell(narrow_word);
		auto tick_c = chrono::high_resolution_clock::now();
		duration_nu += tick_b - tick_a;
		duration_hun += tick_c - tick_b;
		if (res_hun) {
			if (res_nu) {
				++true_pos;
			}
			else {
				++false_neg;
				if (print_false)
					out << "FalseNegativeWord   " << word
					    << '\n';
			}
		}
		else {
			if (res_nu) {
				++false_pos;
				if (print_false)
					out << "FalsePositiveWord   " << word
					    << '\n';
			}
			else {
				++true_neg;
			}
		}
		++total;
		if (test_sugs && !res_nu && !res_hun) {
			auto nus_sugs = vector<string>();
			auto hun_sugs = vector<string>();
			dic.suggest(word, nus_sugs);
			hun.suggest(narrow_word);
		}
	}

	// prevent devision by zero
	if (total == 0) {
		out << total << endl;
		return;
	}
	if (duration_nu.count() == 0) {
		cerr << "Invalid duration of 0 nanoseconds for Nuspell\n";
		out << total << endl;
		return;
	}
	// rates
	auto accuracy = (true_pos + true_neg) * 1.0 / total;
	auto precision = true_pos * 1.0 / (true_pos + false_pos);
	auto speedup = duration_hun.count() * 1.0 / duration_nu.count();

	out << "Total Words         " << total << '\n';
	out << "True Positives      " << true_pos << '\n';
	out << "True Negatives      " << true_neg << '\n';
	out << "False Positives     " << false_pos << '\n';
	out << "False Negatives     " << false_neg << '\n';
	out << "Accuracy            " << accuracy << '\n';
	out << "Precision           " << precision << '\n';
	out << "Duration Nuspell    " << duration_nu.count() << '\n';
	out << "Duration Hunspell   " << duration_hun.count() << '\n';
	out << "Speedup Rate        " << speedup << '\n';
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

	auto aff_name = filename + ".aff";
	auto dic_name = filename + ".dic";
	Hunspell hun(aff_name.c_str(), dic_name.c_str());
	auto hun_loc = gen(
	    "en_US." + Encoding(hun.get_dict_encoding()).value_or_default());

	if (args.files.empty()) {
		normal_loop(cin, cout, dic, hun, hun_loc, args.print_false,
		            args.sugs);
	}
	else {
		for (auto& file_name : args.files) {
			ifstream in(file_name);
			if (!in.is_open()) {
				cerr << "Can't open " << file_name << '\n';
				return 1;
			}
			in.imbue(cin.getloc());
			normal_loop(in, cout, dic, hun, hun_loc,
			            args.print_false);
		}
	}
	return 0;
}
