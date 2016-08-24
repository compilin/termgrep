#include <locale>
#include <list>
#include <vector>
#include <memory>

#ifndef CTYPE
#define CTYPE wchar_t
#endif
#ifndef TERMGREP_NO_GVPP
#include "gvpp.hpp"
#endif


namespace termgrep {
	using namespace std;

	typedef CTYPE CharType;
	typedef basic_string<CTYPE> strtype;
#undef CTYPE

	struct _checkfunc {
		static const strtype DEFAULT_LABEL;
		strtype label = DEFAULT_LABEL;
		function<bool(int)> func = [](int i) -> bool {return true;};
		bool operator()(int chr) { return func(chr); }
		bool operator==(const _checkfunc &cf)
			{ return this->label == cf.label; }
		_checkfunc() {}
		_checkfunc(strtype label, function<bool(int)> func) :
			label(label), func(func) {}
	};

	typedef struct _checkfunc CheckFunc;

	class AbstractFSM {
	public:
		struct _state;
		typedef struct _nxt_state {
			std::shared_ptr<struct _state> state;
			std::unique_ptr<struct _nxt_state> next;
		} NextState;
		typedef struct _state {
			size_t id; // Corresponds to their position in the 'states' vector
			CharType chr;
			CheckFunc func;
			bool isfunc;
			size_t termid = 0;
			std::unique_ptr<struct _nxt_state> next = std::unique_ptr<struct _nxt_state>();
			_state(size_t id, CharType chr, CheckFunc func,
				bool isfunc, size_t termid, std::unique_ptr<struct _nxt_state> next) :
					id(id), chr(chr), func(func), isfunc(isfunc),
					termid(termid), next(move(next)){};
		} State;
		typedef shared_ptr<State> StatePtr;
#ifndef TERMGREP_NO_GVPP
		unique_ptr <gvpp::Graph<CharType>> getGraph();
#endif
		const strtype & getTerm(size_t id);
		const vector<strtype> &getTerms() { return terms; }
	protected:
		AbstractFSM(vector<strtype> &terms) : terms(terms) {};
		vector<StatePtr> states;
		const vector<strtype> &terms;

		inline StatePtr getRoot() { return states[0]; }
		size_t addState(CharType chr);
		size_t addState(CheckFunc func);
	};

	class TermGrep : public AbstractFSM {
	private:
		bool addWordBoundaries;
		vector<strtype> _terms;
		void addStates(StatePtr from, const CharType *chars);
		StatePtr firstState;
		size_t longestTerm = 0;
	public:
		class Matcher;
		TermGrep(bool addWordBoundaries = true) :
				AbstractFSM(_terms), addWordBoundaries(addWordBoundaries) {
			addState((CharType)0);
			_terms.push_back(CWSTR(CharType, "#ERROR#"));
		}
		size_t addTerm(strtype term) { return addTerm(term, addWordBoundaries); }
		size_t addTerm(strtype term, bool bound);

		unique_ptr <Matcher> makeChecker();
	};

	class TermGrep::Matcher : public AbstractFSM {
		friend class TermGrep;
	public:
		class Match {
			friend class TermGrep::Matcher;
		private:
			Match(size_t termid, size_t startPos, const strtype term) :
				termid(termid), startPos(startPos), term(term) {}
		public:
			const size_t termid;
			const size_t startPos;
			const strtype term;
		};
	private:
		TermGrep &grep;
		Matcher(TermGrep &grep);
		StatePtr curstate;
		list<Match> candidates;
		list<Match> matches;
		size_t curPos = 0;
		size_t longestTerm = 0, nextCheck = 0;
	public:
		void reset();
		void end();
		void feed(CharType c);
		void feed(const CharType *chrs, size_t n);
		void feed(strtype str) { feed(str.c_str()); }
		void check(strtype str) { reset(); feed(str); end(); }
		const list<Match> &getMatches() { return matches; }
		map<size_t, size_t> &&getTermidOccurences
				(map<size_t, size_t> occurences = map<size_t, size_t>());
		map<strtype, size_t> &&getTermOccurences
				(map<strtype, size_t> occurences = map<strtype, size_t>());
		void clearMatches() { matches.clear(); }
	};

	template<class CharType>
	basic_istream<CharType> &operator>>(basic_istream<CharType> &is,
		TermGrep::Matcher &checker) {
		static const size_t BUFSIZE = 256;
		CharType buf[BUFSIZE];
		while (is) {
			is.read(buf, BUFSIZE);
			checker.feed(buf, is.gcount());
		}
		return is;
	}
}
