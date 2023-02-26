/*
 * cpptexnumber
 *
 * Replaces latex labels and references
 *
 * Compile with g++ -std=c++11 -O3 cpptexnumber.cpp -o cpptexnumber
 * Use the -DDO_NOT_IGNORE_COMMENTS flag
 * if you do not want to ignore the comments (ignored by default)
 *
 * type ./cpptexnumber --help for help
 */

/*
 * Copyright (c) 2013 - 2021 Vlad Gheorghiu. All rights reserved.
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

using label_idx_map = std::map<std::string, std::size_t>;
using idx_label_map = std::map<std::size_t, std::string>;

// how to use the program
std::string usage(const std::string& name) {
    std::string result;
    result += "Usage: " + name +
              " <pattern> <replacement> [ignore_comments ON(default)/OFF] "
              "[log_file]\n\n";
    result += "Renumbers LaTeX equations. "
              "The program reads from the standard input and writes to the "
              "standard output.\n";
    result += "Warnings and errors are output to the standard error stream.";
    return result;
}

// builds a map sorted by value from a standard map sorted by key
idx_label_map map_by_value(const label_idx_map& labels) {
    idx_label_map result;
    for (auto&& elem : labels)
        result[elem.second] = elem.first;
    return result;
}

// logs the labels replacement map with proper alignment and spacing
void log(std::ostream& os, const label_idx_map& labels,
         const std::string& pattern_out) {
    for (auto&& elem : map_by_value(labels)) {
        os << elem.second << " -> " << pattern_out << elem.first << '\n';
    }
}

// builds the labels map
label_idx_map build_labels(std::stringstream& input,
                           const std::string& pattern_in,
                           bool ignore_comments) {
    label_idx_map result;
    std::regex re{R"(\\label\{)" + pattern_in + R"(.*?\})"};
    std::smatch labels;
    std::string line;
    for (std::size_t line_no = 1, label_no = 1; std::getline(input, line);
         ++line_no) {
        if (ignore_comments) {
            std::size_t pos = line.find('%');
            if (pos != std::string::npos)
                line.erase(pos);
        }
        // search regex in current line
        while (std::regex_search(line, labels, re)) {
            // matches
            for (auto&& label : labels) {
                // strip "content" from \label{content}
                std::string label_content =
                    label.str().substr(7, label.str().size() - 8);
                if (result.find(label_content) != result.end()) {
                    std::cerr << "PARSING WARNING: Duplicate \\label{"
                              << label_content << "} on line " << line_no
                              << '\n';
                    break;
                }
                result[label_content] = label_no++;
            }
            // continue with the remainder of the line
            line = labels.suffix().str();
        }
    }
    return result;
}

// replaces all matching references in the current line up to and including pos
void replace_refs_in_line(std::string& line, const std::string& pattern_in,
                          const std::string& pattern_out,
                          const label_idx_map& labels,
                          const std::vector<std::string>& refs,
                          std::size_t line_no, bool ignore_comments) {
    std::size_t pos_comment = ignore_comments ? line.find('%') : line.size();
    // for all reference types
    for (auto&& elem : refs) {
        std::string::size_type start;
        std::string::size_type end = 0;
        while (true) {
            start = line.find(elem, end);
            if (start == std::string::npos || start > pos_comment)
                break;
            end = line.find('}', start);
            if (end == std::string::npos) {
                std::cerr << "PARSING ERROR: No matching '}'"
                          << " on line " << line_no << '\n';
                std::exit(EXIT_FAILURE);
            }

            auto start_replace = start + elem.size();
            auto count = end - (start + elem.size());
            std::string ref = line.substr(start_replace, count);
            auto found = labels.find(ref);
            if (found != labels.end()) {
                // construct the new reference
                std::string new_ref =
                    pattern_out + std::to_string(labels.at(ref));
                line.replace(start_replace, count, new_ref);
            }
            // the reference starts with pattern_in but it is not in the labels
            // map
            else if (ref.compare(0, pattern_in.size(), pattern_in) == 0) {
                std::cerr << "PARSING WARNING: Undefined " << elem << ref
                          << "} on line " << line_no << '\n';
            }
        }
    }
}

// main program
int main(int argc, char* argv[]) {
    if (argc > 1 && ((std::string(argv[1]).find("help") != std::string::npos) ||
                     std::string(argv[1]).find('?') != std::string::npos)) {
        std::cout << usage(argv[0]) << '\n';
        return 0;
    }
    if (argc < 3) {
        std::cerr << usage(argv[0]) << '\n';
        std::exit(EXIT_FAILURE);
    }

    std::string pattern_in = argv[1];  // pattern to replace
    std::string pattern_out = argv[2]; // replacement
    bool ignore_comments = true;       // ignore LaTeX comments

    // we have ignore_comments flag
    if (argc > 3) {
        std::string flag{argv[3]};
        std::transform(flag.begin(), flag.end(), flag.begin(), ::toupper);
        if (flag == "OFF")
            ignore_comments = false;
    }

    // modify as needed
    std::vector<std::string> refs = {"\\label{", "\\eqref{", "\\ref{",
                                     "\\pageref{"};
    std::stringstream input;
    input << std::cin.rdbuf();
    auto labels = build_labels(input, pattern_in, ignore_comments);
    if (labels.empty()) {
        std::cerr << "PARSING ERROR: pattern <" << pattern_in
                  << "> not found\n";
        std::exit(EXIT_FAILURE);
    }

    input.clear();  // clear the eofbit flag
    input.seekg(0); // rewind the stream

    // replace all matching references line by line
    std::size_t line_no = 1;
    for (std::string line; std::getline(input, line);) {
        replace_refs_in_line(line, pattern_in, pattern_out, labels, refs,
                             line_no++, ignore_comments);
        std::cout << line << '\n'; // write it to output
    }

    // write to log file (if any)
    if (argc > 4) {
        std::ofstream f_log;
        f_log.open(argv[4]);
        if (!f_log) {
            std::cerr << "Cannot write to the " << argv[3] << " log file\n";
            std::exit(EXIT_FAILURE);
        }
        log(f_log, labels, pattern_out);
    }
}
