#include <iostream>
#include <fstream>
#include "termgrep.hpp"
#include <string.h>

using namespace std;
using namespace termgrep;
#ifndef CTYPE
#define QUOTE(m) #m
#define STR(m) QUOTE(m)
#define CTYPE char
#define CTYPENAME STR(CTYPE)
#endif

#define CW(str) CWSTR(CharType, str)

template<class CType = CharType>
inline basic_ostream<CType> &out();

template<> inline basic_ostream<char> &out<char>() { return cout; }
template<> inline basic_ostream<wchar_t> &out<wchar_t>() { return wcout; }

template<class keyT, class valT>
basic_ostream<CharType> &operator<<(basic_ostream<CharType> &os,
		const map<keyT, valT> &mp) {
	os << "map("<< mp.size() <<"){";
	bool cnt = 0;
	for (const pair<keyT, valT> &entry : mp)
		os << (cnt++ > 0 ? ", '" : "'") << entry.first <<"': "<< entry.second;
	return os << "}";
}

int main() {
	cout << "compiled with CharType=" CTYPENAME << endl;
	TermGrep grep;
	for (auto term : {CW("foo"), CW("bar"), CW("foo bar")})
		grep.addTerm(term);
	auto checker = grep.makeChecker();
	checker->check(CW("This is a test text with foo and bar and even foo bar"));
	out() << checker->getTermOccurences() << endl;

	basic_ofstream<CharType>("fsm.gv") << *grep.getGraph();
	basic_ofstream<CharType>("fsm_check.gv") << *checker->getGraph();
	return 0;
}
