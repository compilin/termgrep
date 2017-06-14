Termgrep
=========

Termgrep is a library and command-line utility for finding and counting occurences of terms in documents. More specifically, it is designed to look for many terms at the same time, kind of a regex with many |, except it returns a map of {term-> number of occurences} direclty. For more information about the motivation and the mechanisms involved, see [this blog post](https://gears-of-data.science/termgrep-regular-expressions-from-scratch/).

Building
---------

This repository has git submodules for dependencies, so remember to clone it with the --recursive option.

    git clone --recursive https://github.com/neurovertex/termgrep
    mkdir termgrep/build && cd termgrep/build
    cmake .. && make

Usage
-----

Termgrep requires a list of terms and a list of documents. The simplest command-line would be: `./termgrep_main document1.txt document2.txt document3.txt --terms terms.txt --output-file=output.json`

Terms must be supplied as a newline-separated list of terms, either after the --terms parameter or to the standard input with the --terms-stdin parameter.

The document list can be given as a potitional parameters, or fed to the standard input (better suited for large corpora) as a newline-separated list with --file-list-stdin (obviously mutually exclusive with --terms-stdin). You can also prepend an ID/name and a separator (specified with --fileid-separator) to the path, which will be used in the output to identify the file. The resulting command line would look like:

    ./termgrep_main --terms=terms.txt --termid-separator=: docA:./path/A/doc.txt docB:./path/B/doc.txt docC:./path/C/doc.txt --output-file=output.json

The JSON output will look like so:

```json
[
    {
        "file": "docA",
        "matches": {
            "foo": 0, "bar": 2, "baz": 1
        }
    },
    {
        "file": "docB",
        "matches": {
            "foo": 1, "bar": 5, "baz": 3
        }
    },
    {
        "file": "docC",
        "matches": {
            "foo": 0, "bar": 3, "baz": 0
        }
    }
]```
