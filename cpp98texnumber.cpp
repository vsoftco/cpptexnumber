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
#include <sstream>
#include <string>
#include <vector>

#define DEBUG_PRINT(x) std::cout << (x) << std::endl

// structures
typedef std::map<std::string, std::size_t> label_idx_map;
typedef std::map<std::size_t, std::string> idx_label_map;
typedef std::pair<std::string, std::size_t> label_idx_pair;
typedef std::vector<std::string>::const_iterator vector_string_const_it;

// iterators
typedef label_idx_map::const_iterator label_idx_map_const_it;
typedef idx_label_map::const_iterator idx_label_map_const_it;
typedef std::string::size_type str_size_type;

// builds a map sorted by value from a standard map sorted by key
idx_label_map by_value(const label_idx_map& labels)
{
    idx_label_map result;
    for (label_idx_map_const_it it = labels.begin();
            it != labels.end(); ++it)
    {
        result[it -> second] = it -> first;
    }
    return result;
}

// displays the labels replacement map with proper alignment and spacing
void pretty_print(const label_idx_map& labels,
                  const std::string& pattern_out)
{
    std::size_t max_length = 0;
    for (label_idx_map_const_it it = labels.begin();
            it != labels.end(); ++it)
    {
        if (it -> first.size() > max_length)
            max_length = it -> first.size();
    }
    idx_label_map tmp = by_value(labels);
    for (idx_label_map_const_it it = tmp.begin();
            it != tmp.end(); ++it)
    {
        std::cout << std::left << std::setw(max_length) << it -> second
                  << " -> " << pattern_out << it -> first << std::endl;
    }
}

label_idx_map build_labels(std::ifstream& ifile,
                           const std::string& pattern_in)
{
    label_idx_map result;
    /*std::regex re{R"(\\label\{)" + pattern_in + R"(.*?\})"};
    std::smatch labels;*/

    std::string line;
    std::size_t line_no = 0, label_no = 1;

    while (std::getline(ifile, line))
    {
        ++line_no;
        std::istringstream iss(line);
#ifndef DO_NOT_IGNORE_COMMENTS
        std::getline(iss, line, '%');
#else
        std::getline(iss, line);
#endif
        str_size_type start;
        str_size_type end = 0;
        std::string elem = "\\label{" + pattern_in;
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
            str_size_type start_label = start + 7;
            str_size_type count = end - start_label;
            result[line.substr(start_label, count)] = label_no++;
        }
    }
    return result;
}

std::string replace_line(std::string line,
                         const std::string& pattern_in,
                         const std::string& pattern_out,
                         const label_idx_map& labels,
                         const std::vector<std::string>& refs,
                         std::size_t line_no)
{
    for (vector_string_const_it it = refs.begin(); it != refs.end(); ++it)
    {
        str_size_type start;
        str_size_type end = 0;
        while (true)
        {
            start = line.find(*it, end);
            if (start == std::string::npos)
                break;
            end = line.find("}", start);
            if (end == std::string::npos)
            {
                std::cout << "PARSING ERROR: No matching '}'"
                          << " on line " << line_no << std::endl;
                std::exit(EXIT_FAILURE);
            }

            str_size_type start_replace = start + it -> size();
            str_size_type count = end - (start + it -> size());
            std::string ref = line.substr(start_replace, count);
            label_idx_map_const_it found = labels.find(ref);
            if (found != labels.end())
            {
                // construct the new reference
                std::ostringstream tmp;
                tmp << labels.at(ref);
                std::string new_ref = pattern_out + tmp.str();
                line.replace(start_replace, count, new_ref);
            }
            // the reference starts with pattern_in
            // but it is not in the labels map
            else if (ref.compare(0, pattern_in.size(), pattern_in) == 0)
            {
                std::cout << "PARSING WARNING: Undefined reference "
                          << *it << ref << "} on line " << line_no
                          << std::endl;
            }
        }
    }
    return line;
}

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
        std::istringstream iss(line);
        // get the line without comments
#ifndef DO_NOT_IGNORE_COMMENTS
        std::getline(iss, line_no_comments, '%');
#else
        std::getline(iss, line_no_comments);
#endif
        if (line == line_no_comments) // no comments
        {
            line.clear();
        }
        else // we have a comment
        {
            std::getline(iss, line);
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
    std::vector<std::string> refs;
    refs.push_back("\\label{");
    refs.push_back("\\eqref{");
    refs.push_back("\\ref{");
    refs.push_back("\\pageref{");
    label_idx_map labels = build_labels(ifile, pattern_in);
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
