#include <set>
#include <locale>
#include "termgrep.hpp"

using namespace std;
using namespace gvpp;

#define CW(str) CWSTR(CharType, str)

#define AbstractFSMT AbstractFSM<CharType>
#define TermGrepT TermGrep<CharType>
#define CheckFuncT CheckFunc<CharType>
#define StateT AbstractFSM<CharType>::State
#define StateTN typename StateT
#define StatePtrT AbstractFSM<CharType>::StatePtr
#define StatePtrTN typename StatePtrT
#define NextStateT AbstractFSM<CharType>::NextState
#define NextStateTN typename NextStateT

namespace termgrep {
	template<class CType = DefaultCharType>
	inline basic_ostream<CType> &out();

	template<> inline basic_ostream<char> &out<char>() { return cout; }
	template<> inline basic_ostream<wchar_t> &out<wchar_t>() { return wcout; }

	template<class CharType>
	const strtype CheckFuncT::DEFAULT_LABEL = CW("N/A");

	template<class CharType>
	static const CharType wordBoundary() {
		return (CharType)'\t';
	}

	template<class CharType, class T>
	basic_ostream<CharType> &operator<<(basic_ostream<CharType> &os, const set<T> &st) {
		os << "{";
		auto it = st.begin();
		if (it != st.end()) {
			os << *it;
			for (it ++; it != st.end(); it++)
				os << "," << *it;
		}
		return os << "}";
	}

	template<class CharType>
	basic_ostream<CharType> &operator<<(basic_ostream<CharType> &os, const StateTN &st) {
		os << "State{"<<
		        "id: "<< st.id;
		if (st.chr != (CharType) 0)
			os << ", chr: '" << st.chr << "'";
		else
			os << ", func: " << st.func.label;
		if (st.termid != 0)
			os << ", termid: " << st.termid;
		os <<"}";
		return os;
	}

	template<class CharType>
	basic_ostream<CharType> &operator<<(basic_ostream<CharType> &os, const StatePtrTN &st) {
		os << "*" << *st;
		return os;
	}

	template<class CharType>
	size_t nbNext(unique_ptr<NextStateTN> &ptr) {
		return (ptr) ? nbNext(ptr->next) + 1 : 0;
	}

	template<class charT>
	basic_string<charT> to_str(int val);

	template<>
	basic_string<char> to_str(int val) {
		return to_string(val);
	}

	template<>
	basic_string<wchar_t> to_str(int val) {
		return to_wstring(val);
	}

	template<class CharType>
	inline bool matchesState(StateTN &st, CharType chr) {
		return (!st.isfunc && st.chr == chr) ||
		       (st.isfunc && st.func(chr));
	}

	template<class CharType>
	size_t AbstractFSMT::addState(CharType chr) {
		size_t id = states.size();
		states.emplace_back(new StateT{id, chr, CheckFuncT(), false, 0, unique_ptr<NextState>()});
		return id;
	}

	template<class CharType>
	size_t AbstractFSMT::addState(CheckFuncT func) {
		size_t id = states.size();
		states.emplace_back(new StateT{id, (CharType) 0, func, true, 0, unique_ptr<NextState>()});
		return id;
	}

#ifndef TERMGREP_NO_GVPP

	template<class CharType>
	unique_ptr<Graph<CharType>> AbstractFSMT::getGraph() {
		unique_ptr<Graph<CharType>> gptr(new Graph<CharType>());
		Graph<CharType> &g = *gptr;
		g.set(AttrType::GRAPH, CW("splines"), CW("line"))
				.set(AttrType::GRAPH, CW("rankdir"), CW("LR"))
				.set(AttrType::GRAPH, CW("outputorder"), CW("edgesfirst"))
				.set(AttrType::NODE, CW("shape"), CW("circle"))
				.set(AttrType::NODE, CW("style"), CW("filled"))
				.set(AttrType::NODE, CW("fillcolor"), CW("\"#FFFFFF\""))
				.set(AttrType::NODE, CW("color"), CW("\"#808080\""))
				.set(AttrType::NODE, CW("width"), CW("0.3"))
				.set(AttrType::NODE, CW("height"), CW("0.3"))
				.set(AttrType::NODE, CW("fixedsize"), CW("true"));
		auto &root = g.addNode(CW("0"), CW("Start"))
					.set(CW("fixedsize"), CW("false"))
					.set(CW("shape"), CW("doubleoctagon"))
					.set(CW("fillcolor"), CW("\"#AAAAFF\""));
		map<strtype, size_t> nodeDepth;
		vector<Edge<CharType>*> edges;
		nodeDepth[root.getId()] = 0;
		function<void(StatePtr, Node<CharType> &, size_t)> addRecurse =
				[&](StatePtr cur, Node<CharType> &prev, size_t depth) -> void {
					Node<CharType> *curNode = &root;
					auto toLabel = [](CharType chr) -> strtype {
						return strtype(CW("'")) + chr + strtype(CW("'"));
					};
					if (cur->id != 0) {
						strtype label = cur->isfunc ?
										cur->func.label :
										toLabel(cur->chr);
						if (cur->termid != 0) {
							label = to_str<CharType>(cur->id) + CW(": ") + label;
							curNode = &g.addNode(to_str<CharType>((int) cur->id),
							                     label + CW("\\n") +
							                     this->getTerm(cur->termid));
							curNode->
								 set(CW("shape"), CW("rect"))
								 .set(CW("color"), CW("black"))
								 .set(CW("fillcolor"), CW("lightgrey"))
								 .set(CW("fixedsize"), CW("false"));
						} else
							curNode = &g.addNode(
									to_str<CharType>((int) cur->id), label);

						auto &ed = g.addEdge(prev, *curNode);
						edges.push_back(&ed);
						nodeDepth.insert(make_pair(curNode->getId(), depth));
					}
					for (auto *nxt = cur->next.get(); nxt != nullptr; nxt = nxt->next.get()) {
						auto nxtId = to_str<CharType>((int) nxt->state->id);
						if (!g.hasNode(nxtId))
							addRecurse(nxt->state, *curNode, depth + 1);
						else {
							auto &ed = g.addEdge(*curNode, g.getNode(nxtId));
							edges.push_back(&ed);
							if (depth+1 < nodeDepth[g.getNode(nxtId).getId()]) {
								nodeDepth[g.getNode(nxtId).getId()] = depth + 1;
							}
						}
					}
				};
		addRecurse(this->getRoot(), root, 0);

		for (auto edge : edges) {
			auto frDep = nodeDepth[edge->getFrom().getId()],
				toDep = nodeDepth[edge->getTo().getId()];
			if (frDep >= toDep)
				edge->set(CW("constraint"), CW("false"))
					.set(CW("style"), CW("dashed"))
					.set(CW("splines"), CW("false"));
		}

		return gptr;
	}

#endif

	template<class CharType>
	size_t TermGrepT::addTerm(strtype term, bool bound) {
		size_t tid = this->terms.size();
		_terms.push_back(term);
		if (bound)
			term = wordBoundary<CharType>()+ term +wordBoundary<CharType>();
		if (term.length() > longestTerm)
			longestTerm = term.length();
		addStates(this->getRoot(), term.c_str());
		return tid;
	}

	template<class CharType>
	const strtype &AbstractFSMT::getTerm(size_t id) {
		return terms[id];
	}

	template<class CharType>
	CheckFunc<CharType> checkWordBoundary() {
		locale loc;
		return CheckFunc<CharType>(CW("\\\\b"), [loc](char c) -> bool {
			return c == wordBoundary<CharType>() || isspace(c, loc) || ispunct(c, loc);
		});
	}

	template<class CharType>
	void TermGrepT::addStates(StatePtr from, const CharType *chars) {
		CharType chr = tolower(chars[0]);

		if (chr == (CharType) 0) {
			from->termid = this->terms.size() - 1;
			return;
		}

		StatePtr nextState;
		for (auto *nxt = from->next.get(); nxt; nxt = nxt->next.get()) {
			if (matchesState<>(*nxt->state, chr)) {
				nextState = nxt->state;
				break;
			}
		}
		if (nextState == nullptr) {
			auto tid = (chr == wordBoundary<CharType>()) ? this->addState(checkWordBoundary<CharType>()) :
					this->addState(chr);
			nextState = this->states[tid];
			//from->next->push_back(nextState);
			unique_ptr<NextStateTN> *last = &from->next;
			while (*last)
				last = &(*last)->next;
			last->reset(new NextStateTN{nextState, unique_ptr<NextStateTN>()});
		}

		addStates(nextState, chars + 1);
	}

	template<class CharType>
	void TermGrepT::Matcher::feed(CharType chr) {
		chr = tolower(chr);
		StatePtr nextState;
		for (auto *nxt = curstate->next.get(); nxt; nxt = nxt->next.get())
			if (matchesState<CharType>(*nxt->state, chr))
				nextState = nxt->state;
		if (nextState != nullptr) {
			curstate = nextState;
		} else // No matching transition = return to root
			curstate = this->getRoot();
		if (curstate->termid != 0) {
			auto startPos = curPos - this->getTerm(curstate->termid).length() + 1;
			candidates.remove_if([&](const Match &m)
					{ return m.startPos >= startPos; });
			candidates.push_back(Match(curstate->termid,
						startPos, this->getTerm(curstate->termid)));
			// out() << "Candidate match : "<< candidates.back().term
			// 		<< "("<< candidates.back().termid <<")" << endl;
			if (nextCheck == 0)
				nextCheck = longestTerm;
		}
		if (nextCheck > 0 && --nextCheck == 0) {
			auto candit = candidates.begin();
			while (candit != candidates.end()) {
				if (curPos - candit->startPos >= longestTerm) {
					auto accepted = candit++; // Store iterator then  move it on
					// out<CharType>() << "Validating match : "<< accepted->term << endl;
					matches.splice(matches.end(), candidates,
									accepted, next(accepted));
				} else
					candit ++;
			}
			nextCheck = (candidates.size() > 0) ? longestTerm : 0;
		}
		curPos ++;
	}

	template<class CharType>
	void TermGrepT::Matcher::end() {
		feed((CharType)'\t');
		matches.splice(matches.end(), candidates);
	}

	template<class CharType>
	void TermGrepT::Matcher::feed(const CharType *chrs, size_t n) {
		for (size_t i = 0; i < n; i ++) {
			feed(chrs[i]);
		}
	}

	template<class CharType>
	map<size_t, size_t> TermGrepT::Matcher::getTermidOccurences() {
		map<size_t, size_t> occurences;
		for (size_t i = 0; i < this->getTerms().size() -1; i ++)
			occurences.insert(make_pair(i, 0));
		getTermidOccurences(occurences);
		return occurences;
	}

	template<class CharType>
	map<strtype, size_t> TermGrepT::Matcher::getTermOccurences() {
		map<strtype, size_t> occurences;
		for (size_t i = 0; i < this->getTerms().size() -1; i ++)
			occurences.insert(make_pair(this->getTerm(i), 0));
		getTermOccurences(occurences);
		return occurences;
	}

	template<class CharType>
	map<size_t, size_t> &TermGrepT::Matcher::getTermidOccurences
			(map<size_t, size_t> &occurences) {
		for(Match &m : matches) {
			occurences[m.termid] ++;
		}
		return occurences;
	}

	template<class CharType>
	map<strtype, size_t> &TermGrepT::Matcher::getTermOccurences
			(map<strtype, size_t> &occurences) {
		for(Match &m : matches) {
			occurences[m.term] ++;
		}
		return occurences;
	}

	/*!
	 * \brief Builds a Matcher using the Powerset Construction method.
	 * the parent TermGrep's states are considered to have an implicit empty
	 * transition to the Start node, making the FSM nondeterministic. Then we
	 * use the method to make it deterministic again.
	*/
	template<class CharType>
	TermGrepT::Matcher::Matcher(TermGrep &grep) :
			AbstractFSMT(grep._terms), grep(grep),
			curstate(nullptr), longestTerm(grep.longestTerm) {
		struct stateid {
			CharType chr;
			CheckFuncT func;
			stateid(const StatePtr st) :
					chr(st->isfunc ? (CharType) 0 : st->chr),
					func(st->isfunc ? st->func : CheckFuncT()) {}
			bool operator==(const stateid &st) {
				return chr == st.chr && func.operator==(st.func);
			}
		};
		auto addStateById = [this](struct stateid sid) -> size_t {
			if (sid.chr != 0)
				return this->addState(sid.chr);
			else
				return this->addState(sid.func);
		};

		struct stateidcmp {
			bool operator() (const struct stateid &si1, const struct stateid &si2) const {
				if (si1.chr == si2.chr)
					return si1.func.label < si2.func.label;
				else if (si1.func.label == si2.func.label)
					return si1.chr < si2.chr;
				else return si1.func.label == CheckFuncT::DEFAULT_LABEL;
			}
			bool operator() (const StatePtr &si1, const StatePtr &si2) const {
				return si1->id < si2->id;
			}
		};

		typedef set<StatePtr, stateidcmp> StatePtrSet;
		StatePtrSet root;
		map<const set<size_t>, const StatePtr> newStates;
		root.insert(grep.getRoot());
		function<StatePtr(StatePtrSet)> transformRecurse = [this, &addStateById, &transformRecurse, &newStates](StatePtrSet current) -> StatePtr {
			set<size_t> curIds;
			size_t termid = 0, termlen = 0;
			for (StatePtr st : current) {
				curIds.insert(st->id);
				if (st->termid != 0 && this->grep.getTerm(st->termid).length() > termlen) {
					termid = st->termid;
					termlen = this->grep.getTerm(st->termid).length();
				}
			}

			typename map<const set<size_t>, const StatePtrTN>::iterator id;
			if ((id = newStates.find(curIds)) != newStates.end()) // State exists. Abort
				return get<1>(*id);
			auto newState = this->states[addStateById(stateid(*current.begin()))];

			newState->termid = termid;
			newStates.insert(make_pair(curIds, newState));
			map<const struct stateid, StatePtrSet, stateidcmp> nextStates;
			current.insert(this->grep.getRoot());
			for (auto c : current)
				for (auto *nxt = c->next.get(); nxt; nxt = nxt->next.get()) {
					auto sid = stateid(nxt->state);
					if (nextStates.find(sid) == nextStates.end())
						nextStates.insert(make_pair(sid, StatePtrSet()));
					get<1>(*nextStates.find(sid)).insert(nxt->state);
				}
			unique_ptr<struct NextStateT> nextPtr;
			for (auto next : nextStates) {
				auto idSet = get<1>(next);
				StatePtr st = transformRecurse(idSet);
				nextPtr.reset(new NextStateTN{st});
				nextPtr->next.swap(newState->next);
				newState->next.swap(nextPtr);
			}
			return newState;
		};
		transformRecurse(root);
		reset();
	}

	template<class CharType>
	void TermGrepT::Matcher::reset() {
		curstate = this->getRoot();
		matches.clear();
		candidates.clear();
		feed((CharType)'\t');
		nextCheck = 0;
	}

	template<class CharType>
	unique_ptr<typename TermGrepT::Matcher> TermGrepT::makeChecker() {
		return move(unique_ptr<Matcher>(new TermGrepT::Matcher(*this)));
	}

	template class AbstractFSM<char>;
	template class TermGrep<char>;

	template class AbstractFSM<wchar_t>;
	template class TermGrep<wchar_t>;
}
