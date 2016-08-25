Termgrep
=========

Termgrep is a library and command-line utility for finding and counting occurences of terms in documents. More specifically, it is designed to look for many terms at the same time, kind of a regex with many |, except it returns a map of {term-> number of occurences} direclty. For more information about the motivation and the mechanisms involved, see [this blog post](https://gears-of-data.science/termgrep-regular-expressions-from-scratch/).

Building
---------

This repository has git submodules for dependencies, so remember to clone it with the --recursive option.

    git clone --recursive https://github.com/neurovertex/termgrep
    mkdir termgrep/build && cd termgrep/build
    cmake .. && make
