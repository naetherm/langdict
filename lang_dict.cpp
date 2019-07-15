// Copyright 2018, University of Freiburg.
// Chair of Algorithms and Data Structures.
// Markus Näther <naetherm@informatik.uni-freiburg.de>

#include <string>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <map>
#include <set>
#include "cxxopts.h"
#include "json.h"
#include "metaphone.h"

using namespace std;
typedef char tchar;

#define METAPHONE_KEY_LENGTH 4

#define REAL_WORD "REAL_WORD"
#define ARCHAIC_WORD "ARCHAIC"

typedef std::multimap<std::string, std::string> WordMapType;
typedef std::set<std::string> WordListType;

void phonetic_match(
  WordMapType & words,
  const std::string & searchWord,
  WordListType & matchingWords
) {
  DoubleMetaphone<METAPHONE_KEY_LENGTH> searchKey(searchWord.c_str());
  std::string search1 = searchKey.getPrimaryKey();
  // TODO(naetherm): Implement this!
  for (WordMapType::iterator iter = words.lower_bound(search1);
		   iter != words.upper_bound(search1);
		   iter++) {
		matchingWords.insert((*iter).second);
	}
	if (searchKey.getAlternateKey() != NULL) {
		string search2 = searchKey.getAlternateKey();
		for (WordMapType::iterator iter = words.lower_bound(search2);
			   iter != words.upper_bound(search2);
			   iter++) {
			matchingWords.insert((*iter).second);
		}
	}
}

unsigned int edit_distance(const std::string& s1, const std::string& s2) {
	const std::size_t len1 = s1.size(), len2 = s2.size();
	std::vector<std::vector<unsigned int>> d(len1 + 1, std::vector<unsigned int>(len2 + 1));

	d[0][0] = 0;
	for(unsigned int i = 1; i <= len1; ++i) d[i][0] = i;
	for(unsigned int i = 1; i <= len2; ++i) d[0][i] = i;

	for(unsigned int i = 1; i <= len1; ++i)
		for(unsigned int j = 1; j <= len2; ++j)
      // note that std::min({arg1, arg2, arg3}) works only in C++11,
      // for C++98 use std::min(std::min(arg1, arg2), arg3)
      d[i][j] = std::min({ d[i - 1][j] + 1, d[i][j - 1] + 1, d[i - 1][j - 1] + (s1[i - 1] == s2[j - 1] ? 0 : 1) });
	return d[len1][len2];
}

cxxopts::ParseResult parse(int argc, char* argv[]) {
	cxxopts::Options options(argv[0], " - Language Dictionary Creator");

        try {
		options.allow_unrecognised_options()
		  .add_options()
			("i,input", "Input file", cxxopts::value<std::string>())
			("a,archaic", "Archaic file", cxxopts::value<std::string>()->default_value(""))
			("o,output", "The output file", cxxopts::value<std::string>())
			("l,levenshtein", "The levenshtein distance", cxxopts::value<int>()->default_value("1"))
		;

		return options.parse(argc, argv);
	} catch (...) {
		options.help();
		exit(1);
	}
}


int main(int argc, char ** argv) {

  auto result = parse(argc, argv);

  //auto arguments = result.arguments();

	auto ifilename = result["input"].as<std::string>();
	auto afilename = result["archaic"].as<std::string>();
	auto ofilename = result["output"].as<std::string>();
	auto levenshtein = result["levenshtein"].as<int>();

	std::cout << "Input File: " << ifilename.c_str() << std::endl;
	std::cout << "Archaic File: " << afilename.c_str() << std::endl;
	std::cout << "Output File: " << ofilename.c_str() << std::endl;
	std::cout << "Levenshtein: " << levenshtein << std::endl;

	std::vector<std::string> iwords;
  WordMapType wordMap;
  DoubleMetaphone<METAPHONE_KEY_LENGTH> mphone;
	std::cout << "Add real words to the dictionary" << std::endl;
	// Read in inputfile
	std::ifstream ifile(ifilename);
	std::string line;
	while (std::getline(ifile, line)) {
		iwords.push_back(line);

    //Compute the metaphone keys for the word
		mphone.computeKeys(line.c_str());

    //Add a string object containing the word to the map,
		//with the primary and alternate metaphone keys as map keys
		std::string word = line;
		wordMap.insert(WordMapType::value_type(string(mphone.getPrimaryKey()), word));
		if (mphone.getAlternateKey() != nullptr) {
			wordMap.insert(WordMapType::value_type(string(mphone.getAlternateKey()),  word));
		}

		if (line.find_first_of("\n") != std::string::npos) {
			std::cout << "Line ending found!" << std::endl;
		}
	}
	ifile.close();

	// Create json file
	nlohmann::json j;
	for (int i = 0; i < iwords.size(); ++i) {
		j["real"][iwords.at(i)] = { {"id", std::to_string(i)}, {"type", REAL_WORD}, {"neighbor", {}}, {"archaic", {}} };
	}

	for (int i = 0; i < iwords.size(); ++i) {
		std::cout << "Processing " << i << " of " << iwords.size() << std::endl;
		for (int k = i; k < iwords.size(); ++k) {
			if (abs(int(iwords.at(i).size() - iwords.at(k).size())) <= levenshtein) {
				if ((std::find(j["real"][iwords.at(i)]["neighbor"].begin(), j["real"][iwords.at(i)]["neighbor"].end(), k) == j["real"][iwords.at(i)]["neighbor"].end()) &&
						(edit_distance(iwords.at(i), iwords.at(k)) == levenshtein)) {
					// Add j
					j["real"][iwords.at(i)]["neighbor"].push_back(k);
					j["real"][iwords.at(k)]["neighbor"].push_back(i);
				}
			}
		}
    // In the end: Compute phonetic similaries and add the words to j if not already present
    WordListType matchingWords;
    phonetic_match(wordMap, iwords.at(i), matchingWords);

    for (WordListType::iterator iter = matchingWords.begin(); iter != matchingWords.end(); iter++) {
      if (*iter != iwords.at(i)) {
        if (std::find(j["real"][iwords.at(i)]["neighbor"].begin(), j["real"][iwords.at(i)]["neighbor"].end(), (*iter)) == j["real"][iwords.at(i)]["neighbor"].end()) {
          j["real"][iwords.at(i)]["neighbor"].push_back(std::distance(iwords.begin(), std::find(iwords.begin(), iwords.end(), (*iter))));
        }
      }
    }
	}
	std::cout << "\tDone.\nWill now induce archaic words to the dictionary" << std::endl;
	if (!afilename.empty()) {

		std::vector<std::string> awords;
		std::ifstream afile(afilename);
		std::string line;
		while (std::getline(afile, line)) {
			awords.push_back(line);
		}

		// Append archaic words
		for (int i = 0; i < awords.size(); ++i) {
			// TODO(naetherm): Check if the word is already present within the real words
			j["archaic"][awords.at(i)] = { {"id", std::to_string(iwords.size()+i)}, {"type", ARCHAIC_WORD}, {"neighbor", {}}, {"archaic", {}} };
			for (int k = 0; k < iwords.size(); ++k) {
				if (abs(int(awords.at(i).size() - iwords.at(k).size())) <= levenshtein) {
					if ((std::find(j["real"][iwords.at(i)]["neighbor"].begin(), j["real"][iwords.at(i)]["neighbor"].end(), k) == j["real"][iwords.at(i)]["neighbor"].end()) &&
							(edit_distance(awords.at(i), iwords.at(k)) == levenshtein)) {
						// Add j
						j["archaic"][awords.at(i)]["neighbor"].push_back(k);
						j["real"][iwords.at(k)]["archaic"].push_back(i);
					}
				}
			}
			for (int k = i; k < awords.size(); ++k) {
				if (abs(int(awords.at(i).size() - awords.at(k).size())) <= levenshtein) {
					if ((std::find(j["archaic"][awords.at(i)]["archaic"].begin(), j["archaic"][awords.at(i)]["archaic"].end(), k) == j["archaic"][awords.at(i)]["archaic"].end()) &&
							(edit_distance(awords.at(i), awords.at(k)) == levenshtein)) {
						// Add j
						j["archaic"][awords.at(i)]["archaic"].push_back(k);
						j["archaic"][awords.at(k)]["archaic"].push_back(i);
					}
				}
			}
		}
		afile.close();
	}
	std::cout << "\tDone.\n" << std::endl;

	// Open the output file
	std::ofstream ofile(ofilename);
	ofile << std::setw(4) << j << std::endl;
	ofile.close();

	return 0;
}
