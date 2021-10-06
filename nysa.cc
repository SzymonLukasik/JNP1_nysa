#include <iostream>
#include <stdio.h>
#include <vector>
#include <string>
#include <regex>

using namespace std;

enum input_t{INPUT, NOT, XOR, AND, NAND, OR, NOR};

int main() {
    string s;
    regex operators_r = regex("(NOT|XOR|AND|NAND|OR|NOR)");
    regex number_r = regex("[^[1-9]\\d{0,8}");
    regex whitespace_r = regex("\\s+");

    while (getline(cin, s)) {
        vector<string> t;
        copy(sregex_token_iterator(s.begin(), s.end(), whitespace_r, -1), sregex_token_iterator(),
             back_inserter(t));
        cout << t.size() << '\n';
        for(size_t i = 0; i < t.size(); i++)
            cout << t[i] << '\n';
        cout << "\n";
    }
}