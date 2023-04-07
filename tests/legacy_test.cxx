/* Copyright 2018-2023 Dimitrij Mijoski
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

#include <fstream>
#include <iostream>

using namespace std;

int main(int argc, char* argv[])
{
	if (argc < 2)
		return 3;
	auto test = string(argv[1]);
	if (test.size() < 4) {
		cerr << "Invalid test type\n";
		return 3;
	}
	auto type = test.substr(test.size() - 4);
	auto file = ifstream(test);
	if (!file.is_open()) {
		cerr << "Can not open test file " << test << " \n";
		return 2;
	}
	file.close();
	test.replace(test.size() - 4, 4, ".aff");
	auto d = nuspell::Dictionary();
	try {
		d.load_aff_dic(test);
	}
	catch (const nuspell::Dictionary_Loading_Error& err) {
		cerr << err.what() << '\n';
		return 2;
	}
	test.erase(test.size() - 4);
	auto word = string();
	if (type == ".dic") {
		auto error = vector<string>();
		file.open(test + ".good");
		while (file >> word) {
			if (d.spell(word) == false)
				error.push_back(word);
		}
		if (!error.empty()) {
			cout << "Good words recognised as bad:\n";
			for (auto& w : error) {
				cout << w << '\n';
			}
			return 1;
		}
		file.close();

		error.clear();
		file.open(test + ".wrong");
		while (file >> word) {
			if (d.spell(word))
				error.push_back(word);
		}
		if (!error.empty()) {
			cout << "Bad words recognised as good:\n";
			for (auto& w : error) {
				cout << w << '\n';
			}
			return 1;
		}
	}
	else if (type == ".sug") {
		file.open(test + ".wrong");
		if (!file.is_open()) {
			cerr << "Can not open test file " << test + ".wrong"
			     << " \n";
			return 2;
		}
		auto list_sugs = vector<vector<string>>();
		auto sug = vector<string>();
		while (file >> word) {
			if (d.spell(word))
				continue;

			d.suggest(word, sug);
			if (!sug.empty())
				list_sugs.push_back(sug);
		}
		file.close();

		file.open(test + ".sug");
		if (!file.is_open())
			return 2;
		auto expected_list_sugs = vector<vector<string>>();
		auto line = string();
		while (getline(file, line)) {
			if (line.empty())
				continue;
			sug.clear();
			for (size_t i = 0;;) {
				auto j = line.find(',', i);
				word.assign(line, i, j - i);
				sug.push_back(word);
				if (j == line.npos)
					break;

				i = line.find_first_not_of(' ', j + 1);
				if (i == line.npos)
					break;
			}
			if (!sug.empty())
				expected_list_sugs.push_back(sug);
		}
		if (list_sugs != expected_list_sugs) {
			cout << "Bad suggestions.\nExpected output:\n";
			for (auto& x : expected_list_sugs) {
				for (auto& w : x) {
					cout << w << ", ";
				}
				cout << '\n';
			}
			cout << "\nActual output:\n";
			for (auto& x : list_sugs) {
				for (auto& w : x) {
					cout << w << ", ";
				}
				cout << '\n';
			}
			return 1;
		}
	}
	else {
		cerr << "Invalid test type\n";
		return 3;
	}
}
