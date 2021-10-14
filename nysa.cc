#include <cstdio>
#include <iostream>
#include <regex>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

/* Typ reprezentujący numer sygnału. */
using signal_t = int32_t;

/* Typ reprezentujący graf sygnałów.
 * W poprawnym grafie bramkę utożsamiamy z jej sygnałem wyjściowym.
 * Krawędzie są skierowane od bramek do sygnałów wejściowych. */
using graph_t = unordered_map<signal_t, vector<signal_t>>;

/* Klasa wyliczeniowa reprezentująca rodzaj bramki. */
enum class Gate {
    NOT, XOR, AND, NAND, OR, NOR
};

/* Typ reprezentujący mapę łączącą numer bramki z jej rodzajem. */
using gate_map_t = unordered_map<signal_t, Gate>;

/* Mapa łącząca odpowiadający bramce napis z jej rodzajem. */
static unordered_map<string, Gate> string_to_gate = {
        {"NOT",  Gate::NOT},
        {"XOR",  Gate::XOR},
        {"AND",  Gate::AND},
        {"NAND", Gate::NAND},
        {"OR",   Gate::OR},
        {"NOR",  Gate::NOR}
};

/* Zwraca wartość bramki rodzaju 'gate' dla
 * wejść o wartościach z wektora 'inputs'. */
static bool get_gate_value(Gate gate, vector<bool> &inputs) {
    switch (gate) {
        case Gate::NOT:
            return !inputs[0];
        case Gate::XOR:
            return inputs[0] != inputs[1];
        case Gate::AND:
            return all_of(inputs.begin(), inputs.end(),
                          [](bool i) { return i; });
        case Gate::NAND:
            return !all_of(inputs.begin(), inputs.end(),
                           [](bool i) { return i; });
        case Gate::OR:
            return any_of(inputs.begin(), inputs.end(),
                          [](bool i) { return i; });
        case Gate::NOR:
            return !any_of(inputs.begin(), inputs.end(),
                           [](bool i) { return i; });
        default:
            // Ta sytuacja nigdy nie wystąpi, sprawdziliśmy poprawność wejścia.
            return false;
    }
}

/* Zmienna globalna przetrzymująca informacje o
 * tym czy wystąpił błąd wejścia. */
static bool ERROR_REPORTED = false;

/* Informuje o błędzie składni w linii 'line'
 * o numerze 'line_nr'. */
static void report_syntax_error(size_t line_nr, const string &line) {
    cerr << "Error in line " + to_string(line_nr) + ": " + line + "\n";
    ERROR_REPORTED = true;
}

/* Informuje o błędzie podłączenia wyjść wielu bramek
 * do jednego sygnału o numerze 'signal_nr' w linii o numerze 'line_nr'. */
static void report_signal_multiple_outputs_error(size_t line_nr,
                                                 signal_t signal_nr) {
    cerr << "Error in line " + to_string(line_nr) + ": signal " +
            to_string(signal_nr) + " is assigned to multiple outputs.\n";
    ERROR_REPORTED = true;
}

/* Zwraca regex sprawdzający poprawność składniową linii. */
static regex get_line_regex() {
    string number_p = "(\\s+[1-9]\\d{0,8})";
    string not_p = "NOT" + number_p + "{2}";
    string xor_p = "XOR" + number_p + "{3}";
    string others_p = "(AND|NAND|OR|NOR)" + number_p + "{3,}";
    string gate_p = "(" + not_p + "|" + xor_p + "|" + others_p + ")";
    string line_p = "\\s*" + gate_p + "\\s*";
    regex line_regex = regex(line_p);
    return line_regex;
}

/* Funkcja tokenizująca linię. Dla danego stringa 'line' zwraca
 * wektor stringów, które były w linii oddzielone białymi znakami. */
static vector<string> tokenize_line(string line) {
    regex notwhitespace_regex = regex("[^\\s]+");
    vector<string> vect;

    copy(sregex_token_iterator(line.begin(), line.end(), notwhitespace_regex,
                               0),
         sregex_token_iterator(), back_inserter(vect));
    return vect;
}

/* Konwertuje stringi z wyznaczonego przez wskaźniki 'begin' oraz 'end'
 * przedziału wektora na liczby reprezentujące numery sygnałów i zwraca wynik
 * w postaci wektora liczb. */
static vector<signal_t> get_signal_numbers(vector<string>::iterator begin,
                                           vector<string>::iterator end) {
    vector<signal_t> numbers = vector<signal_t>();

    for (auto it = begin; it != end; it++) numbers.push_back(stoi(*it));
    return numbers;
}

/* Przetwarza poprawną linię 'line'.
 * Jeśli sygnał, do którego ma być podłączona bramka jest jeszcze wolny,
 * dodaje nowe połączenia do gragu 'graph',oraz mapy 'gate_map'.
 * Numer bramki dodaje do zbiorów 'all_signals' oraz 'gate_signals', a
 * numery jej sygnałów wejściowych dodaje do zbioru 'all_signals'.
 * W przeciwnym przypadku zgłasza wystąpienie błędu. */
static void add_gate(graph_t &graph, gate_map_t &gate_map,
                     set<signal_t> &all_signals, set<signal_t> &gate_signals,
                     string &line, size_t line_nr) {
    vector<string> tokenized_line = (tokenize_line(line));
    Gate gate = string_to_gate[tokenized_line[0]];
    signal_t gate_number = stoi(tokenized_line[1]);
    vector<signal_t> kids_numbers = get_signal_numbers(
            tokenized_line.begin() + 2, tokenized_line.end());

    if (graph.find(gate_number) == graph.end()) {
        graph[gate_number] = kids_numbers;
        gate_map[gate_number] = gate;

        gate_signals.insert(gate_number);
        all_signals.insert(gate_number);
        copy(kids_numbers.begin(), kids_numbers.end(),
             inserter(all_signals, all_signals.end()));
    } else {
        report_signal_multiple_outputs_error(line_nr, gate_number);
    }
}

/* Wczytuje wejście. Sprawdza poprawność składniową kolejnych linii i zgłasza
 * wystąpienie błędu składniowego w przypadku niepoprawności linii.
 * W zmiennych 'graph', 'gate_map' oraz 'input_signals' zapisuje odpowiednio
 * połączenia między sygnałami, mapę definiującą rodzaj bramek oraz
 * wektor zawierający numery sygnałów wejściowych w kolejności rosnącej. */
static void get_input(graph_t &graph, gate_map_t &gate_map,
                      vector<signal_t> &input_signals) {
    regex line_regex = get_line_regex();
    string line;
    size_t line_nr = 1;
    set<signal_t> all_signals = set<signal_t>();
    set<signal_t> gate_signals = set<signal_t>();
    graph = unordered_map<signal_t, vector<signal_t>>();
    gate_map = unordered_map<signal_t, Gate>();
    input_signals = vector<signal_t>();

    while (getline(cin, line)) {
        if (regex_match(line, line_regex))
            add_gate(graph, gate_map, all_signals, gate_signals, line, line_nr);
        else
            report_syntax_error(line_nr, line);
        line_nr++;
    }
    set_difference(all_signals.begin(), all_signals.end(), gate_signals.begin(),
                   gate_signals.end(), back_inserter(input_signals));
}

/* Zwraca wektor zawierający wszystkie korzenie grafu 'graph'. */
static vector<signal_t> get_roots(graph_t &graph) {
    unordered_map<signal_t, bool> is_root;

    for (auto &graph_it : graph) {
        signal_t gate_nr = graph_it.first;
        if (is_root.find(gate_nr) == is_root.end())
            is_root[gate_nr] = true;
        vector<signal_t> &kids_numbers = graph_it.second;
        for (auto &kid_number : kids_numbers)
            is_root[kid_number] = false;
    }

    vector<signal_t> roots = vector<signal_t>();
    for (auto &map_it : is_root)
        if (map_it.second)
            roots.push_back(map_it.first);
    return roots;
}

/* Sprawdza czy w grafie 'graph' o korzeniach 'roots' występują cykle. */
static bool is_sequential(const graph_t &graph, const vector<signal_t> &roots) {
    if (roots.empty())
        return true;
    enum color {
        VISITING, VISITED
    };
    unordered_map<signal_t, color> colors = unordered_map<signal_t, color>();
    vector<signal_t> stack = vector<signal_t>(roots);

    while (!stack.empty()) {
        signal_t node = stack.back();
        if (colors.find(node) == colors.end()) {
            colors[node] = VISITING; // Wchodzimy do bramki.
            if (graph.find(node) != graph.end()) {
                for (auto kid : graph.at(node))
                    if (colors.find(kid) == colors.end())
                        stack.push_back(kid); // Dodajemy nieodwiedzone bramki.
                    else if (colors.at(kid) == VISITING)
                        return true; // Napotkaliśmy cykl.
            }
        } else {
            colors[node] = VISITED; // Wychodzimy z bramki.
            stack.pop_back();
        }
    }
    return false;
}

/* Dla grafu 'graph', mapy definiującej rodzaj bramek 'gate_map',
 * wektora zawierającego korzenie grafu 'roots' oraz mapy 'signal_values'
 * zawierającej informacje o wartości wszystkich sygnałów wejściowych,
 * oblicza wartości pozostałych sygnałów i zapisuje wynik w
 * mapie 'signal_values' */
static void compute_state(graph_t &graph, gate_map_t &gate_map,
                          vector<signal_t> &roots,
                          map<signal_t, bool> &signal_values) {
    vector<signal_t> stack = vector<signal_t>(roots);
    unordered_set<signal_t> entered_gates = unordered_set<signal_t>();

    while (!stack.empty()) {
        signal_t node = stack.back();
        vector<signal_t> &kids = graph[node];

        if (entered_gates.find(node) == entered_gates.end()) {
            entered_gates.insert(node); // Wchodzimy do bramki.
            for (auto &kid : kids)
                if (signal_values.find(kid) == signal_values.end())
                    stack.push_back(kid); // Dodajemy niepoliczone bramki.
        } else { // Obliczamy wartość bramki i wychodzimy z niej.
            vector<bool> kids_values = vector<bool>();
            for (auto &kid : kids)
                kids_values.push_back(signal_values[kid]);
            signal_values[node] = get_gate_value(gate_map[node], kids_values);
            stack.pop_back();
        }
    }
}

/* Dla grafu 'graph', mapy definiującej rodzaj bramek 'gate_map' oraz
 * wektora korzeni grafu 'roots' iteruje się po wszystkich kombinacjach
 * sygnałów wejściowych, oblicza dla nich wartości pozostałych sygnałów
 * i wypisuje wartości wszystkich sygnalów. */
static void compute_states(graph_t &graph, gate_map_t &gate_map,
                           vector<signal_t> &roots,
                           vector<signal_t> &input_signals) {
    map<signal_t, bool> signal_values = map<signal_t, bool>();
    size_t n_input_signals = input_signals.size();
    // Liczba podanych sygnałów wejściowych jest za duża.
    if (n_input_signals >= 64)
        exit(1);
    size_t range = (1 << n_input_signals) - 1;

    for (size_t input_values = 0; input_values <= range; input_values++) {
        size_t signal_idx = n_input_signals - 1, input_it = input_values;
        for (size_t i = 0; i < n_input_signals; i++) {
            signal_t signal = input_signals[signal_idx];
            signal_values[signal] = input_it & 1;
            signal_idx--;
            input_it = input_it >> 1;
        }
        compute_state(graph, gate_map, roots, signal_values);
        for (auto &map_it : signal_values)
            cout << (map_it.second ? 1 : 0);
        cout << '\n';
        signal_values.clear();
    }
}

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    graph_t graph;
    gate_map_t gate_map;
    vector<int> input_signals;
    get_input(graph, gate_map, input_signals);
    if (ERROR_REPORTED)
        exit(1);
    vector<int> roots = get_roots(graph);
    if (is_sequential(graph, roots)) {
        cerr << "Error: sequential logic analysis"
             << " has not yet been implemented.\n";
        exit(1);
    }
    compute_states(graph, gate_map, roots, input_signals);
}