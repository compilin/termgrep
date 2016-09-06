#include "termgrep.hpp"
#include <json.hpp>
#include <vector>
#include <iostream>
#include <codecvt>
#include <boost/algorithm/string/predicate.hpp>

namespace termgrep {
    using json = nlohmann::json;

    template<class CharT>
    inline std::string toNarrowString(std::basic_string<CharT> in);

    template<> inline std::string toNarrowString(std::string in) { return in; }
    template<> inline std::string toNarrowString(std::wstring in) {
    	return std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(in);
    }

    template <class CharType>
    class OutputFormat;

    template<class CharType>
    ostream &operator<<
        (ostream &os, const OutputFormat<CharType> &frmt);

    struct OutputOptions {
        bool outputTermids = true;
        std::string separator = ",";
    };

    enum Formats {
        JSON,
        CSV,
        TSV
    };

    template <class CharType>
    class OutputFormat {
        friend ostream &operator<<<CharType>
            (ostream &os, const OutputFormat<CharType> &frmt);
    public:
        static std::unique_ptr<OutputFormat<CharType>>
            makeOutput(Formats format, OutputOptions options);
        virtual void addFileResult(std::string fname,
            typename TermGrep<CharType>::Matcher &matcher) = 0;
    protected:
        virtual void write(std::ostream &os) const = 0;
        OutputFormat(OutputOptions options) : options(options) {}
        const OutputOptions options;
    private:
    };


    template <class CharType = DefaultCharType>
    class JSONOutputFormat : public OutputFormat<CharType> {
        friend std::unique_ptr<OutputFormat<CharType>>
            OutputFormat<CharType>::makeOutput(Formats format, OutputOptions options);
    public:
        virtual void addFileResult(std::string fname,
            typename TermGrep<CharType>::Matcher &matcher) override {
            json fileData;
            if (this->options.outputTermids) {
    			auto occmap = matcher.getTermidOccurences();
    			for (size_t i = 1; i < matcher.getTerms().size(); i ++)
    				fileData.push_back(occmap[i]);
            } else {
                auto occMap = matcher.getTermOccurences();
                for (auto &match : occMap)
                    fileData[toNarrowString(match.first)] = match.second;
            }
            data.push_back(json::object({
                {"file", fname},
                {"matches", fileData}
            }));
        }
    private:
        void write(std::ostream &os) const override {
            os << data;
        }
        json data;
        JSONOutputFormat(OutputOptions options) : OutputFormat<CharType>(options) {}
    };

    template <class CharType = DefaultCharType>
    class CSVOutputFormat : public OutputFormat<CharType>  {
        friend std::unique_ptr<OutputFormat<CharType>>
            OutputFormat<CharType>::makeOutput(Formats format, OutputOptions options);
    public:
        virtual void addFileResult(std::string fname,
            typename TermGrep<CharType>::Matcher &matcher) override {
            json fileData;
            terms = &matcher.getTerms();
			auto occmap = matcher.getTermidOccurences();
			for (size_t i = 1; i < terms->size(); i ++)
				fileData.push_back(occmap[i]);
            data.push_back(json::object({
                {"file", fname},
                {"matches", fileData}
            }));
        }
    private:
        const vector<strtype> *terms = nullptr;
        json data;
        void write(std::ostream &os) const override {
            if (terms != nullptr) {
                size_t cnt = 0;
                for (auto &term : *terms)
                    if (cnt ++ == 0)
                        os << "filename";
                    else
                        os << this->options.separator << toNarrowString(term);
                os << endl;
                for (auto &file : data) {
                    os << file["file"];
                    auto matches = file["matches"];
                    for (size_t i = 0; i < terms->size()-1; i ++)
                        os << this->options.separator << matches[i];
                    os << endl;
                }
            }
        }
        CSVOutputFormat(OutputOptions options) : OutputFormat<CharType>(options) {}
    };

    template <class CharType>
    std::unique_ptr<OutputFormat<CharType>>
        OutputFormat<CharType>::makeOutput(Formats format, OutputOptions options) {
        switch (format) {
        case JSON:
            return std::unique_ptr<OutputFormat<CharType>>
                (new JSONOutputFormat<CharType>(options));
        case CSV:
            return std::unique_ptr<OutputFormat<CharType>>
                (new CSVOutputFormat<CharType>(options));
        case TSV:
            options.separator = "\t";
            return std::unique_ptr<OutputFormat<CharType>>
                (new CSVOutputFormat<CharType>(options));
        default:
            throw std::runtime_error("Unknown format");
        }
    }

    Formats getFormatByName(string format) {
        if (boost::iequals(format, "json"))
            return Formats::JSON;
        if (boost::iequals(format, "csv"))
            return Formats::CSV;
        if (boost::iequals(format, "tsv"))
            return Formats::TSV;
        throw std::runtime_error("Unknown format");
    }

    template<class CharType>
    std::ostream &operator<<
        (std::ostream &os, const OutputFormat<CharType> &frmt) {
        frmt.write(os);
        return os;
    }

}
