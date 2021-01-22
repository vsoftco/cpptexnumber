# cpptexnumber

Renumbers LaTeX references.

### Building without a build system

Compile with

    g++ -std=c++11 -O3 src/cpptexnumber.cpp -o cpptexnumber

A `CMakeLists.txt` is provided for automatic building
via [CMake](http://www.cmake.org/) and
[g++](https://gcc.gnu.org/)/[clang++](http://clang.llvm.org/). The compiler must
be C++11 compliant and must fully support the `std::regex` Regular Expression
library of the C++ Standard Library. I recommend [g++](https://gcc.gnu.org/)
version 4.9 or later, or [clang++](http://clang.llvm.org/) version 3.5 or later.

### Building with [CMake](http://www.cmake.org/)

I recommend an out-of-source build, i.e., from the root of the project type

    mkdir ./build
    cd ./build
    cmake ..
    make

The above commands build the executable `cpptexnumber`.

### Usage

The program reads from the standard input and writes to the standard output.

    cpptexnumber <in.tex >out.tex pattern replacement [ignore_comments ON(default)/OFF] [log_file]

### Description

Replaces all labels from the input stream (in this case redirected
from `in.tex`) that match a given input pattern, such as `\label{pattern*}`
and all corresponding references (`\ref`, `\eqref`, `\pageref`) with renumbered
ones `replacement+idx`, where `idx` counts the matching `\label{pattern*}` order
of appearance, i.e., equals 1 for the first `\label` that matches the input
pattern, 2 for the second etc. The output is written to the standard output (in
this case redirected to `out.tex`). Optionally, writes the replacement pattern
sequence to `log_file`. Warnings and errors are output to the standard error
stream.

### Example

Consider a LaTeX file `in.tex` having three labels, named, in order of
appearance, `\label{eqn5}`, `\label{eqn_important}`and lastly `\label{entropy}`.
Then the command

    cpptexnumber <in.tex >out.tex eqn Eqn ON log.txt

will replace `\label{eqn5}` by `\label{Eqn1}` (and also all references to `eqn5`
will automatically become references to `Eqn1`) and replace
`\label{eqn_important}` by `\label{Eqn2}` (again together with the corresponding
references), while ignoring any LaTeX comments. The `\label{entropy}` remains
unchanged since it does not match the input pattern `\label{eqn*}`. The changes
are saved to the file `out.tex`, and the replacement pattern sequence is written
to the file `log.txt`.
