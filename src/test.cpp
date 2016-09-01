#include <iostream>
#include <fstream>
#include <string.h>
#ifndef DEFAULT_CTYPE
#define DEFAULT_CTYPE char
#define CTYPENAME STR(DEFAULT_CTYPE)
#endif
#include "termgrep.hpp"

using namespace std;
using namespace termgrep;
#define QUOTE(m) #m
#define STR(m) QUOTE(m)

#define CW(str) CWSTR(DefaultCharType, str)

template<class CType = DefaultCharType>
inline basic_ostream<CType> &out();

template<> inline basic_ostream<char> &out<char>() { return cout; }
template<> inline basic_ostream<wchar_t> &out<wchar_t>() { return wcout; }

template<class keyT, class valT>
basic_ostream<DefaultCharType> &operator<<(basic_ostream<DefaultCharType> &os,
		const map<keyT, valT> &mp) {
	os << "map("<< mp.size() <<"){";
	bool cnt = 0;
	for (const pair<keyT, valT> &entry : mp)
		os << (cnt++ > 0 ? ", '" : "'") << entry.first <<"': "<< entry.second;
	return os << "}";
}

int main() {
	cout << "compiled with DefaultCharType=" CTYPENAME << endl;
	TermGrep<> grep;
	for (auto term : {CW("foo"), CW("bar"), CW("foo bar")})
		grep.addTerm(term);
	auto checker = grep.makeChecker();
	checker->check(CW("This is a test text with foo and bar and even foo bar"));
	out() << checker->getTermOccurences() << endl;

	basic_ofstream<DefaultCharType>("fsm.gv") << *grep.getGraph();
	basic_ofstream<DefaultCharType>("fsm_check.gv") << *checker->getGraph();
	return 0;
}
