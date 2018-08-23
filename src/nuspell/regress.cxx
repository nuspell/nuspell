/* Copyright 2018 Dimitrij Mijoski and Sander van Geloven
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
 * @file regress.cxx
 * Command line spell check regression testing.
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
#define PACKAGE_STRING "regress 2.0.0"
#endif
#ifndef PACKAGE
#define PACKAGE "regress"
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
	DEFAULT_MODE /**< regression test */,
	HELP_MODE /**< printing help information */,
	VERSION_MODE /**< printing version information */,
	ERROR_MODE /**< where the arguments used caused an error */
};

struct Args_t {
	Mode mode = DEFAULT_MODE;
	string program_name = PACKAGE; // ignore warning padding struct
	string dictionary;
	string encoding;
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
	const char* shortopts = ":d:i:hv";
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
	o << p << " -h|--help|-v|--version\n";
	o << "\n"
	     "Regression testing spell check of each FILE. Without FILE, check "
	     "standard "
	     "input.\n"
	     "For simple test, use /usr/share/dict/american-english for FILE.\n"
	     "\n"
	     "  -d di_CT      use di_CT dictionary. Only one dictionary is\n"
	     "                currently supported\n"
	     "  -i enc        input encoding, default is active locale\n"
	     "  -h, --help    display this help and exit\n"
	     "  -v, --version print version number and exit\n"
	     "\n";
	o << "Example: " << p << " -d en_US file.txt\n";
	o << "\n"
	     "All word for which resutls differ with Hunspell are printed\n"
	     "standard output. At the end of each presented file, space-\n"
	     "separated statistics are printed to standard output, being:\n"
	     "  total number of words [0,1,..]\n"
	     "  total true positives  [0,1,..]\n"
	     "  true positive rate    [0.000,..,1.000]\n"
	     "  total true negatives  [0,1,..]\n"
	     "  true negative rate    [0.000,..,1.000]\n"
	     "  total false positives [0,1,..]\n"
	     "  false positive rate   [0.000,..,1.000]\n"
	     "  total false negatives [0,1,..]\n"
	     "  false negative rate   [0.000,..,1.000]\n"
	     "  accuracy rate         [0.000,..,1.000]\n"
	     "  precision rate        [0.000,..,1.000]\n"
	     "  duration Nuspell      [0,1,..] nanoseconds\n"
	     "  duration Hunspell     [0,1,..] nanoseconds\n"
	     "  speedup rate          [0.000,..,9.999]\n"
	     "All durations are highly machine and platform dependent.\n"
	     "If speedup is 0.600, Nuspell uses 60% of the time of Hunspell.\n"
	     "Use only executable from production build with optimizations.\n"
	     "The last line contains a summary for easy Nuspell performance\n"
	     "reporting only. It contains, space-separated, the following:\n"
	     "  total number of words\n"
	     "  total true positives\n"
	     "  true positive rate\n"
	     "  total true negatives\n"
	     "  true negative rate\n"
	     "  total false positives\n"
	     "  false positive rate\n"
	     "  total false negatives\n"
	     "  false negative rate\n"
	     "  accuracy rate\n"
	     "  precision rate\n"
	     "  duration Nuspell\n"
	     "  speedup rate\n";
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
	    "Copyright (C) 2018 Dimitrij Mijoski and Sander van Geloven\n"
	    "License LGPLv3+: GNU LGPL version 3 or later "
	    "<http://gnu.org/licenses/lgpl.html>.\n"
	    "This is free software: you are free to change and "
	    "redistribute it.\n"
	    "There is NO WARRANTY, to the extent permitted by law.\n"
	    "\n"
	    "Written by Dimitrij Mijoski, Sander van Geloven and others,\n"
	    "see https://github.com/hunspell/nuspell/blob/master/AUTHORS\n";
}

auto normal_loop(istream& in, ostream& out, Dictionary& dic, Hunspell& hun,
                 locale& hloc)
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
	while (in >> word) {
		auto tick_a = chrono::high_resolution_clock::now();
		auto res = dic.spell(word);
		auto tick_b = chrono::high_resolution_clock::now();
		auto hres = hun.spell(to_narrow(to_wide(word, in_loc), hloc));
		auto tick_c = chrono::high_resolution_clock::now();
		duration_nu += tick_b - tick_a;
		duration_hun += tick_c - tick_b;
		if (hres) {
			if (res) {
				++true_pos;
			}
			else {
				++false_neg;
				out << "FalseNegativeWord   " << word << '\n';
			}
		}
		else {
			if (res) {
				++false_pos;
				out << "FalsePositiveWord   " << word << '\n';
			}
			else {
				++true_neg;
			}
		}
		++total;
	}
	// prevent devision by zero
	if (total == 0) {
		out << total << endl;
		return;
	}
	// rates
	auto speedup =
	    static_cast<float>(duration_nu.count()) / duration_hun.count();
	auto trueposr = static_cast<float>(true_pos) / total;
	auto truenegr = static_cast<float>(true_neg) / total;
	auto falseposr = static_cast<float>(false_pos) / total;
	auto falsenegr = static_cast<float>(false_neg) / total;
	auto accuracy = static_cast<float>(true_pos + true_neg) / total;
	auto precision = static_cast<float>(true_pos) / (true_pos + false_pos);

	out.precision(3);
	out << "TotalWords          " << total << '\n';
	out << "TruePositives       " << true_pos << '\n';
	out << "TruePositiveRate    " << fixed << trueposr << '\n';
	out << "TrueNegatives       " << true_neg << '\n';
	out << "TrueNegativeRate    " << fixed << truenegr << '\n';
	out << "FalsePositives      " << false_pos << '\n';
	out << "FalsePositiveRate   " << fixed << falseposr << '\n';
	out << "FalseNegatives      " << false_neg << '\n';
	out << "FalseNegativeRate   " << fixed << falsenegr << '\n';
	out << "AccuracyRate        " << fixed << accuracy << '\n';
	out << "PrecisionRate       " << fixed << precision << '\n';
	out << "DurationNuspell     " << duration_nu.count() << '\n';
	out << "DurationHunspell    " << duration_hun.count() << '\n';
	out << "SpeedupRate         " << fixed << speedup << '\n';

	// summarey for easy reporting
	out << fixed << total << ' ' << true_pos << ' ' << trueposr << ' '
	    << true_neg << ' ' << truenegr << ' ' << false_pos << ' '
	    << falseposr << ' ' << false_neg << ' ' << falsenegr << ' '
	    << accuracy << ' ' << precision << ' ' << duration_nu.count() << ' '
	    << speedup << endl;
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
		loop_function(cin, cout, dic, hun, hun_loc);
	}
	else {
		for (auto& file_name : args.files) {
			ifstream in(file_name.c_str());
			if (!in.is_open()) {
				cerr << "Can't open " << file_name << '\n';
				return 1;
			}
			in.imbue(cin.getloc());
			loop_function(in, cout, dic, hun, hun_loc);
		}
	}
	return 0;
}
