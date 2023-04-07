/* Copyright 2016-2023 Dimitrij Mijoski
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

#include <hunspell/hunspell.hxx>
#include <nuspell/dictionary.hxx>
#include <nuspell/finder.hxx>

#include <chrono>
#include <clocale>
#include <fstream>
#include <iostream>
#include <unicode/ucnv.h>

#include <getopt.h>

#if __has_include(<unistd.h>)
#include <unistd.h> // defines _POSIX_VERSION
#endif
#ifdef _POSIX_VERSION
#include <langinfo.h>
#include <sys/resource.h>
#include <sys/time.h>
#endif
#ifdef _WIN32
#include <io.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <psapi.h>
#endif

// manually define if not supplied by the build system
#ifndef PROJECT_VERSION
#define PROJECT_VERSION "unknown.version"
#endif

using namespace std;
using nuspell::Dictionary, nuspell::Dictionary_Loading_Error,
    nuspell::Dict_Finder_For_CLI_Tool_2;
namespace {
enum Mode { NORMAL, HELP, VERSION };
auto print_help(const char* program_name) -> void
{
	auto p = string_view(program_name);
	auto& o = cout;
	o << "Usage:\n"
	  << p << " [-d dict_NAME] [OPTION]... [FILE...]\n"
	  << p << " --help|--version\n"
	  << R"(
Check spelling of each FILE, and measure speed and correctness in regard to
other spellchecking libraries. If no FILE is specified, check standard input.
The input text should be a simple wordlist with one word per line.

  -d, --dictionary=di_CT    use di_CT dictionary (only one is supported)
  -m, --print-mismatches    print mismatches (false positives and negatives)
  -s, --test-suggestions    call suggest function (useful only for debugging)
  -w, --wikipedia-list      input is Wikipedia's list of common misspellings
  --encoding=enc            set both input and output encoding
  --input-encoding=enc      input encoding, default is active locale
  --output-encoding=enc     (UNUSED, always UTF-8) output encoding, default is
                            active locale
  --help                    print this help
  --version                 print version number

The following environment variables can have effect:

  DICTIONARY - same as -d,
  DICPATH    - additional directory path to search for dictionaries.

Example:
)"
	  << "    " << p << " -d en_US file.txt\n"
	  << "    " << p << " -d ../../subdir/di_CT.aff\n";
}

auto ver_str = "nuspell " PROJECT_VERSION R"(
Copyright (C) 2016-2023 Dimitrij Mijoski
License LGPLv3+: GNU LGPL version 3 or later <http://gnu.org/licenses/lgpl.html>.
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.

Written by Dimitrij Mijoski.
)";

auto print_version() -> void { cout << ver_str; }

auto get_peak_ram_usage() -> long
{
#ifdef _POSIX_VERSION
	rusage r;
	auto err = getrusage(RUSAGE_SELF, &r);
	if (!err)
		return r.ru_maxrss;
#elif _WIN32
	PROCESS_MEMORY_COUNTERS pmc;
	auto suc = GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
	if (suc)
		return pmc.PeakWorkingSetSize >> 10;
#endif
	return 0;
}

auto to_utf8(string_view source, string& dest, UConverter* ucnv,
             UErrorCode& uerr)
{
	dest.resize(dest.capacity());
	auto len = ucnv_toAlgorithmic(UCNV_UTF8, ucnv, dest.data(), dest.size(),
	                              source.data(), source.size(), &uerr);
	dest.resize(len);
	if (uerr == U_BUFFER_OVERFLOW_ERROR) {
		uerr = U_ZERO_ERROR;
		ucnv_toAlgorithmic(UCNV_UTF8, ucnv, dest.data(), dest.size(),
		                   source.data(), source.size(), &uerr);
	}
}

auto from_utf8(string_view source, string& dest, UConverter* ucnv,
               UErrorCode& uerr)
{
	dest.resize(dest.capacity());
	auto len =
	    ucnv_fromAlgorithmic(ucnv, UCNV_UTF8, dest.data(), dest.size(),
	                         source.data(), source.size(), &uerr);
	dest.resize(len);
	if (uerr == U_BUFFER_OVERFLOW_ERROR) {
		uerr = U_ZERO_ERROR;
		ucnv_fromAlgorithmic(ucnv, UCNV_UTF8, dest.data(), dest.size(),
		                     source.data(), source.size(), &uerr);
	}
}

struct Options {
	bool print_mismatches = false;
	bool test_suggestions = false;
	bool wikipedia_list = false;
};

auto process_text(Options opt, const Dictionary& dic, Hunspell& hun,
                  UConverter* hun_cnv, istream& in, UConverter* in_cnv,
                  ostream& out, UErrorCode& uerr)
{
	using Duration = chrono::high_resolution_clock::duration;
	string word, line, u8_tmp_buf, hun_word;
	size_t total = 0;
	size_t true_pos = 0, true_neg = 0, false_pos = 0, false_neg = 0;
	Duration time_nu_true = {}, time_nu_false = {};
	Duration time_hun_true = {}, time_hun_false = {};

	auto nus_sugs = vector<string>();
	Duration time_nu_sugs = {}, time_hun_sugs = {};
	size_t nus_hits = 0, nus_misses = 0, nus_sum_orders = 0;
	size_t hun_hits = 0, hun_misses = 0, hun_sum_orders = 0;

	auto hun_enc =
	    nuspell::Encoding(hun.get_dict_encoding()).value_or_default();
	auto in_is_utf8 = ucnv_getType(in_cnv) == UCNV_UTF8;
	// auto out_is_utf8 = ucnv_getType(out_cnv) == UCNV_UTF8;
	auto hun_is_utf8 = ucnv_getType(hun_cnv) == UCNV_UTF8;

	for (;;) {
		size_t i = 0;
		if (opt.wikipedia_list) {
			if (!getline(in, line))
				break;
			if (line.empty() || line.front() == '#')
				continue;
			auto rtrim_idx = line.find_last_not_of(" \t\r");
			if (rtrim_idx != line.npos)
				line.erase(++rtrim_idx);
			if (!in_is_utf8) {
				to_utf8(line, u8_tmp_buf, in_cnv, uerr);
				line = u8_tmp_buf;
			}
			if ((i = line.find("->")) != line.npos) {
				word.assign(line, 0, i);
				i += 2;
			}
			else if ((i = line.find('\t')) != line.npos) {
				word.assign(line, 0, i);
				i += 1;
			}
			else
				continue;
		}
		else if (in >> word) {
			if (!in_is_utf8) {
				to_utf8(word, u8_tmp_buf, in_cnv, uerr);
				word = u8_tmp_buf;
			}
		}
		else
			break;
		auto time_a = chrono::high_resolution_clock::now();
		auto res_nu = dic.spell(word);
		auto time_b = chrono::high_resolution_clock::now();
		if (hun_is_utf8)
			hun_word = word;
		else
			from_utf8(word, hun_word, hun_cnv, uerr);
		auto res_hun = hun.spell(hun_word);
		auto time_c = chrono::high_resolution_clock::now();
		auto time_nu = time_b - time_a;
		auto time_hun = time_c - time_b;
		if (res_nu)
			time_nu_true += time_nu;
		else
			time_nu_false += time_nu;
		if (res_hun)
			time_hun_true += time_hun;
		else
			time_hun_false += time_hun;
		if (res_hun) {
			if (res_nu) {
				++true_pos;
			}
			else {
				++false_neg;
				if (opt.print_mismatches)
					out << "FN: " << word << '\n';
			}
		}
		else {
			if (res_nu) {
				++false_pos;
				if (opt.print_mismatches)
					out << "FP: " << word << '\n';
			}
			else {
				++true_neg;
			}
		}
		++total;

		if (!opt.test_suggestions && !opt.wikipedia_list)
			continue;
		time_a = chrono::high_resolution_clock::now();
		dic.suggest(word, nus_sugs);
		time_b = chrono::high_resolution_clock::now();
		auto hun_sugs = hun.suggest(hun_word);
		time_c = chrono::high_resolution_clock::now();
		time_nu_sugs += time_b - time_a;
		time_hun_sugs += time_c - time_b;
		if (!opt.wikipedia_list)
			continue;
		if (opt.print_mismatches && (res_nu || res_hun)) {
			out << "Word " << word << " detected as correct.\n"
			    << "Suggestions from list: " << line.substr(i)
			    << '\n';
		}
		if (opt.print_mismatches && res_nu && !empty(nus_sugs)) {
			out << "Nuspell sugs : ";
			for (auto it = begin(nus_sugs);;) {
				out << *it;
				if (++it == end(nus_sugs))
					break;
				out << ", ";
			}
			out << '\n';
		}

		for (auto& hs : hun_sugs) {
			to_utf8(hs, u8_tmp_buf, hun_cnv, uerr);
			hs = u8_tmp_buf;
		}
		// hun_sugs is now utf8

		if (opt.print_mismatches && res_hun && !empty(hun_sugs)) {
			out << "Hunspell sugs: ";
			for (auto it = begin(hun_sugs);;) {
				out << *it;
				if (++it == end(hun_sugs))
					break;
				out << ", ";
			}
			out << '\n';
		}
		for (auto needle = ", "sv;;) {
			auto j = line.find(needle, i);
			auto wiki_sug = line.substr(i, j - i);

			// process wiki_sug
			auto it =
			    find(begin(nus_sugs), end(nus_sugs), wiki_sug);
			if (it != end(nus_sugs)) {
				++nus_hits;
				auto order = distance(begin(nus_sugs), it) + 1;
				nus_sum_orders += order;
			}
			else {
				++nus_misses;
			}

			it = find(begin(hun_sugs), end(hun_sugs), wiki_sug);
			if (it != end(hun_sugs)) {
				++hun_hits;
				auto order = distance(begin(hun_sugs), it) + 1;
				hun_sum_orders += order;
			}
			else {
				++hun_misses;
			}

			// stop condition
			if (j == line.npos)
				break;
			// increment
			i = j + size(needle);
		}
		if (total % 100 == 0)
			out << total << " words processed." << endl;
	}
	out << "Total Words = " << total << '\n';
	if (total == 0)
		return;
	auto accuracy = (true_pos + true_neg) * 1.0 / total;
	auto precision = true_pos * 1.0 / (true_pos + false_pos);
	auto total_hun_time = time_hun_true + time_hun_false;
	auto total_nu_time = time_nu_true + time_nu_false;
	auto speedup = total_hun_time.count() * 1.0 / total_nu_time.count();
	out << "TP = " << true_pos << '\n';
	out << "TN = " << true_neg << '\n';
	out << "FP = " << false_pos << '\n';
	out << "FN = " << false_neg << '\n';
	out << "Accuracy  = " << accuracy << '\n';
	out << "Precision = " << precision << '\n';
	out << "Time Nuspell True   = " << time_nu_true.count() << '\n';
	out << "Time Nuspell False  = " << time_nu_false.count() << '\n';
	out << "Time Hunspell True  = " << time_hun_true.count() << '\n';
	out << "Time Hunspell False = " << time_hun_false.count() << '\n';
	out << "Speedup = " << speedup << '\n';
	if (!opt.test_suggestions && !opt.wikipedia_list)
		return;
	out << '\n';
	out << "Nus hits      = " << nus_hits << '\n';
	out << "Nus misses    = " << nus_misses << '\n';
	out << "Nus avg order = " << nus_sum_orders * 1.0 / nus_hits << '\n';
	out << "Hun hits      = " << hun_hits << '\n';
	out << "Hun misses    = " << hun_misses << '\n';
	out << "Hun avg order = " << hun_sum_orders * 1.0 / hun_hits << '\n';
	out << "Time Nuspell sugs  = " << time_nu_sugs.count() << '\n';
	out << "Time Hunspell sugs = " << time_hun_sugs.count() << '\n';
}
} // namespace
int main(int argc, char* argv[])
{
	auto mode_int = int(Mode::NORMAL);
	auto program_name = "nuspell";
	auto dictionary = string();
	auto input_enc = string();
	auto output_enc = string();
	auto options = Options();

	if (argc > 0 && argv[0])
		program_name = argv[0];

	ios_base::sync_with_stdio(false);

	auto optstring = "d:msw";
	option longopts[] = {
	    {"help", no_argument, &mode_int, Mode::HELP},
	    {"version", no_argument, &mode_int, Mode::VERSION},
	    {"dictionary", required_argument, nullptr, 'd'},
	    {"print-mismatches", no_argument, nullptr, 'm'},
	    {"test-suggestions", no_argument, nullptr, 's'},
	    {"wikipedia-list", no_argument, nullptr, 'w'},
	    {"encoding", required_argument, nullptr, 'e'},
	    {"input-encoding", required_argument, nullptr, 'i'},
	    {"output-encoding", required_argument, nullptr, 'o'},
	    {}};
	int longindex;
	int c;
	while ((c = getopt_long(argc, argv, optstring, longopts, &longindex)) !=
	       -1) {
		switch (c) {
		case 0:
			// check longopts[longindex] if needed
			break;
		case 'd':
			dictionary = optarg;
			break;
		case 'm':
			options.print_mismatches = true;
			break;
		case 's':
			options.test_suggestions = true;
			break;
		case 'w':
			options.wikipedia_list = true;
			break;
		case 'e':
			input_enc = optarg;
			output_enc = optarg;
			break;
		case 'i':
			input_enc = optarg;
			break;
		case 'o':
			output_enc = optarg;
			break;
		case '?':
			return EXIT_FAILURE;
		}
	}
	auto mode = static_cast<Mode>(mode_int);
	if (mode == Mode::VERSION) {
		print_version();
		return 0;
	}
	else if (mode == Mode::HELP) {
		print_help(program_name);
		return 0;
	}
	auto f = Dict_Finder_For_CLI_Tool_2();

	char* loc_str = nullptr;
#if _WIN32
	loc_str = setlocale(LC_CTYPE, nullptr); // will return "C"
#else
	loc_str = setlocale(LC_CTYPE, "");
	if (!loc_str) {
		clog << "WARNING: Can not set to system locale, fall back to "
		        "\"C\".\n";
		loc_str = setlocale(LC_CTYPE, nullptr); // will return "C"
	}
#endif
#if _POSIX_VERSION
	auto enc_str = nl_langinfo(CODESET);
	if (input_enc.empty())
		input_enc = enc_str;
	if (output_enc.empty())
		output_enc = enc_str;
#elif _WIN32
	if (optind == argc && _isatty(_fileno(stdin)))
		input_enc = "cp" + to_string(GetConsoleCP());
	else if (input_enc.empty())
		input_enc = "UTF-8";
	if (_isatty(_fileno(stdout)))
		output_enc = "cp" + to_string(GetConsoleOutputCP());
	else if (output_enc.empty())
		output_enc = "UTF-8";
#endif
	auto loc_str_sv = string_view(loc_str);
	clog << "INFO: Locale LC_CTYPE=" << loc_str_sv
	     << ", Input encoding=" << input_enc
	     << ", Output encoding=" << output_enc << endl;

	if (dictionary.empty()) {
		auto denv = getenv("DICTIONARY");
		if (denv)
			dictionary = denv;
	}
	if (dictionary.empty()) {
		// infer dictionary from locale
		auto idx = min(loc_str_sv.find('.'), loc_str_sv.find('@'));
		dictionary = loc_str_sv.substr(0, idx);
	}
	if (dictionary.empty()) {
		clog << "ERROR: No dictionary provided and can not infer from "
		        "OS locale\n";
		return EXIT_FAILURE;
	}
	auto filename = f.get_dictionary_path(dictionary);
	if (filename.empty()) {
		clog << "ERROR: Dictionary " << dictionary << " not found\n";
		return EXIT_FAILURE;
	}
	clog << "INFO: Pointed dictionary " << filename.string() << endl;
	auto peak_ram_a = get_peak_ram_usage();
	auto dic = Dictionary();
	try {
		dic.load_aff_dic_internal(filename, clog);
	}
	catch (const Dictionary_Loading_Error& e) {
		clog << "ERROR: " << e.what() << '\n';
		return EXIT_FAILURE;
	}
	auto nuspell_ram = get_peak_ram_usage() - peak_ram_a;
	auto aff_name = filename.string();
	auto dic_name = filename.replace_extension(".dic").string();
	peak_ram_a = get_peak_ram_usage();
	auto hun = Hunspell(aff_name.c_str(), dic_name.c_str());
	auto hunspell_ram = get_peak_ram_usage() - peak_ram_a;
	cout << "Nuspell peak RAM usage:  " << nuspell_ram << "KB\n"
	     << "Hunspell peak RAM usage: " << hunspell_ram << "KB\n";

	// ICU reports all types of errors, logic errors and runtime errors
	// using this enum. We should not check for logic errors, they should
	// not happened. Optionally, only assert that they are not there can be
	// used. We should check for runtime errors.
	// The encoding conversion is a common case where runtime error can
	// happen, but by default ICU uses Unicode replacement character on
	// errors and reprots success. This can be changed, but there is no need
	// for that.
	auto uerr = U_ZERO_ERROR;
	auto inp_enc_cstr = input_enc.c_str();
	if (input_enc.empty()) {
		inp_enc_cstr = nullptr;
		clog << "WARNING: using default ICU encoding converter for IO"
		     << endl;
	}
	auto in_ucnv =
	    icu::LocalUConverterPointer(ucnv_open(inp_enc_cstr, &uerr));
	if (U_FAILURE(uerr)) {
		clog << "ERROR: Invalid encoding " << input_enc << ".\n";
		return EXIT_FAILURE;
	}

	auto hun_enc =
	    nuspell::Encoding(hun.get_dict_encoding()).value_or_default();
	auto hun_cnv =
	    icu::LocalUConverterPointer(ucnv_open(hun_enc.c_str(), &uerr));
	if (U_FAILURE(uerr)) {
		clog << "ERROR: Invalid Hun encoding " << hun_enc << ".\n";
		return EXIT_FAILURE;
	}

	if (optind == argc) {
		process_text(options, dic, hun, hun_cnv.getAlias(), cin,
		             in_ucnv.getAlias(), cout, uerr);
	}
	else {
		for (; optind != argc; ++optind) {
			auto file_name = argv[optind];
			ifstream in(file_name);
			if (!in.is_open()) {
				clog << "ERROR: Can't open " << file_name
				     << '\n';
				return EXIT_FAILURE;
			}
			process_text(options, dic, hun, hun_cnv.getAlias(), in,
			             in_ucnv.getAlias(), cout, uerr);
		}
	}
}
