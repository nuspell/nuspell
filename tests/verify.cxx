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

#include <hunspell/hunspell.hxx>
#include <nuspell/dictionary.hxx>
#include <nuspell/finder.hxx>

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <unicode/ucnv.h>

#if defined(__MINGW32__) || defined(__unix__) || defined(__unix) ||            \
    (defined(__APPLE__) && defined(__MACH__)) || defined(__HAIKU__)
#include <getopt.h>
#include <unistd.h>
#endif
#ifdef _POSIX_VERSION
#include <langinfo.h>
#include <sys/resource.h>
#include <sys/time.h>
#endif

// manually define if not supplied by the build system
#ifndef PROJECT_VERSION
#define PROJECT_VERSION "unknown.version"
#endif
#define PACKAGE_STRING "nuspell " PROJECT_VERSION

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
	     "Verification testing of Nuspell for each FILE.\n"
	     "Without FILE, check standard input.\n"
	     "\n"
	     "  -d di_CT      use di_CT dictionary. Only one dictionary is\n"
	     "                currently supported\n"
	     "  -i enc        input encoding, default is active locale\n"
	     "  -f            print false negative and false positive words\n"
	     "  -s            also test suggestions (usable only in debugger)\n"
	     "  -h, --help    print this help and exit\n"
	     "  -v, --version print version number and exit\n"
	     "\n";
	o << "Example: " << p << " -d en_US /usr/share/dict/american-english\n";
	o << "\n"
	     "The input should contain one word per line. Each word is\n"
	     "checked in Nuspell and Hunspell and the results are compared.\n"
	     "After all words are processed, some statistics are printed like\n"
	     "correctness and speed of Nuspell compared to Hunspell.\n"
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
	    "Written by Dimitrij Mijoski and Sander van Geloven.\n";
}

auto get_peak_ram_usage() -> long
{
#ifdef _POSIX_VERSION
	rusage r;
	getrusage(RUSAGE_SELF, &r);
	return r.ru_maxrss;
#else
	return 0;
#endif
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

auto normal_loop(const Args_t& args, const Dictionary& dic, Hunspell& hun,
                 istream& in, ostream& out)
{
	auto print_false = args.print_false;
	auto test_sugs = args.sugs;
	auto word = string();
	auto u8_buffer = string();
	auto hun_word = string();
	auto total = 0;
	auto true_pos = 0;
	auto true_neg = 0;
	auto false_pos = 0;
	auto false_neg = 0;
	auto duration_hun = chrono::high_resolution_clock::duration();
	auto duration_nu = duration_hun;
	auto in_loc = in.getloc();

	auto uerr = U_ZERO_ERROR;
	auto io_cnv = icu::LocalUConverterPointer(
	    ucnv_open(args.encoding.c_str(), &uerr));
	if (U_FAILURE(uerr))
		throw runtime_error("Invalid io encoding");
	auto hun_enc =
	    nuspell::Encoding(hun.get_dict_encoding()).value_or_default();
	auto hun_cnv =
	    icu::LocalUConverterPointer(ucnv_open(hun_enc.c_str(), &uerr));
	if (U_FAILURE(uerr))
		throw runtime_error("Invalid hun encoding");
	auto io_is_utf8 = ucnv_getType(io_cnv.getAlias()) == UCNV_UTF8;
	auto hun_is_utf8 = ucnv_getType(hun_cnv.getAlias()) == UCNV_UTF8;

	// need to take entine line here, not `in >> word`
	while (getline(in, word)) {
		auto u8_word = string_view();
		auto tick_a = chrono::high_resolution_clock::now();
		if (io_is_utf8) {
			u8_word = word;
		}
		else {
			to_utf8(word, u8_buffer, io_cnv.getAlias(), uerr);
			u8_word = u8_buffer;
		}
		auto res_nu = dic.spell(u8_word);
		auto tick_b = chrono::high_resolution_clock::now();
		if (hun_is_utf8)
			hun_word = u8_word;
		else
			from_utf8(u8_word, hun_word, hun_cnv.getAlias(), uerr);
		auto res_hun = hun.spell(hun_word);
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
			hun.suggest(hun_word);
		}
	}
	out << "Total Words         " << total << '\n';
	// prevent devision by zero
	if (total == 0)
		return;
	auto accuracy = (true_pos + true_neg) * 1.0 / total;
	auto precision = true_pos * 1.0 / (true_pos + false_pos);
	auto speedup = duration_hun.count() * 1.0 / duration_nu.count();
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

int main(int argc, char* argv[])
{
	// May speed up I/O. After this, don't use C printf, scanf etc.
	ios_base::sync_with_stdio(false);

	auto args = Args_t(argc, argv);

	switch (args.mode) {
	case HELP_MODE:
		print_help(args.program_name);
		return 0;
	case VERSION_MODE:
		print_version();
		return 0;
	case ERROR_MODE:
		cerr << "Invalid (combination of) arguments, try '"
		     << args.program_name << " --help' for more information\n";
		return 1;
	default:
		break;
	}
	auto f = Dict_Finder_For_CLI_Tool();

	auto loc_str = setlocale(LC_CTYPE, "");
	if (!loc_str) {
		clog << "WARNING: Invalid locale string, fall back to \"C\".\n";
		loc_str = setlocale(LC_CTYPE, nullptr); // will return "C"
	}
	auto loc_str_sv = string_view(loc_str);
	if (args.encoding.empty()) {
#if _POSIX_VERSION
		auto enc_str = nl_langinfo(CODESET);
		args.encoding = enc_str;
#elif _WIN32
#endif
	}
	clog << "INFO: Locale LC_CTYPE=" << loc_str_sv
	     << ", Used encoding=" << args.encoding << '\n';
	if (args.dictionary.empty()) {
		// infer dictionary from locale
		auto idx = min(loc_str_sv.find('.'), loc_str_sv.find('@'));
		args.dictionary = loc_str_sv.substr(0, idx);
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
	auto peak_ram_a = get_peak_ram_usage();
	auto dic = Dictionary();
	try {
		dic = Dictionary::load_from_path(filename);
	}
	catch (const Dictionary_Loading_Error& e) {
		cerr << e.what() << '\n';
		return 1;
	}
	auto nuspell_ram = get_peak_ram_usage() - peak_ram_a;
	auto aff_name = filename + ".aff";
	auto dic_name = filename + ".dic";
	peak_ram_a = get_peak_ram_usage();
	Hunspell hun(aff_name.c_str(), dic_name.c_str());
	auto hunspell_ram = get_peak_ram_usage() - peak_ram_a;
	cout << "Nuspell peak RAM usage:  " << nuspell_ram << "kB\n"
	     << "Hunspell peak RAM usage: " << hunspell_ram << "kB\n";
	if (args.files.empty()) {
		normal_loop(args, dic, hun, cin, cout);
	}
	else {
		for (auto& file_name : args.files) {
			ifstream in(file_name);
			if (!in.is_open()) {
				cerr << "Can't open " << file_name << '\n';
				return 1;
			}
			in.imbue(cin.getloc());
			normal_loop(args, dic, hun, in, cout);
		}
	}
	return 0;
}
