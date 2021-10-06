#include <iostream>
#include <stdio.h>
#include <vector>
#include <string>
#include <regex>

using namespace std;

enum input_t {
    INPUT, NOT, XOR, AND, NAND, OR, NOR
};

static void report_error(size_t line_nr, string line) {
    cout << "Error in line " + to_string(line_nr) + ": " + line + "\n";
}

static vector<string> tokenize_line(string line) {
    regex whitespace_regex = regex("\\s+");
    vector<string> container;
    copy(sregex_token_iterator(line.begin(), line.end(), whitespace_regex, -1),
         sregex_token_iterator(), back_inserter(container));
    return container;
}

static vector<vector<string>> get_input() {
    regex line_regex = regex(
            "\\s*(NOT|XOR|AND|NAND|OR|NOR)(\\s+[1-9]\\d{0,8}){2,}\\s*"
    );
    string line;
    size_t line_nr = 1;
    vector<vector<string>> lines;

    while (getline(cin, line)) {
        if(!regex_match(line, line_regex))
            report_error(line_nr, line);
        else
            lines.push_back(tokenize_line(line));
        line_nr++;
    }
    return lines;
}

int main() {
    vector<vector<string>> lines;
    lines = get_input();
    for(auto & line: lines) {
        for (auto &s: line)
            cout << s << " ";
        cout << "\n";
    }
}