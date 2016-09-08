#include <iostream>
#include <fstream>
#include <string>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#ifndef DEFAULT_CTYPE
#define DEFAULT_CTYPE char
#define CTYPENAME STR(DEFAULT_CTYPE)
#endif
#include "outputformats.hpp"

using namespace std;
using namespace termgrep;
using json = nlohmann::json;
namespace po = boost::program_options;

#define QUOTE(m) #m
#define STR(m) QUOTE(m)

#define CW(str) CWSTR(CharType, str)


template<class CType = DefaultCharType>
inline basic_istream<CType> &in();

template<> inline basic_istream<char> &in<char>() { return cin; }
template<> inline basic_istream<wchar_t> &in<wchar_t>() { return wcin; }

template<class keyT, class valT, class CharType>
basic_ostream<CharType> &operator<<(basic_ostream<CharType> &os,
		const map<keyT, valT> &mp) {
	os << "map("<< mp.size() <<"){";
	bool cnt = 0;
	for (const pair<keyT, valT> &entry : mp)
		os << (cnt++ > 0 ? ", '" : "'") << entry.first <<"': "<< entry.second;
	return os << "}";
}

pair<string,string> getFileIdentifier(string input, string separator) {
	if (separator.length() > 0) {
		size_t equals = input.rfind(separator);
		if (equals != string::npos) {
			auto p = make_pair(input.substr(0,equals),
				input.substr(equals + separator.length()));
			return p;
		}
	}
	return make_pair(input, input);
}

template<class CharType>
void readTermsFrom(TermGrep<CharType> &grep, basic_istream<CharType> &infile) {
	basic_string<CharType> line;
	while (infile) {
		getline(infile, line);
		boost::trim(line);
		if (line.length() > 0)
			grep.addTerm(line);
	}
	cerr << "Successfully read "<< grep.getTerms().size() << " terms" << endl;
}

template<class CharType>
void readTermsFrom(TermGrep<CharType> &grep, string infname) {
	basic_ifstream<CharType> infile(infname);
	readTermsFrom(grep, infile);
	infile.close();
}

template<class CharType>
bool feedTo(const string &fname, typename TermGrep<CharType>::Matcher &matcher) {
	auto fin = basic_ifstream<CharType>(fname);
	if (!fin) {
		cerr << "Can't read file "<< fname <<" :" << endl
			<< "\t" << strerror(errno) << endl;
		return false;
	}
	fin >> matcher;
	return true;
}

int main(int argc, char **argv) {
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "Display help message")
		("terms", po::value<string>(),
			"File containing terms to look for (1 per line)")
		("fileid-separator", po::value<string>()->default_value(""),
			"Separator for fileid/filepath")
		("no-whole-words", po::bool_switch(), "Do not automatically surround "
			"terms with \"bound-of-word\" symbols")
		("output-format", po::value<string>()->default_value("json"),
			"Output format. Supported: json (default), csv")
		("output-fsm", po::value<string>())
		("output-matcher-fsm", po::value<string>())
		("output-file", po::value<string>())
		("json-output-termids", po::bool_switch())
		("csv-output-separator", po::value<string>())
		("input", po::value<vector<string>>())
		("terms-stdin", po::bool_switch())
		("file-list-stdin", po::bool_switch());
	po::positional_options_description pdesc;
	pdesc.add("input", -1);
	po::variables_map vm;
	try {
		po::store(po::command_line_parser(argc, argv)
			.options(desc).positional(pdesc).run(), vm);
		po::notify(vm);
	} catch (exception const &ex) {
		cerr << "Error : "<< ex.what() << endl
			<< desc << endl;
		return 1;
	}

	const bool
		wholeWords = !vm["no-whole-words"].as<bool>(),
		termsStdin = vm["terms-stdin"].as<bool>(),
		fileListStdin = vm["file-list-stdin"].as<bool>();

	TermGrep<> grep(wholeWords);

	if (vm.count("help")) {
		cout << desc << endl;
		return 0;
	}
	if (termsStdin && fileListStdin) {
		cerr << "Can't use both --terms-stdin and --file-list-stdin" << endl;
		return 1;
	}
	if (!vm.count("input") && termsStdin) {
		cerr << "Error : no input file(s) specified and stdin reserved for terms" << endl;
		return 1;
	}

	if (!vm.count("terms") && !termsStdin) {
		cerr << "Must specify terms file or use --terms-stdin" << endl;
		return 1;
	} else if (!termsStdin)
		readTermsFrom(grep, vm["terms"].as<string>());
	else
		readTermsFrom(grep, in());
	auto matcher = grep.makeChecker();
	if (vm.count("output-fsm"))
		basic_ofstream<DefaultCharType>(vm["output-fsm"].as<string>())
			<< *grep.getGraph();
	if (vm.count("output-matcher-fsm"))
		basic_ofstream<DefaultCharType>(vm["output-matcher-fsm"].as<string>())
			<< *matcher->getGraph();

	vector<string> inputFiles;
	if (vm.count("input"))
		for (auto fname : vm["input"].as<vector<string>>())
			inputFiles.push_back(fname);

	if (fileListStdin) {
		string line;
		while (cin) {
			getline(cin, line);
			boost::trim(line);
			if (line.length() > 0)
				inputFiles.push_back(line);
		}
	}

	OutputOptions opts;
	opts.outputTermids = vm["json-output-termids"].as<bool>();
	if (vm.count("csv-output-separator"))
		opts.separator = vm["csv-output-separator"].as<string>();
	Formats format;
	try {
		format = getFormatByName(vm["output-format"].as<string>());
	} catch (runtime_error &er) {
		cerr << er.what() << endl
			<< "Supported output formats: JSON, CSV, TSV" << endl;
		return 1;
	}
	auto result =
		OutputFormat<DefaultCharType>::makeOutput(format, opts);

	if (!inputFiles.empty()) {
		size_t nwidth = to_string(inputFiles.size()).length(),
			fcount = 0;
		for (auto fname : inputFiles) {
			auto fileid = getFileIdentifier(fname,
				vm["fileid-separator"].as<string>());
			cerr << "Reading ("<< setw(nwidth) << (++fcount) <<"/"<<
				inputFiles.size() <<")"<< fileid.second << endl;
			if (feedTo<DefaultCharType>(fileid.second, *matcher)) {
				matcher->end();
				result->addFileResult(fileid.first, *matcher);
				matcher->reset();
			}
		}
	} else {
		cerr << "Reading from standard input" << endl;
		in() >> *matcher;
		matcher->end();
		result->addFileResult("stdin", *matcher);
	}
	if (vm.count("output-file")) {
		cout << "Writing results to "<< vm["output-file"].as<string>() << endl;
		ofstream(vm["output-file"].as<string>()) << *result << flush;
	} else {
		cout << setw(2) << *result << endl;
	}
	return 0;
}
