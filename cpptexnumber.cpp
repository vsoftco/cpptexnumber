// Replaces latex labels and references
// Compile with g++ -std=c++11 -O3 cpptexnumber.cpp -o cpptexnumber
// Use the -DDO_NOT_IGNORE_COMMENTS flag
// if you do not want to ignore the comments (ignored by default)

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#define DEBUG_PRINT(x) std::cout << (x) << std::endl

using label_idx_map = std::map<std::string, std::size_t>;
using label_idx_pair = std::pair<std::string, std::size_t>;
using idx_label_map = std::map<std::size_t, std::string>;

// builds a map sorted by value from a standard map sorted by key
idx_label_map by_value(const label_idx_map& labels)
{
    idx_label_map result;
    for (auto && elem : labels)
        result[elem.second] = elem.first;
    return result;
}

// displays the labels replacement map with proper alignment and spacing
void pretty_print(const label_idx_map& labels,
                  const std::string& pattern_out)
{
    auto max_length_it =
        std::max_element(labels.begin(), labels.end(),
                    [](const label_idx_pair & lhs, const label_idx_pair & rhs)
                    {
                        return lhs.first.size() < rhs.first.size();
                    }
        );
    auto max_length = max_length_it->first.size();
    for (auto && elem : by_value(labels))
    {
        std::cout << std::left << std::setw(max_length) << elem.second
                  << " -> " << pattern_out << elem.first << std::endl;
    }
}

// builds the labels map
label_idx_map build_labels(std::ifstream& ifile,
                           const std::string& pattern_in)
{
    label_idx_map result;
    std::regex re{R"(\\label\{)" + pattern_in + R"(.*?\})"};
    std::smatch labels;
    std::string line;
    std::size_t line_no = 0, label_no = 1;
    while (std::getline(ifile, line))
    {
        ++line_no;
#ifndef DO_NOT_IGNORE_COMMENTS
        std::getline(std::istringstream {line}, line, '%');
#else
        std::getline(std::istringstream {line}, line);
#endif
        // search regex in current line
        while (std::regex_search(line, labels, re))
        {
            for (auto && label : labels) // matches
            {
                // strip "content" from \label{content}
                std::string label_content =
                    label.str().substr(7, label.str().size() - 8);
                if (result.find(label_content) != result.end())
                {
                    std::cout << "PARSING ERROR: Duplicate \\label{"
                              << label_content << "} on line "
                              << line_no << std::endl;
                    std::exit(EXIT_FAILURE);
                }
                result[label_content] = label_no++;
            }
            // continue with the remainder of the line
            line = labels.suffix().str();
        }
    }
    return result;
}

// replaces all matching references in the current line
std::string replace_line(std::string line,
                         const std::string& pattern_in,
                         const std::string& pattern_out,
                         const label_idx_map& labels,
                         const std::vector<std::string>& refs,
                         std::size_t line_no)
{
    for (auto& elem : refs) // for all reference types
    {
        std::string::size_type start;
        std::string::size_type end = 0;
        while (true)
        {
            start = line.find(elem, end);
            if (start == std::string::npos)
                break;
            end = line.find("}", start);
            if (end == std::string::npos)
            {
                std::cout << "PARSING ERROR: No matching '}'"
                          << " on line " << line_no << std::endl;
                std::exit(EXIT_FAILURE);
            }

            auto start_replace = start + elem.size();
            auto count = end - (start + elem.size());
            std::string ref = line.substr(start_replace, count);
            auto found = labels.find(ref);
            if (found != labels.end())
            {
                // construct the new reference
                std::string new_ref = pattern_out +
                                      std::to_string(labels.at(ref));
                line.replace(start_replace, count, new_ref);
            }
            // the reference starts with pattern_in
            // but it is not in the labels map
            else if (ref.compare(0, pattern_in.size(), pattern_in) == 0)
            {
                std::cout << "PARSING WARNING: Undefined reference "
                          << elem << ref << "} on line " << line_no
                          << std::endl;
            }
        }
    }
    return line;
}

// replace all matching references
void replace_refs(std::ifstream& ifile,
                  std::ofstream& ofile,
                  const std::string& pattern_in,
                  const std::string& pattern_out,
                  const label_idx_map& labels,
                  const std::vector<std::string>& refs)
{
    std::string line, line_no_comments;
    std::size_t line_no = 0;

    while (std::getline(ifile, line))
    {
        ++line_no;
        auto ss = std::istringstream{line};
        // get the line without comments
#ifndef DO_NOT_IGNORE_COMMENTS
        std::getline(ss, line_no_comments, '%');
#else
        std::getline(ss, line_no_comments);
#endif
        if (line == line_no_comments) // no comments
        {
            line = {};
        }
        else // we have a comment
        {
            std::getline(ss, line);
            line = '%' + line;
        }
        line_no_comments = replace_line(line_no_comments, pattern_in,
                                        pattern_out, labels, refs, line_no);
        ofile << line_no_comments + line + '\n'; // write it to output

    }
}

int main(int argc, char* argv[])
{
    if (argc != 5)
    {
        std::cout << "Usage: " << argv[0]
                  << " <in.tex> <out.tex> <pattern> <replacement>"
                  << std::endl;
        std::cout << "(c) Vlad Gheorghiu 2015, vsoftco@gmail.com"
                  << std::endl;
        std::exit(EXIT_FAILURE);
    }
    std::ifstream ifile(argv[1]);
    if (!ifile)
    {
        std::cout << "SYSTEM ERROR: Can not open the input file "
                  << argv[1] << std::endl;
        std::exit(EXIT_FAILURE);

    }
    if (std::string(argv[1]) == argv[2])
    {
        std::cout << "SYSTEM ERROR: "
                  << "The output file must be different from the input file!"
                  << std::endl;
        std::exit(EXIT_FAILURE);
    }
    std::ofstream ofile(argv[2]);
    if (!ofile)
    {
        std::cout << "SYSTEM ERROR: Can not open the output file "
                  << argv[2] << std::endl;
        std::exit(EXIT_FAILURE);
    }
    std::string pattern_in = argv[3];
    std::string pattern_out = argv[4];
    // Modify as needed
    std::vector<std::string> refs = {"\\label{",
                                     "\\eqref{",
                                     "\\ref{",
                                     "\\pageref{"
                                    };
    auto labels = build_labels(ifile, pattern_in);
    if (labels.size() == 0)
    {
        std::cout << "PARSING ERROR: pattern <" << pattern_in
                  << "> not found" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    ifile.clear();
    ifile.seekg(0);
    replace_refs(ifile, ofile, pattern_in, pattern_out, labels, refs);

    std::cout << "REPLACED: " << std::endl;
    pretty_print(labels, pattern_out);
}
