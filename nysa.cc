#include <cstdio>
#include <iostream>
#include <regex>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;
using graph_t = unordered_map<int, vector<int>>;
enum gate_t { NOT, XOR, AND, NAND, OR, NOR };
using gate_map_t = unordered_map<int, gate_t>;

static bool ERROR_REPORTED = false;

static unordered_map<string, gate_t> string_to_gate = {
    {"NOT", NOT},   {"XOR", XOR}, {"AND", AND},
    {"NAND", NAND}, {"OR", OR},   {"NOR", NOR}};

static bool get_gate_value(gate_t gate, vector<bool> &inputs) {
  switch (gate) {
    case NOT:
      return !inputs[0];
    case XOR:
      return inputs[0] != inputs[1];
    case AND:
      return all_of(inputs.begin(), inputs.end(), [](bool i) { return i; });
    case NAND:
      return !all_of(inputs.begin(), inputs.end(), [](bool i) { return i; });
    case OR:
      return any_of(inputs.begin(), inputs.end(), [](bool i) { return i; });
    case NOR:
      return !any_of(inputs.begin(), inputs.end(), [](bool i) { return i; });
    default:
      return false;
  }
}

static void report_syntax_error(size_t line_nr, const string &line) {
  cerr << "Error in line " + to_string(line_nr) + ": " + line + "\n";
  ERROR_REPORTED = true;
}

static void report_signal_multiple_outputs_error(size_t line_nr,
                                                 int signal_nr) {
  cerr << "Error in line " + to_string(line_nr) + ": signal " +
              to_string(signal_nr) + " is assigned to multiple outputs.\n";
  ERROR_REPORTED = true;
}

static regex get_line_regex() {
  string number_p = "(\\s+[1-9]\\d{0,8})";
  string not_p = "NOT" + number_p + "{2}";
  string xor_p = "XOR" + number_p + "{3}";
  string others_p = "(AND|NAND|OR|NOR)" + number_p + "{3,}";
  string gate_p = "(" + not_p + "|" + xor_p + "|" + others_p + ")";
  string line_pattern = "\\s*" + gate_p + "\\s*";
  regex line_regex = regex(line_pattern);
  return line_regex;
}

static vector<string> tokenize_line(string line) {
  regex whitespace_regex = regex("[^\\s]+");
  vector<string> vect;

  copy(sregex_token_iterator(line.begin(), line.end(), whitespace_regex, 0),
       sregex_token_iterator(), back_inserter(vect));
  return vect;
}

static vector<int> convert_to_int_vector(vector<string>::iterator begin,
                                         vector<string>::iterator end) {
  vector<int> numbers = vector<int>();
  for (auto it = begin; it != end; it++) numbers.push_back(stoi(*it));
  return numbers;
}

static void add_gate(graph_t &graph, gate_map_t &gate_map,
                     set<int> &all_signals, set<int> &gate_signals,
                     string &line, size_t line_nr) {
  vector<string> tokenized_line = (tokenize_line(line));
  gate_t gate = string_to_gate[tokenized_line[0]];
  int gate_number = stoi(tokenized_line[1]);
  vector<int> kids_numbers =
      convert_to_int_vector(tokenized_line.begin() + 2, tokenized_line.end());

  if (graph.find(gate_number) == graph.end()) {
    graph[gate_number] = kids_numbers;
    gate_map[gate_number] = gate;
    gate_signals.insert(gate_number);
    all_signals.insert(gate_number);
    copy(kids_numbers.begin(), kids_numbers.end(),
         inserter(all_signals, all_signals.end()));
  } else
    report_signal_multiple_outputs_error(line_nr, gate_number);
}

static tuple<graph_t, gate_map_t, vector<int>> get_input() {
  regex line_regex = get_line_regex();
  string line;
  size_t line_nr = 1;
  graph_t graph = unordered_map<int, vector<int>>();
  gate_map_t gate_map = unordered_map<int, gate_t>();
  set<int> all_signals = set<int>();
  set<int> gate_signals = set<int>();
  while (getline(cin, line)) {
    if (regex_match(line, line_regex))
      add_gate(graph, gate_map, all_signals, gate_signals, line, line_nr);
    else
      report_syntax_error(line_nr, line);
    line_nr++;
  }
  vector<int> input_signals = vector<int>();
  set_difference(all_signals.begin(), all_signals.end(), gate_signals.begin(),
                 gate_signals.end(), back_inserter(input_signals));
  return make_tuple(graph, gate_map, input_signals);
}

static vector<int> get_roots(graph_t &graph) {
  unordered_map<int, bool> is_root;
  for (auto &graph_it : graph) {
    int gate_nr = graph_it.first;
    if (is_root.find(gate_nr) == is_root.end()) is_root[gate_nr] = true;
    vector<int> &kids_numbers = graph_it.second;
    for (auto &kid_number : kids_numbers) is_root[kid_number] = false;
  }
  vector<int> roots = vector<int>();
  for (auto &map_it : is_root)
    if (map_it.second) roots.push_back(map_it.first);
  return roots;
}

static bool is_sequential(const graph_t &graph, const vector<int> &roots) {
  if (roots.empty()) return true;
  enum color { VISITING, VISITED };
  unordered_map<int, color> colors = unordered_map<int, color>();
  vector<int> stack = vector<int>(roots);
  while (!stack.empty()) {
    int node = stack.back();
    if (colors.find(node) == colors.end()) {
      colors[node] = VISITING;
      if (graph.find(node) != graph.end()) {
        for (auto kid : graph.at(node))
          if (colors.find(kid) == colors.end())
            stack.push_back(kid);
          else if (colors.at(kid) == VISITING)
            return true;
      }
    } else {
      colors[node] = VISITED;
      stack.pop_back();
    }
  }
  return false;
}

static void compute_state(graph_t &graph, gate_map_t &gate_map,
                          vector<int> &roots, map<int, bool> &signal_values) {
  vector<int> stack = vector<int>(roots);
  unordered_set<int> entered_gates = unordered_set<int>();
  while (!stack.empty()) {
    int node = stack.back();
    vector<int> &kids = graph[node];
    if (entered_gates.find(node) == entered_gates.end()) {
      for (auto &kid : kids)
        if (signal_values.find(kid) == signal_values.end())
          stack.push_back(kid);
      entered_gates.insert(node);
    } else {
      vector<bool> kids_values = vector<bool>();
      for (auto &kid : kids) kids_values.push_back(signal_values[kid]);
      signal_values[node] = get_gate_value(gate_map[node], kids_values);
      stack.pop_back();
    }
  }
}

static void compute_states(graph_t &graph, gate_map_t &gate_map,
                           vector<int> &roots, vector<int> &input_signals) {
  map<int, bool> signal_values = map<int, bool>();
  size_t n_input_signals = input_signals.size();
  size_t range = (1 << n_input_signals) - 1;
  for (size_t input_values = 0; input_values <= range; input_values++) {
    size_t signal_idx = n_input_signals - 1, input_it = input_values;
    for (size_t i = 0; i < n_input_signals; i++) {
      int signal = input_signals[signal_idx];
      signal_values[signal] = input_it & 1;
      signal_idx--;
      input_it = input_it >> 1;
    }
    compute_state(graph, gate_map, roots, signal_values);
    for (auto &map_it : signal_values) cout << (map_it.second ? 1 : 0);
    cout << '\n';
    signal_values.clear();
  }
}

int main() {
  graph_t graph;
  gate_map_t gate_map;
  vector<int> input_signals;
  tie(graph, gate_map, input_signals) = get_input();
  if (ERROR_REPORTED) exit(1);
  vector<int> roots = get_roots(graph);
  if (is_sequential(graph, roots)) {
    cerr << "Error: sequential logic analysis has not yet been implemented.\n";
    exit(1);
  }
  compute_states(graph, gate_map, roots, input_signals);
}