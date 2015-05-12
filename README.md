# cpptexnumber
## C++11 program for renumbering latex references

### Building without a build system
  
Compile with 
    
    g++ -std=c++11 -O3 cpptexnumber.cpp -o cpptexnumber

Use the `-DDO_NOT_IGNORE_COMMENTS` flag 
if you do not want to ignore the comments in the latex source file 
(ignored by default). A `CMakeLists.txt` is provided for automatic 
building via [cmake](http://www.cmake.org/) and 
[g++](https://gcc.gnu.org/)/[clang++](http://clang.llvm.org/). 
The compiler must be C++11 compliant and must fully support the 
`std::regex` Regular Expression part of the C++ Standard Library. 
I recommend `[g++](https://gcc.gnu.org/)` version 4.9
or later, or [clang++](http://clang.llvm.org/) version 3.5 or later.

### Building with [cmake](http://www.cmake.org/):

I recommend an out-of-source build, i.e., from the root of the project type

    mkdir ./build
    cd ./build
    cmake ..
    make

Use `cmake -DDO_NOT_IGNORE_COMMENTS=ON ..` if you do not want to 
ignore the comments in the latex source file (ignored by default).

The above commands build the relase version (default) 
executable `cpptexnumber`, from the source file `./cpptexnumber.cpp`.

### Usage: 
    
        cpptexnumber in.tex out.tex pattern replacement 

### Description: 

Replaces all labels in `in.tex` that match 
a given input pattern, such as `\label{pattern*}` 
and all corresponding references (`\ref`, `\eqref`, `\pageref`) 
with renumbered ones `replacement+idx`, where `idx` counts 
the matching `\label{pattern*}` order of appearance, 
i.e. equals 1 for the first `\label` that matches the input pattern, 
2 for the second etc. The `in.tex` is left unchanged and the 
modifications are written to `out.tex`.

### Example: 

Consider a latex file `source.tex` having three labels, named, 
in order of appearance, `\label{eqn5}`, `\label{eqn_important}` 
and lastly `\label{entropy}`. Then the command 

        cpptexnumber in.tex out.tex eqn Eqn 

will replace `\label{eqn5}` by `\label{Eqn1}` (and also all references 
to `eqn5` will automatically become references to `Eqn1`) and replace 
`\label{eqn_important}` by `\label{Eqn2}` (again together with 
the corresponding references). The `\label{entropy}` remains unchanged 
since it does not match the input pattern `\label{eqn*}`. 
The changes are saved to the file `out.tex`.