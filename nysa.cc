#include <iostream>
#include <stdio.h>
#include <vector>
#include <string>
#include <regex>

using namespace std;

enum input_t {
    INPUT, NOT, XOR, AND, NAND, OR, NOR
};

static void report_error(size_t line_nr, const string &line) {
    cout << "Error in line " + to_string(line_nr) + ": " + line + "\n";
}

static regex get_line_regex() {
    string number_p = "(\\s+[1-9]\\d{0,8})";
    string not_p = "NOT" + number_p + "{1}";
    string xor_p = "XOR" + number_p + "{2}";
    string others_p = "(AND|NAND|OR|NOR)" + number_p + "{2,}";
    string operator_p = "(" + number_p + "|" + xor_p + "|" + others_p + ")";
    string line_pattern = "\\s*" + operator_p + "\\s*";
    regex line_regex = regex(line_pattern);
    return line_regex;
}

static vector<string> tokenize_line(string line) {
    regex whitespace_regex = regex("\\s+");
    vector<string> container;

    copy(sregex_token_iterator(line.begin(), line.end(), whitespace_regex, -1),
         sregex_token_iterator(), back_inserter(container));
    return container;
}

static vector<vector<string>> get_input() {
    regex line_regex = get_line_regex();
    string line;
    size_t line_nr = 1;
    vector<vector<string>> lines;

    while (getline(cin, line)) {
        if (!regex_match(line, line_regex))
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

    for (auto &line: lines) {
        for (auto &s: line)
            cout << s << " ";
        cout << "\n";
    }
}