/* Copyright 2018 Dimitrij Mijoski, Sander van Geloven
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
 * @file verify.cxx
 * Command line spell check for verification testing.
 */

#include "dictionary.hxx"
#include "finder.hxx"
#include "string_utils.hxx"

#include <chrono>
#include <clocale>
#include <fstream>
#include <iomanip>
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
#define PACKAGE_STRING "verify 2.0.0"
#endif
#ifndef PACKAGE
#define PACKAGE "verify"
#endif
#endif

#if defined(__MINGW32__) || defined(__unix__) || defined(__unix) ||            \
    (defined(__APPLE__) && defined(__MACH__))
#include <getopt.h>
#include <unistd.h>
#endif

#include "../hunspell/hunspell.hxx"

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
	string program_name = PACKAGE; // ignore warning padding struct
	string dictionary;
	string encoding;
	bool print_false = false;
	vector<string> other_dicts;
	vector<string> files;

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
	const char* shortopts = ":d:i:Fhv";
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
		case 'F':
			print_false = true;

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
 * Prints help information to standard output.
 *
 * @param program_name pass argv[0] here.
 */
auto print_help(const string& program_name) -> void
{
	auto& p = program_name;
	auto& o = cout;
	o << "Usage:\n"
	     "\n";
	o << p << " [-d dict_NAME] [-i enc] [file_name]...\n";
	o << p << " -h|--help|-v|--version\n";
	o << "\n"
	     "Verification testing spell check of each FILE. Without FILE, "
	     "check "
	     "standard "
	     "input.\n"
	     "For simple test, use /usr/share/dict/american-english for FILE.\n"
	     "\n"
	     "  -d di_CT      use di_CT dictionary. Only one dictionary is\n"
	     "                currently supported\n"
	     "  -i enc        input encoding, default is active locale\n"
	     "  -F            print false negative and false positive words\n"
	     "  -h, --help    print this help and exit\n"
	     "  -v, --version print version number and exit\n"
	     "\n";
	o << "Example: " << p << " -d en_US file.txt\n";
	o << "\n"
	     "All word for which results differ with Hunspell are printed\n"
	     "standard output. At the end of each presented file, space-\n"
	     "separated statistics are printed to standard output, being:\n"
	     "  Total Words         [0,1,..]\n"
	     "  Positives Hunspell  [0,1,..]\n"
	     "  Negatives Hunspell  [0,1,..]\n"
	     "  Positives Nuspell   [0,1,..]\n"
	     "  Negatives Nuspell   [0,1,..]\n"
	     "  True Positives      [0,1,..]\n"
	     "  True Positive Rate  [0.000,..,1.000]\n"
	     "  True Negatives      [0,1,..]\n"
	     "  True Negative Rate  [0.000,..,1.000]\n"
	     "  False Positives     [0,1,..]\n"
	     "  False Positive Rate [0.000,..,1.000]\n"
	     "  False Negatives     [0,1,..]\n"
	     "  False Negative Rate [0.000,..,1.000]\n"
	     "  Accuracy Rate       [0.000,..,1.000]\n"
	     "  Precision Rate      [0.000,..,1.000]\n"
	     "  Duration Nuspell    [0,1,..] nanoseconds\n"
	     "  Duration Hunspell   [0,1,..] nanoseconds\n"
	     "  Speedup Rate        [0.00,..,9.99]\n"
	     "All durations are highly machine and platform dependent.\n"
	     "Even on the same machine it varies a lot in the second decimal!\n"
	     "If speedup is 1.60, Nuspell is 1.60 times faster as Hunspell.\n"
	     "Use only executable from production build with optimizations.\n"
	     "The last line contains a summary for easy Nuspell performance\n"
	     "reporting only. It contains, space-separated, the following:\n"
	     "  Total Words\n"
	     "  True Positives\n"
	     "  True Positive Rate\n"
	     "  True Negatives\n"
	     "  True Negative Rate\n"
	     "  False Positives\n"
	     "  False Positive Rate\n"
	     "  False Negatives\n"
	     "  False Negative Rate\n"
	     "  Accuracy Rate\n"
	     "  Precision Rate\n"
	     "  Duration Nuspell\n"
	     "  Speedup Rate\n"
	     "\n"
	     "Please note, messages containing:\n"
	     "  This UTF-8 encoding can't convert to UTF-16:"
	     "are caused by Hunspell and can be ignored.\n";
	o << "\n"
	     "Bug reports: <https://github.com/nuspell/nuspell/issues>\n"
	     "Full documentation: "
	     "<https://github.com/nuspell/nuspell/wiki>\n"
	     "Home page: <http://nuspell.github.io/>\n";
}

/**
 * Prints the version number to standard output.
 */
auto print_version() -> void
{
	cout << PACKAGE_STRING
	    "\n"
	    "Copyright (C) 2018 Dimitrij Mijoski and Sander van Geloven\n"
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
                 locale& hloc, bool print_false = false)
{
	auto word = string();
	// total number of words
	auto total = 0;
	// total number of words with identical spelling correctness
	auto true_pos = 0;
	auto true_neg = 0;
	auto false_pos = 0;
	auto false_neg = 0;
	// store cpu time for Hunspell and Nuspell
	auto duration_hun = chrono::nanoseconds();
	auto duration_nu = chrono::nanoseconds();
	auto in_loc = in.getloc();
	// need to take entine line here, not `in >> word`
	while (getline(in, word)) {
		auto tick_a = chrono::high_resolution_clock::now();
		auto res_nu = dic.spell(word);
		auto tick_b = chrono::high_resolution_clock::now();
		auto res_hun =
		    hun.spell(to_narrow(to_wide(word, in_loc), hloc));
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
	}
	auto pos_nu = true_pos + false_pos;
	auto pos_hun = true_pos + false_neg;
	auto neg_nu = true_neg + false_neg;
	auto neg_hun = true_neg + false_pos;

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
	auto true_pos_rate = true_pos * 1.0 / total;
	auto true_neg_rate = true_neg * 1.0 / total;
	auto false_pos_rate = false_pos * 1.0 / total;
	auto false_neg_rate = false_neg * 1.0 / total;
	auto accuracy = (true_pos + true_neg) * 1.0 / total;
	auto precision = true_pos * 1.0 / (true_pos + false_pos);
	auto speedup = duration_hun.count() * 1.0 / duration_nu.count();

	out.precision(3);
	out << "Total Words         " << total << '\n';
	out << "Positives Nuspell   " << pos_nu << '\n';
	out << "Positives Hunspell  " << pos_hun << '\n';
	out << "Negatives Nuspell   " << neg_nu << '\n';
	out << "Negatives Hunspell  " << neg_hun << '\n';
	out << "True Positives      " << true_pos << '\n';
	out << "True Positive Rate  " << true_pos_rate << '\n';
	out << "True Negatives      " << true_neg << '\n';
	out << "True Negative Rate  " << true_neg_rate << '\n';
	out << "False Positives     " << false_pos << '\n';
	out << "False Positive Rate " << false_pos_rate << '\n';
	out << "False Negatives     " << false_neg << '\n';
	out << "False Negative Rate " << false_neg_rate << '\n';
	out << "Accuracy Rate       " << accuracy << '\n';
	out << "Precision Rate      " << precision << '\n';
	out << "Duration Nuspell    " << duration_nu.count() << '\n';
	out << "Duration Hunspell   " << duration_hun.count() << '\n';
	out.precision(2); // otherwise too much variation
	out << "Speedup Rate        " << speedup << '\n';
	out.precision(3);

	// summarey for easy reporting
	out << fixed << total << ' ' << true_pos << ' ' << true_pos_rate << ' '
	    << true_neg << ' ' << true_neg_rate << ' ' << false_pos << ' '
	    << false_pos_rate << ' ' << false_neg << ' ' << false_neg_rate
	    << ' ' << accuracy << ' ' << precision << ' ' << duration_nu.count()
	    << ' ' << setprecision(2) << speedup << endl;
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
	dic.imbue(cin.getloc());

	auto aff_name = filename + ".aff";
	auto dic_name = filename + ".dic";
	Hunspell hun(aff_name.c_str(), dic_name.c_str());
	auto hun_loc = gen(
	    "en_US." + Encoding(hun.get_dict_encoding()).value_or_default());
	auto loop_function = normal_loop;

	if (args.files.empty()) {
		loop_function(cin, cout, dic, hun, hun_loc, args.print_false);
	}
	else {
		for (auto& file_name : args.files) {
			ifstream in(file_name.c_str());
			if (!in.is_open()) {
				cerr << "Can't open " << file_name << '\n';
				return 1;
			}
			in.imbue(cin.getloc());
			loop_function(in, cout, dic, hun, hun_loc,
			              args.print_false);
		}
	}
	return 0;
}
