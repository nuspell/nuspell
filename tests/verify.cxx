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
	bool quiet = false;
	bool print_sug = false;
	bool print_false = false;
	bool sugs = false;
	string program_name = "verify";
	string dictionary;
	string encoding;
	vector<string> other_dicts;
	vector<string> files;
	string correction;

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
#if defined(_POSIX_VERSION) || defined(__MINGW32__)
	int c;
	// The program can run in various modes depending on the
	// command line options. The mode is FSM state, this while loop is FSM.
	const char* shortopts = ":d:i:c:fspqhv";
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
		case 'c':
			if (correction.empty())
				correction = optarg;
			else
				cerr << "WARNING: Ignoring additional "
				        "suggestions TSV file "
				     << optarg << '\n';
			break;
		case 'f':
			print_false = true;
			break;
		case 's':
			sugs = true;
			break;
		case 'p':
			print_sug = true;
			break;
		case 'q':
			quiet = true;
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
			cerr << "ERROR: Option -" << static_cast<char>(optopt)
			     << " requires an operand\n";
			mode = ERROR_MODE;
			break;
		case '?':
			cerr << "ERROR: Unrecognized option: '-"
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
	o << p
	  << " [-d di_CT] [-i enc] [-c TSV] [-f] [-p] [-s] [-q] [FILE]...\n";
	o << p << " -h|--help|-v|--version\n";
	o << "\n"
	     "Verification testing spell check of each FILE.\n"
	     "\n"
	     "  -d di_CT      use di_CT dictionary. Only one dictionary is\n"
	     "                currently supported\n"
	     "  -i enc        input encoding, default is active locale\n"
	     "  -c TSV        TSV file with corrections to verify suggestions\n"
	     "  -f            print false negative and false positive words\n"
	     "  -s            also test suggestions (usable only in debugger)\n"
	     "  -p            print suggestions\n"
	     "  -q            quiet, supress informative log messages\n"
	     "  -h, --help    print this help and exit\n"
	     "  -v, --version print version number and exit\n"
	     "\n";
	o << "Example: " << p << " -d en_US /usr/share/dict/american-english\n";
	o << "\n"
	     "List available dictionaries: nuspell -D\n"
	     "\n"
	     "Then some statistics for correctness and "
	     "performance are printed to standard output, being:\n"
	     "  Word File\n"
	     "  Total Words Spelling\n"
	     "  Positives Nuspell\n"
	     "  Positives Hunspell\n"
	     "  Negatives Nuspell\n"
	     "  Negatives Hunspell\n"
	     "  True Positives\n"
	     "  True Negatives\n"
	     "  False Positives\n"
	     "  False Negatives\n"
	     "  True Positive Rate\n"
	     "  True Negative Rate\n"
	     "  False Positive Rate\n"
	     "  False Negative Rate\n"
	     "  Total Duration Nuspell\n"
	     "  Total Duration Hunspell\n"
	     "  Minimum Duration Nuspell\n"
	     "  Minimum Duration Hunspell\n"
	     "  Average Duration Nuspell\n"
	     "  Average Duration Hunspell\n"
	     "  Maximum Duration Nuspell\n"
	     "  Maximum Duration Hunspell\n"
	     "  Maximum Speedup\n"
	     "  Accuracy\n"
	     "  Precision\n"
	     "  Speedup\n"
	     "\n"
	     "All durations are in nanoseconds. Even on the same machine, "
	     "timing can vary\n"
	     "considerably in the second significant decimal. Use only a "
	     "production build\n"
	     "executable with optimizations. A speedup of 1.62 means Nuspell "
	     "is 1.6 times\n"
	     "faster than Hunspell.\n"
	     "\n"
	     "Verification will be done on suggestions when a corrections file "
	     "is provided.\n"
	     "Each line in that file contains a unique incorrect word, a tab "
	     "character and\n"
	     "the most desired correct suggestions. Note that the second word "
	     "needs to be\n"
	     "incorrect for Nuspell and Hunspell. The correction should be "
	     "correct for\n"
	     "Nuspell and Hunspell.\n"
	     "\n"
	     "The same statistics as above will be report followed by "
	     "statistics on the\n"
	     "  Total Words Suggestion\n"
	     "  Correction In Suggestions Nuspell\n"
	     "  Correction In Suggestions Hunspell\n"
	     "  Correction In Suggestions Both\n"
	     "  Correction As First Suggestion Nuspell\n"
	     "  Correction As First Suggestion Hunspell\n"
	     "  Correction As First Suggestion Both\n"
	     "  Nuspell More Suggestions\n"
	     "  Hunspell More Suggestions\n"
	     "  Same Number Of Suggestions\n"
	     "  Nuspell No Suggestions\n"
	     "  Hunspell No Suggestions\n"
	     "  Both No Suggestions\n"
	     "  Maximum Suggestions Nuspell\n"
	     "  Maximum Suggestions Hunspell\n"
	     "  Rate Corr. In Suggestions Nuspell\n"
	     "  Rate Corr. In Suggestions Hunspell\n"
	     "  Rate Corr. As First Suggestion Nuspell\n"
	     "  Rate Corr. As First Suggestion Hunspell\n"
	     "  Total Duration Suggestions Nuspell\n"
	     "  Total Duration Suggestions Hunspell\n"
	     "  Minimum Duration Suggestions Nuspell\n"
	     "  Minimum Duration Suggestions Hunspell\n"
	     "  Average Duration Suggestions Nuspell\n"
	     "  Average Duration Suggestions Hunspell\n"
	     "  Maximum Duration Suggestions Nuspell\n"
	     "  Maximum Duration Suggestions Hunspell\n"
	     "  Maximum Suggestions Speedup\n"
	     "  Suggestions Speedup\n";
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

/**
 * @brief Reports spelling check results.
 */
auto spell_report(ostream& out, int total, int true_pos, int true_neg,
                  int false_pos, int false_neg, long duration_nu_tot,
                  long duration_hun_tot, long duration_nu_min,
                  long duration_hun_min, long duration_nu_max,
                  long duration_hun_max, float speedup_max)
{
	// counts
	auto pos_nu = true_pos + false_pos;
	auto pos_hun = true_pos + false_neg;
	auto neg_nu = true_neg + false_neg;
	auto neg_hun = true_neg + false_pos;

	// rates
	auto true_pos_rate = true_pos * 1.0 / total;
	auto true_neg_rate = true_neg * 1.0 / total;
	auto false_pos_rate = false_pos * 1.0 / total;
	auto false_neg_rate = false_neg * 1.0 / total;

	auto accuracy = (true_pos + true_neg) * 1.0 / total;
	auto precision = 0.0;
	if (true_pos + false_pos != 0)
		precision = true_pos * 1.0 / (true_pos + false_pos);
	auto speedup = duration_hun_tot * 1.0 / duration_nu_tot;

	// report
	out << "Total Words Spelling        " << total << '\n';
	out << "Positives Nuspell           " << pos_nu << '\n';
	out << "Positives Hunspell          " << pos_hun << '\n';
	out << "Negatives Nuspell           " << neg_nu << '\n';
	out << "Negatives Hunspell          " << neg_hun << '\n';
	out << "True Positives              " << true_pos << '\n';
	out << "True Negatives              " << true_neg << '\n';
	out << "False Positives             " << false_pos << '\n';
	out << "False Negatives             " << false_neg << '\n';
	out << "True Positive Rate          " << true_pos_rate << '\n';
	out << "True Negative Rate          " << true_neg_rate << '\n';
	out << "False Positive Rate         " << false_pos_rate << '\n';
	out << "False Negative Rate         " << false_neg_rate << '\n';
	out << "Total Duration Nuspell      " << duration_nu_tot << '\n';
	out << "Total Duration Hunspell     " << duration_hun_tot << '\n';
	out << "Minimum Duration Nuspell    " << duration_nu_min << '\n';
	out << "Minimum Duration Hunspell   " << duration_hun_min << '\n';
	out << "Average Duration Nuspell    " << duration_nu_tot / total
	    << '\n';
	out << "Average Duration Hunspell   " << duration_hun_tot / total
	    << '\n';
	out << "Maximum Duration Nuspell    " << duration_nu_max << '\n';
	out << "Maximum Duration Hunspell   " << duration_hun_max << '\n';
	out << "Maximum Speedup             " << speedup_max << '\n';
	out << "Accuracy                    " << accuracy << '\n';
	out << "Precision                   " << precision << '\n';
	out << "Speedup                     " << speedup << '\n';
}

/**
 * @brief Loops through text file with on each line a unique word. The spelling
 * of the words is verified and a report is printed.
 */
auto spell_loop(istream& in, ostream& out, Dictionary& dic, Hunspell& hun,
                locale& hloc, bool print_false = false, bool test_sugs = false)
{
	using chrono::high_resolution_clock;
	using chrono::nanoseconds;

	// verify spelling
	auto total = 0;
	auto true_pos = 0;
	auto true_neg = 0;
	auto false_pos = 0;
	auto false_neg = 0;
	auto duration_nu_tot = nanoseconds();
	auto duration_hun_tot = nanoseconds();
	auto duration_nu_min = nanoseconds(nanoseconds::max());
	auto duration_hun_min = nanoseconds(nanoseconds::max());
	auto duration_nu_max = nanoseconds();
	auto duration_hun_max = nanoseconds();
	auto speedup_max = 0.0;

	// need to take the entire line here, not `in >> word`
	auto in_loc = in.getloc();
	auto word = string();
	while (getline(in, word)) {
		auto tick_a = high_resolution_clock::now();
		auto res_nu = dic.spell(word);
		auto tick_b = high_resolution_clock::now();
		auto res_hun =
		    hun.spell(to_narrow(to_wide(word, in_loc), hloc));
		auto tick_c = high_resolution_clock::now();
		auto duration_nu = tick_b - tick_a;
		auto duration_hun = tick_c - tick_b;

		duration_nu_tot += duration_nu;
		duration_hun_tot += duration_hun;
		if (duration_nu < duration_nu_min)
			duration_nu_min = duration_nu;
		if (duration_hun < duration_hun_min)
			duration_hun_min = duration_hun;
		if (duration_nu > duration_nu_max)
			duration_nu_max = duration_nu;
		if (duration_hun > duration_hun_max)
			duration_hun_max = duration_hun;

		auto speedup = duration_hun * 1.0 / duration_nu;
		if (speedup > speedup_max)
			speedup_max = speedup;

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

		// usable only in debugger
		if (test_sugs && !res_nu && !res_hun) {
			auto sugs_nu = vector<string>();
			dic.suggest(word, sugs_nu);
			auto sugs_hun =
			    hun.suggest(to_narrow(to_wide(word, in_loc), hloc));
		}
	}

	// prevent division by zero
	if (total == 0) {
		cerr << "WARNING: File did not have any content\n";
		return;
	}
	if (duration_nu_tot.count() == 0) {
		cerr
		    << "ERROR: Invalid duration of 0 nanoseconds for Nuspell\n";
		return;
	}
	if (duration_hun_tot.count() == 0) {
		cerr << "ERROR: Invalid duration of 0 nanoseconds for "
		        "Hunspell\n";
		return;
	}

	spell_report(out, total, true_pos, true_neg, false_pos, false_neg,
	             long(duration_nu_tot.count()), long(duration_hun_tot.count()),
	             long(duration_nu_min.count()), long(duration_hun_min.count()),
	             long(duration_nu_max.count()), long(duration_hun_max.count()),
	             float(speedup_max));
}

/**
 * @brief Loops through tab-separated file with on each line a unique
 * incorrectly spelled word and a desired correction. The spelling and
 * suggestions of the words are verified and a report is printed.
 */
auto suggest_loop(istream& in, ostream& out, Dictionary& dic, Hunspell& hun,
                  locale& hloc, bool print_false = false,
                  bool print_sug = false)
{
	using chrono::high_resolution_clock;
	using chrono::nanoseconds;

	// verify spelling
	auto total = 0;
	auto true_pos = 0;
	auto true_neg = 0;
	auto false_pos = 0;
	auto false_neg = 0;
	auto duration_nu_tot = nanoseconds();
	auto duration_hun_tot = nanoseconds();
	auto duration_nu_min = nanoseconds(nanoseconds::max());
	auto duration_hun_min = nanoseconds(nanoseconds::max());
	auto duration_nu_max = nanoseconds();
	auto duration_hun_max = nanoseconds();
	auto speedup_max = 0.0;

	// verify suggestions
	auto sug_total = 0;
	// correction is somewhere in suggestions
	auto sug_in_nu = 0;
	auto sug_in_hun = 0;
	auto sug_in_both = 0;
	// correction is first suggestion
	auto sug_first_nu = 0;
	auto sug_first_hun = 0;
	auto sug_first_both = 0;
	auto sug_same_first = 0;
	// compared number of suggestions
	auto sug_nu_more = 0;
	auto sug_hun_more = 0;
	auto sug_same_amount = 0;
	// no suggestions
	auto sug_nu_none = 0;
	auto sug_hun_none = 0;
	auto sug_both_none = 0;
	// maximum suggestions
	auto sug_nu_max = (size_t)0;
	auto sug_hun_max = (size_t)0;
	// number suggestion at or over maximum
	auto sug_nu_at_max = 0;
	auto sug_hun_at_max = 0;
	auto sug_both_at_max = 0;

	auto sug_duration_nu_tot = nanoseconds();
	auto sug_duration_hun_tot = nanoseconds();
	auto sug_duration_nu_min = nanoseconds(nanoseconds::max());
	auto sug_duration_hun_min = nanoseconds(nanoseconds::max());
	auto sug_duration_nu_max = nanoseconds();
	auto sug_duration_hun_max = nanoseconds();
	auto sug_speedup_max = 0.0;

	auto sug_line = string();
	auto sug_excluded = vector<string>();

	// need to take the entire line here, not `in >> word`
	auto in_loc = in.getloc();
	auto word = string();
	auto correction = string();
	while (getline(in, sug_line)) {
		auto word_correction = vector<string>();
		split_on_any_of(sug_line, "\t", word_correction);
		word = word_correction[0];
		correction = word_correction[1];
		auto tick_a = high_resolution_clock::now();
		auto res_nu = dic.spell(word);
		auto tick_b = high_resolution_clock::now();
		auto res_hun =
		    hun.spell(to_narrow(to_wide(word, in_loc), hloc));
		auto tick_c = high_resolution_clock::now();
		auto duration_nu = tick_b - tick_a;
		auto duration_hun = tick_c - tick_b;

		duration_nu_tot += duration_nu;
		duration_hun_tot += duration_hun;
		if (duration_nu < duration_nu_min)
			duration_nu_min = duration_nu;
		if (duration_hun < duration_hun_min)
			duration_hun_min = duration_hun;
		if (duration_nu > duration_nu_max)
			duration_nu_max = duration_nu;
		if (duration_hun > duration_hun_max)
			duration_hun_max = duration_hun;

		auto speedup = duration_hun * 1.0 / duration_nu;
		if (speedup > speedup_max)
			speedup_max = speedup;

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

		if (res_nu || res_hun) {
			sug_excluded.push_back(word);
			continue;
		}
		// print word and expected suggestion
		if (print_sug)
			out << word << '\t' << correction << '\t';

		auto sugs_nu = vector<string>();
		tick_a = high_resolution_clock::now();
		dic.suggest(word, sugs_nu);
		tick_b = high_resolution_clock::now();
		auto sugs_hun =
		    hun.suggest(to_narrow(to_wide(word, in_loc), hloc));
		tick_c = high_resolution_clock::now();
		auto sug_duration_nu = tick_b - tick_a;
		auto sug_duration_hun = tick_c - tick_b;

		sug_duration_nu_tot += sug_duration_nu;
		sug_duration_hun_tot += sug_duration_hun;
		if (sug_duration_nu < sug_duration_nu_min)
			sug_duration_nu_min = sug_duration_nu;
		if (sug_duration_hun < sug_duration_hun_min)
			sug_duration_hun_min = sug_duration_hun;
		if (sug_duration_nu > sug_duration_nu_max)
			sug_duration_nu_max = sug_duration_nu;
		if (sug_duration_hun > sug_duration_hun_max)
			sug_duration_hun_max = sug_duration_hun;

		auto sug_speedup = sug_duration_hun * 1.0 / sug_duration_nu;
		if (sug_speedup > sug_speedup_max)
			sug_speedup_max = sug_speedup;

		// print number of suggestions
		if (print_sug)
			out << sug_duration_nu.count() << '\t'
			    << sug_duration_hun.count() << '\n';

		// correction is somewhere in suggestions
		auto in_nu = false;
		for (auto& sug : sugs_nu) {
			if (sug == correction) {
				++sug_in_nu;
				in_nu = true;
				break;
			}
		}
		for (auto& sug : sugs_hun) {
			if (sug == correction) {
				++sug_in_hun;
				if (in_nu)
					++sug_in_both;
				break;
			}
		}

		// correction is first suggestion
		auto first_nu = false;
		if (!sugs_nu.empty() && sugs_nu[0] == correction) {
			++sug_first_nu;
			first_nu = true;
		}
		if (!sugs_hun.empty() && sugs_hun[0] == correction) {
			++sug_first_hun;
			if (first_nu)
				++sug_first_both;
		}

		// same first suggestion, regardless of desired correction
		if (!sugs_nu.empty() && !sugs_hun.empty() &&
		    sugs_nu[0] == sugs_hun[0])
			++sug_same_first;

		// compared number of suggestions
		if (sugs_nu.size() == sugs_hun.size())
			++sug_same_amount;
		if (sugs_nu.size() > sugs_hun.size())
			++sug_nu_more;
		if (sugs_hun.size() > sugs_nu.size())
			++sug_hun_more;

		// no suggestions
		if (sugs_nu.empty()) {
			++sug_nu_none;
			if (sugs_hun.empty())
				++sug_both_none;
		}
		if (sugs_hun.empty())
			++sug_hun_none;

		// maximum suggestions
		if (sugs_nu.size() > sug_nu_max)
			sug_nu_max = sugs_nu.size();
		if (sugs_hun.size() > sug_hun_max)
			sug_hun_max = sugs_hun.size();

		// number suggestion at or over maximum
		if (sugs_nu.size() >= 15) {
			++sug_nu_at_max;
			if (sugs_hun.size() >= 15)
				++sug_both_at_max;
		}
		if (sugs_hun.size() >= 15)
			++sug_hun_at_max;

		++sug_total;
	}

	// prevent division by zero
	if (total == 0) {
		cerr << "WARNING: No input was provided\n";
		return;
	}
	if (duration_nu_tot.count() == 0) {
		cerr
		    << "ERROR: Invalid duration of 0 nanoseconds for Nuspell\n";
		return;
	}
	if (duration_hun_tot.count() == 0) {
		cerr << "ERROR: Invalid duration of 0 nanoseconds for "
		        "Hunspell\n";
		return;
	}

	spell_report(out, total, true_pos, true_neg, false_pos, false_neg,
	             long(duration_nu_tot.count()), long(duration_hun_tot.count()),
	             long(duration_nu_min.count()), long(duration_hun_min.count()),
	             long(duration_nu_max.count()), long(duration_hun_max.count()),
	             float(speedup_max));

	// prevent division by zero
	if (sug_total == 0) {
		cerr << "WARNING: No input for suggestions was provided\n";
		return;
	}
	if (sug_duration_nu_tot.count() == 0) {
		cerr << "ERROR: Invalid duration of 0 nanoseconds for Nuspell "
		        "suggestions\n";
		return;
	}
	if (sug_duration_hun_tot.count() == 0) {
		cerr << "ERROR: Invalid duration of 0 nanoseconds for Hunspell "
		        "suggestions\n";
		return;
	}

	// rates
	auto sug_in_nu_rate = sug_in_nu * 1.0 / sug_total;
	auto sug_in_hun_rate = sug_in_hun * 1.0 / sug_total;
	auto sug_first_nu_rate = sug_first_nu * 1.0 / sug_total;
	auto sug_first_hun_rate = sug_first_hun * 1.0 / sug_total;

	auto sug_speedup =
	    sug_duration_hun_tot.count() * 1.0 / sug_duration_nu_tot.count();

	// suggestion report
	out << "Total Words Suggestion                  " << sug_total << '\n';
	out << "Correction In Suggestions Nuspell       " << sug_in_nu << '\n';
	out << "Correction In Suggestions Hunspell      " << sug_in_hun << '\n';
	out << "Correction In Suggestions Both          " << sug_in_both
	    << '\n';

	out << "Correction As First Suggestion Nuspell  " << sug_first_nu
	    << '\n';
	out << "Correction As First Suggestion Hunspell " << sug_first_hun
	    << '\n';
	out << "Correction As First Suggestion Both     " << sug_first_both
	    << '\n';

	out << "Nuspell More Suggestions                " << sug_nu_more
	    << '\n';
	out << "Hunspell More Suggestions               " << sug_hun_more
	    << '\n';
	out << "Same Number Of Suggestions              " << sug_in_hun << '\n';

	out << "Nuspell No Suggestions                  " << sug_nu_none
	    << '\n';
	out << "Hunspell No Suggestions                 " << sug_hun_none
	    << '\n';
	out << "Both No Suggestions                     " << sug_both_none
	    << '\n';

	out << "Maximum Suggestions Nuspell             " << sug_nu_max << '\n';
	out << "Maximum Suggestions Hunspell            " << sug_hun_max
	    << '\n';

	out << "Rate Corr. In Suggestions Nuspell       " << sug_in_nu_rate
	    << '\n';
	out << "Rate Corr. In Suggestions Hunspell      " << sug_in_hun_rate
	    << '\n';

	out << "Rate Corr. As First Suggestion Nuspell  " << sug_first_nu_rate
	    << '\n';
	out << "Rate Corr. As First Suggestion Hunspell " << sug_first_hun_rate
	    << '\n';

	out << "Total Duration Suggestions Nuspell      "
	    << sug_duration_nu_tot.count() << '\n';
	out << "Total Duration Suggestions Hunspell     "
	    << sug_duration_hun_tot.count() << '\n';
	out << "Minimum Duration Suggestions Nuspell    "
	    << sug_duration_nu_min.count() << '\n';
	out << "Minimum Duration Suggestions Hunspell   "
	    << sug_duration_hun_min.count() << '\n';
	out << "Average Duration Suggestions Nuspell    "
	    << sug_duration_nu_tot.count() / sug_total << '\n';
	out << "Average Duration Suggestions Hunspell   "
	    << sug_duration_hun_tot.count() / sug_total << '\n';
	out << "Maximum Duration Suggestions Nuspell    "
	    << sug_duration_nu_max.count() << '\n';
	out << "Maximum Duration Suggestions Hunspell   "
	    << sug_duration_hun_max.count() << '\n';
	out << "Maximum Suggestions Speedup             " << sug_speedup_max
	    << '\n';
	out << "Suggestions Speedup                     " << sug_speedup
	    << '\n';

	if (!sug_excluded.empty()) {
		out << "The following words are correct and should not be "
		       "used:\n";
		for (auto& excl : sug_excluded)
			out << excl << '\n';
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
		cerr << "ERROR: See `locale -m` for supported "
		        "encodings.\n";
#endif
		return 1;
	}
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
	if (!args.quiet)
		clog << "INFO: I/O locale " << loc << '\n';

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
	if (args.dictionary.empty())
		cerr << "No dictionary provided and can not infer from OS "
		        "locale\n";
	auto filename = f.get_dictionary_path(args.dictionary);
	if (filename.empty()) {
		cerr << "Dictionary " << args.dictionary << " not found\n";
		return 1;
	}
	if (!args.quiet)
		clog << "INFO: Pointed dictionary " << filename
		     << ".{dic,aff}\n";
	auto dic = Dictionary();
	try {
		dic = Dictionary::load_from_path(filename);
	}
	catch (const Dictionary_Loading_Error& e) {
		cerr << e.what() << '\n';
		return 1;
	}
	dic.imbue(loc);

	auto aff_name = filename + ".aff";
	auto dic_name = filename + ".dic";
	Hunspell hun(aff_name.c_str(), dic_name.c_str());
	auto hun_loc = gen(
	    "en_US." + Encoding(hun.get_dict_encoding()).value_or_default());

	for (auto& file_name : args.files) {
		ifstream in(file_name);
		if (!in.is_open()) {
			cerr << "Can't open " << file_name << '\n';
			return 1;
		}
		in.imbue(loc);
		cout << "Word File                   " << file_name << '\n';
		spell_loop(in, cout, dic, hun, hun_loc, args.print_false,
		           args.sugs);
	}
	if (!args.correction.empty()) {
		ifstream in(args.correction);
		if (!in.is_open()) {
			cerr << "Can't open " << args.correction << '\n';
			return 1;
		}
		in.imbue(loc);
		cout << "Correction File             " << args.correction
		     << '\n';
		suggest_loop(in, cout, dic, hun, hun_loc, args.print_false,
		             args.print_sug);
	}
	return 0;
}
