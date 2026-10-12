#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <map>
#include <cmath>
#include <set>
#include <fstream>
#include <queue>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;


void printError(const string& message) {
    cerr << "Error: " << message << endl;
}

void printSuccess(const string& message) {
    cout << "SUCCESS: " << message << endl;
}

class Element {
public:
    string name;
    string type;
    string node1_name;
    string node2_name;
    double voltage = 0.0;
    double current = 0.0;

    Element(string n, string t, string n1, string n2 = "")
            : name(std::move(n)), type(std::move(t)), node1_name(std::move(n1)), node2_name(std::move(n2)) {}

    virtual ~Element() = default;

    virtual void display() const {
        cout << "Element: " << name << " (" << type << ") connected to "
             << node1_name << (node2_name.empty() ? "" : " and " + node2_name);
    }

    virtual string toNetlistSting() const = 0;
};


class Resistor : public Element {
public: double value;
    Resistor(string n, string n1, string n2, double val) : Element(std::move(n), "Resistor", std::move(n1), std::move(n2)), value(val) {}
    void display() const override {
        Element::display();
        cout << ", Value: " << scientific << setprecision(3) << value << " Ohms" << defaultfloat << endl;
    }
    string toNetlistSting() const override {
        return name + " " + node1_name + " " + node2_name + " " + to_string(value);
    }
};
class Capacitor : public Element {
public:
    double value;
    Capacitor(string n, string n1, string n2, double val) : Element(std::move(n), "Capacitor", std::move(n1), std::move(n2)), value(val) {}
    void display() const override {
        Element::display(); cout << ", Value: " << scientific << setprecision(3) << value << " F" << defaultfloat << endl;
    }
    string toNetlistSting() const override {
        return name + " " + node1_name + " " + node2_name + " " + to_string(value);
    }
};

class Inductor : public Element {
public:
    double value; Inductor(string n, string n1, string n2, double val) : Element(std::move(n), "Inductor", std::move(n1), std::move(n2)), value(val) {}
    void display() const override {
        Element::display(); cout << ", Value: " << scientific << setprecision(3) << value << " H" << defaultfloat << endl;
    }
    string toNetlistSting() const override {
        return name + " " + node1_name + " " + node2_name + " " + to_string(value);
    }
};

class Diode : public Element {
public:
    string model; Diode(string n, string n1, string n2, string m) : Element(std::move(n), "Diode", std::move(n1), std::move(n2)), model(std::move(m)) {}
    void display() const override {
        Element::display(); cout << ", Model: " << model << endl;
    }
    string toNetlistSting() const override {
        return name + " " + node1_name + " " + node2_name + " " + model;
    }
};

class IndependentVoltageSource : public Element {
public:
    string source_form;
    double dc_value;
    double v_offset;
    double v_amplitude;
    double frequency;
    IndependentVoltageSource(string n, string n1, string n2, double dc_val) : Element(std::move(n), "IndependentVoltageSource", std::move(n1), std::move(n2)), source_form("DC"), dc_value(dc_val), v_offset(0), v_amplitude(0), frequency(0) {}
    IndependentVoltageSource(string n, string n1, string n2, double vo, double va, double fq) : Element(std::move(n), "IndependentVoltageSource", std::move(n1), std::move(n2)), source_form("SIN"), dc_value(0), v_offset(vo), v_amplitude(va), frequency(fq) {}
    void display() const override {
        Element::display(); if (source_form == "DC") { cout << ", Type: DC, Value: " << dc_value << "V" << endl; } else if (source_form == "SIN") {
            cout << ", Type: SIN, Offset: " << v_offset << "V, Amplitude: " << v_amplitude << "V, Frequency: " << frequency << "Hz" << endl;
        }
    }
    string toNetlistSting() const override {
        if (source_form == "DC")
            return name + " " + node1_name + " " + node2_name + " " + to_string(dc_value);
        else return name + " " + node1_name + " " + node2_name + " SIN(" + to_string(v_offset) + " " + to_string(v_amplitude) + " " + to_string(frequency) + ")";
    }
};

class IndependentCurrentSource : public Element {
public:
    double dc_value;
    IndependentCurrentSource(string n, string n1, string n2, double dc_val) : Element(std::move(n), "IndependentCurrentSource", std::move(n1), std::move(n2)), dc_value(dc_val) {}
    void display() const override {
        Element::display(); cout << ", Type: DC, Value: " << dc_value << "A" << endl;
    }
    string toNetlistSting() const override {
        return name + " " + node1_name + " " + node2_name + " " + to_string(dc_value);
    }
};

class VCVS_E_Source : public Element {
public:
    string ctrl_node1_name;
    string ctrl_node2_name;
    double gain;
    VCVS_E_Source(string n, string n1, string n2, string cn1, string cn2, double g) : Element(std::move(n), "VCVS_E", std::move(n1), std::move(n2)), ctrl_node1_name(std::move(cn1)), ctrl_node2_name(std::move(cn2)), gain(g) {}
    void display() const override {
        Element::display(); cout << ", Control Nodes: " << ctrl_node1_name << ", " << ctrl_node2_name << ", Gain: " << gain << endl;
    }
    string toNetlistSting() const override {
        return name + " " + node1_name + " " + node2_name + " " + ctrl_node1_name + " " + ctrl_node2_name + " " + to_string(gain);
    }
};

class VCCS_G_Source : public Element {
public:
    string ctrl_node1_name;
    string ctrl_node2_name;
    double gain;
    VCCS_G_Source(string n, string n1, string n2, string cn1, string cn2, double g) : Element(std::move(n), "VCCS_G", std::move(n1), std::move(n2)), ctrl_node1_name(std::move(cn1)), ctrl_node2_name(std::move(cn2)), gain(g) {}
    void display() const override {
        Element::display(); cout << ", Control Nodes: " << ctrl_node1_name << ", " << ctrl_node2_name << ", Gain (transconductance): " << gain << " S" << endl;
    }
    string toNetlistSting() const override {
        return name + " " + node1_name + " " + node2_name + " " + ctrl_node1_name + " " + ctrl_node2_name + " " + to_string(gain);
    }
};

class CCVS_H_Source : public Element {
public:
    string controlling_vsource_name;
    double gain;
    CCVS_H_Source(string n, string n1, string n2, string vname, double g) : Element(std::move(n), "CCVS_H", std::move(n1), std::move(n2)), controlling_vsource_name(std::move(vname)), gain(g) {}
    void display() const override {
        Element::display(); cout << ", Controlling VSource: " << controlling_vsource_name << ", Gain (transresistance): " << gain << " Ohms" << endl;
    }
    string toNetlistSting() const override { return name + " " + node1_name + " " + node2_name + " " + controlling_vsource_name + " " + to_string(gain); }
};

class CCCS_F_Source : public Element {
public:
    string controlling_vsource_name;
    double gain;
    CCCS_F_Source(string n, string n1, string n2, string vname, double g) : Element(std::move(n), "CCCS_F", std::move(n1), std::move(n2)), controlling_vsource_name(std::move(vname)), gain(g) {}
    void display() const override {
        Element::display(); cout << ", Controlling VSource: " << controlling_vsource_name << ", Gain (current gain): " << gain << endl;
    }
    string toNetlistSting() const override {
        return name + " " + node1_name + " " + node2_name + " " + controlling_vsource_name + " " + to_string(gain);
    }
};


class Node {
public:
    string name;
    double voltage = 0.0;
    Node(string n) : name(std::move(n)) {}
};

struct PrintRequest {
    string analysis_type;
    double tstep = 0, tstop = 0, tstart = 0, tmaxstep = 0;
    string dc_source_name, dc_start_val, dc_end_val, dc_increment;
    vector<pair<char, string>> variables;
};



vector<double> solveLinearSystem(vector<vector<double>> A, vector<double> b) {

    int n = A.size();
    if (n == 0) return {};

    for (int i = 0; i < n; i++) {
        int max_row = i;
        for (int k = i + 1; k < n; k++) {
            if (abs(A[k][i]) > abs(A[max_row][i])) {
                max_row = k;
            }
        }
        swap(A[i], A[max_row]);
        swap(b[i], b[max_row]);

        if (abs(A[i][i]) < 1e-12) {
            printError("Matrix is singular or nearly singular. Cannot solve the system.");
            return {};
        }

        for (int k = i + 1; k < n; k++) {
            double factor = A[k][i] / A[i][i];
            b[k] -= factor * b[i];
            for (int j = i; j < n; j++) {
                A[k][j] -= factor * A[i][j];
            }
        }
    }

    vector<double> x(n);
    for (int i = n - 1; i >= 0; i--) {
        double sum = 0;
        for (int j = i + 1; j < n; j++) {
            sum += A[i][j] * x[j];
        }
        x[i] = (b[i] - sum) / A[i][i];
    }
    return x;
}

class Circuit {
public:
    map<string, Node*> nodes;
    map<string, Element*> elements;
    vector<string> element_order;
    set<string> grounded_node_names;
    string filepath;


    void solveDCAnalysis(const PrintRequest& request);
    void solveTranAnalysis(const PrintRequest& request);

    Circuit(string path = "") : filepath(std::move(path)) {}
    ~Circuit() {
        for (auto& pair : nodes) delete pair.second;
        for (auto& pair : elements) delete pair.second;
    }


    void clear() {
        for (auto& pair : nodes) delete pair.second;
        for (auto& pair : elements) delete pair.second;
        nodes.clear();
        elements.clear();
        element_order.clear();
        grounded_node_names.clear();
    }


    bool parseComponentValue(const string& s_val, double& out_val, const string& component_type_char_str) const {
        if (s_val.empty()) return false;
        string num_part = s_val;
        double multiplier = 1.0;

        string lower_s_val = s_val;
        transform(lower_s_val.begin(), lower_s_val.end(), lower_s_val.begin(), ::tolower);

        if (lower_s_val.length() > 2 && lower_s_val.substr(lower_s_val.length() - 2) == "ms") {
            multiplier = 1e-3; num_part = s_val.substr(0, s_val.length() - 2);
        }
        else if (lower_s_val.length() > 2 && lower_s_val.substr(lower_s_val.length() - 2) == "us") {
            multiplier = 1e-6; num_part = s_val.substr(0, s_val.length() - 2);
        }
        else if (lower_s_val.length() > 2 && lower_s_val.substr(lower_s_val.length() - 2) == "ns") {
            multiplier = 1e-9; num_part = s_val.substr(0, s_val.length() - 2);
        }
        else if (lower_s_val.length() > 2 && lower_s_val.substr(lower_s_val.length() - 2) == "ps") {
            multiplier = 1e-12; num_part = s_val.substr(0, s_val.length() - 2);
        }
        else if (lower_s_val.length() > 3 && lower_s_val.substr(lower_s_val.length() - 3) == "meg") {
            multiplier = 1e6; num_part = s_val.substr(0, s_val.length() - 3);
        }
        else if (!num_part.empty() && !isdigit(lower_s_val.back()) && lower_s_val.back() != '.') {
            if (lower_s_val.back() == 'g') {
                multiplier = 1e9; num_part = s_val.substr(0, s_val.length() - 1);
            }
            else if (lower_s_val.back() == 'k') {
                multiplier = 1e3; num_part = s_val.substr(0, s_val.length() - 1);
            }
            else if (lower_s_val.back() == 'm') {
                multiplier = 1e-3; num_part = s_val.substr(0, s_val.length() - 1);
            }
            else if (lower_s_val.back() == 'u') {
                multiplier = 1e-6; num_part = s_val.substr(0, s_val.length() - 1);
            }
            else if (lower_s_val.back() == 'n') {
                multiplier = 1e-9; num_part = s_val.substr(0, s_val.length() - 1);
            }
            else if (lower_s_val.back() == 'p') {
                multiplier = 1e-12; num_part = s_val.substr(0, s_val.length() - 1);
            }
            else {
                num_part = s_val;
            }
        }

        try {
            out_val = stod(num_part) * multiplier;
            return true;
        } catch (const invalid_argument&) {
            return false;
        }
        catch (const out_of_range&) {
            return false;
        }
    }


    Node* getOrCreateNode(const string& node_name) {
        if (nodes.find(node_name) == nodes.end()) {
            nodes[node_name] = new Node(node_name);
        }
        return nodes[node_name];
    }

    bool addResistorInternal(const string& name, const string& n1, const string& n2, const string& val_str) {
        if (name.empty() || toupper(name[0]) != 'R') {
            printError("Resistor name must start with 'R'."); return false;
        }
        if (name[0] != 'R') {
            printError("Element " + name + " not found in library. Resistor name must start with a capital 'R'."); return false;
        }
        if (elements.count(name)) {
            printError("Resistor " + name + " already exists in the circuit."); return false;
        }
        double r_value;
        if (!parseComponentValue(val_str, r_value, "R")) {
            printError("Invalid resistance value '" + val_str + "' for " + name + "."); return false;
        }
        if (r_value <= 0) {
            printError("Negative or zero value for a component is invalid. (Resistor " + name + ")"); return false;
        }
        getOrCreateNode(n1);
        getOrCreateNode(n2);
        elements[name] = new Resistor(name, n1, n2, r_value);
        element_order.push_back(name);
        cout << "Resistor " << name << " added successfully." << endl; elements[name]->display();
        return true;
    }

    bool addCapacitorInternal(const string& name, const string& n1, const string& n2, const string& val_str) {
        if (name.empty() || toupper(name[0]) != 'C') {
            printError("Capacitor name must start with 'C'."); return false;
        }
        if (name[0] != 'C') {
            printError("Element " + name + " not found in library. Capacitor name must start with a capital 'C'."); return false;
        }
        if (elements.count(name)) {
            printError("Capacitor " + name + " already exists in the circuit."); return false;
        }
        double c_value;
        if (!parseComponentValue(val_str, c_value, "C")) { printError("Invalid capacitance value '" + val_str + "' for " + name + "."); return false; }
        if (c_value <= 0) {
            printError("Negative or zero value for a component is invalid. (Capacitor " + name + ")"); return false;
        }
        getOrCreateNode(n1);
        getOrCreateNode(n2);
        elements[name] = new Capacitor(name, n1, n2, c_value); element_order.push_back(name);
        cout << "Capacitor " << name << " added successfully." << endl; elements[name]->display();
        return true;
    }

    bool addInductorInternal(const string& name, const string& n1, const string& n2, const string& val_str) {
        if (name.empty() || toupper(name[0]) != 'L') {
            printError("Inductor name must start with 'L'."); return false;
        }
        if (name[0] != 'L') {
            printError("Element " + name + " not found in library. Inductor name must start with a capital 'L'."); return false;
        }
        if (elements.count(name)) {
            printError("inductor " + name + " already exists in the circuit."); return false;
        }
        double l_value;
        if (!parseComponentValue(val_str, l_value, "L")) { printError("Invalid inductance value '" + val_str + "' for " + name + "."); return false; }
        if (l_value <= 0) {
            printError("Negative or zero value for a component is invalid. (Inductor " + name + ")"); return false;
        }
        getOrCreateNode(n1);
        getOrCreateNode(n2);
        elements[name] = new Inductor(name, n1, n2, l_value);
        element_order.push_back(name);
        cout << "Inductor " << name << " added successfully." << endl; elements[name]->display();
        return true;
    }

    bool addDiodeInternal(const string& name, const string& n1, const string& n2, const string& model_str) {
        if (name.empty() || toupper(name[0]) != 'D') {
            printError("Diode name must start with 'D'.");
            return false;
        }
        if (name[0] != 'D') {
            printError("Element " + name + " not found in library. Diode name must start with a capital 'D'.");
            return false;
        }
        if (elements.count(name)) {
            printError("diode " + name + " already exists in the circuit.");
            return false;
        }
        getOrCreateNode(n1);
        getOrCreateNode(n2);
        elements[name] = new Diode(name, n1, n2, model_str);
        element_order.push_back(name);
        cout << "Diode " << name << " added successfully." << endl; elements[name]->display();
        return true;
    }

    bool addIndependentVoltageSourceInternal(const string& name, const string& n1, const string& n2, const string& val_params_str) {
        if (name.empty() || toupper(name[0]) != 'V') {
            printError("Voltage Source name must start with 'V'.");
            return false;
        }
        if (elements.count(name)) {
            printError("Duplicate node or component name detected. (Element " + name + " already exists)");
            return false;
        }
        getOrCreateNode(n1);
        getOrCreateNode(n2);

        string temp_params = val_params_str;
        string upper_temp_params = temp_params;
        transform(upper_temp_params.begin(), upper_temp_params.end(), upper_temp_params.begin(), ::toupper);

        if (upper_temp_params.rfind("SIN(", 0) == 0 && upper_temp_params.back() == ')') {
            string sin_params_content = val_params_str.substr(4, val_params_str.length() - 5);
            stringstream sin_ss(sin_params_content);
            string voff_str, vamp_str, freq_str;
            double v_offset, v_amplitude, frequency;
            if (!(sin_ss >> voff_str >> vamp_str >> freq_str) ) {
                printError("Missing parameters for time-dependent source. (SIN for " + name + ")");
                return false;
            }
            if (!parseComponentValue(voff_str, v_offset, "V") || !parseComponentValue(vamp_str, v_amplitude, "V") || !parseComponentValue(freq_str, frequency, "V")) {
                printError("Invalid numeric value in SIN parameters for " + name + ".");
                return false;
            }
            elements[name] = new IndependentVoltageSource(name, n1, n2, v_offset, v_amplitude, frequency);
        } else {
            double dc_value;
            if (!parseComponentValue(val_params_str, dc_value, "V")) {
                printError("Invalid DC value '" + val_params_str + "' for Voltage Source " + name + ".");
                return false;
            }
            elements[name] = new IndependentVoltageSource(name, n1, n2, dc_value);
        }
        element_order.push_back(name);
        cout << "IndependentVoltageSource " << name << " added successfully." << endl;
        elements[name]->display();
        return true;
    }
    bool addIndependentCurrentSourceInternal(const string& name, const string& n1, const string& n2, const string& val_str) {
        if (name.empty() || toupper(name[0]) != 'I') {
            printError("Current Source name must start with 'I'.");
            return false;
        }
        if (elements.count(name)) {
            printError("Duplicate node or component name detected. (Element " + name + " already exists)");
            return false;
        }
        double dc_value;
        if (!parseComponentValue(val_str, dc_value, "I")) {
            printError("Invalid DC value '" + val_str + "' for Current Source " + name + ".");
            return false;
        }
        getOrCreateNode(n1);
        getOrCreateNode(n2);
        elements[name] = new IndependentCurrentSource(name, n1, n2, dc_value);
        element_order.push_back(name);
        cout << "IndependentCurrentSource " << name << " added successfully." << endl; elements[name]->display();
        return true;
    }
    bool addVCVS_E_Internal(const string& name, const string& n_plus, const string& n_minus, const string& nc_plus, const string& nc_minus, const string& gain_str) {
        if (name.empty() || toupper(name[0]) != 'E') {
            printError("VCVS (E) name must start with 'E'.");
            return false;
        }
        if (elements.count(name)) {
            printError("Duplicate node or component name detected. (Element " + name + " already exists)");
            return false;
        }
        if (nodes.find(nc_plus) == nodes.end() || nodes.find(nc_minus) == nodes.end()) {
            printError("Dependent source has an undefined control element. (Control node(s) for " + name + " not found)");
            return false;
        }
        double gain;
        if (!parseComponentValue(gain_str, gain, "E")) {
            printError("Invalid gain value for " + name + ".");
            return false;
        }
        getOrCreateNode(n_plus);
        getOrCreateNode(n_minus);
        elements[name] = new VCVS_E_Source(name, n_plus, n_minus, nc_plus, nc_minus, gain);
        element_order.push_back(name);
        cout << "VCVS (E-source) " << name << " added successfully." << endl;
        elements[name]->display();
        return true;
    }
    bool addVCCS_G_Internal(const string& name, const string& n_plus, const string& n_minus, const string& nc_plus, const string& nc_minus, const string& gain_str) {
        if (name.empty() || toupper(name[0]) != 'G') {
            printError("VCCS (G) name must start with 'G'.");
            return false;
        }
        if (elements.count(name)) {
            printError("Duplicate node or component name detected. (Element " + name + " already exists)");
            return false;
        }
        if (nodes.find(nc_plus) == nodes.end() || nodes.find(nc_minus) == nodes.end()) {
            printError("Dependent source has an undefined control element. (Control node(s) for " + name + " not found)");
            return false;
        }
        double gain;
        if (!parseComponentValue(gain_str, gain, "G")) {
            printError("Invalid gain value for " + name + ".");
            return false;
        }
        getOrCreateNode(n_plus); getOrCreateNode(n_minus);
        elements[name] = new VCCS_G_Source(name, n_plus, n_minus, nc_plus, nc_minus, gain);
        element_order.push_back(name);
        cout << "VCCS (G-source) " << name << " added successfully." << endl; elements[name]->display(); return true;
    }
    bool addCCVS_H_Internal(const string& name, const string& n_plus, const string& n_minus, const string& v_ctrl_name, const string& gain_str) {
        if (name.empty() || toupper(name[0]) != 'H') {
            printError("CCVS (H) name must start with 'H'.");
            return false;
        }
        if (elements.count(name)) {
            printError("Duplicate node or component name detected. (Element " + name + " already exists)");
            return false;
        }
        if (elements.find(v_ctrl_name) == elements.end() || elements.at(v_ctrl_name)->type != "IndependentVoltageSource") {
            printError("Dependent source has an undefined control element. (Controlling VSource " + v_ctrl_name + " for " + name + " not found or not a V-type).");
            return false;
        }
        double gain;
        if (!parseComponentValue(gain_str, gain, "H")) {
            printError("Invalid gain value for " + name + ".");
            return false;
        }
        getOrCreateNode(n_plus);
        getOrCreateNode(n_minus);
        elements[name] = new CCVS_H_Source(name, n_plus, n_minus, v_ctrl_name, gain);
        element_order.push_back(name);
        cout << "CCVS (H-source) " << name << " added successfully." << endl; elements[name]->display();
        return true;
    }
    bool addCCCS_F_Internal(const string& name, const string& n_plus, const string& n_minus, const string& v_ctrl_name, const string& gain_str) {
        if (name.empty() || toupper(name[0]) != 'F') {
            printError("CCCS (F) name must start with 'F'.");
            return false;
        }
        if (elements.count(name)) {
            printError("Duplicate node or component name detected. (Element " + name + " already exists)");
            return false;
        }
        if (elements.find(v_ctrl_name) == elements.end() || elements.at(v_ctrl_name)->type != "IndependentVoltageSource") {
            printError("Dependent source has an undefined control element. (Controlling VSource " + v_ctrl_name + " for " + name + " not found or not a V-type).");
            return false;
        }
        double gain;
        if (!parseComponentValue(gain_str, gain, "F")) {
            printError("Invalid gain value for " + name + ".");
            return false;
        }
        getOrCreateNode(n_plus);
        getOrCreateNode(n_minus);
        elements[name] = new CCCS_F_Source(name, n_plus, n_minus, v_ctrl_name, gain);
        element_order.push_back(name);
        cout << "CCCS (F-source) " << name << " added successfully." << endl;
        elements[name]->display();
        return true;
    }


    void addResistorCommand(const string& name, stringstream& ss) {
        string n1, n2, val_str; if (!(ss >> n1 >> n2 >> val_str)) {
            printError("Invalid 'add " + name + "' format. Missing parameters. Correct: add R<name> <n1> <n2> <value>");
            return;
        }
        addResistorInternal(name, n1, n2, val_str);
    }
    void addCapacitorCommand(const string& name, stringstream& ss) {
        string n1, n2, val_str;
        if (!(ss >> n1 >> n2 >> val_str)) {
            printError("Invalid 'add " + name + "' format. Missing parameters. Correct: add C<name> <n1> <n2> <value>");
            return;
        }
        addCapacitorInternal(name, n1, n2, val_str);
    }
    void addInductorCommand(const string& name, stringstream& ss) {
        string n1, n2, val_str;
        if (!(ss >> n1 >> n2 >> val_str)) {
            printError("Invalid 'add " + name + "' format. Missing parameters. Correct: add L<name> <n1> <n2> <value>");
            return;
        }
        addInductorInternal(name, n1, n2, val_str);
    }
    void addDiodeCommand(const string& name, stringstream& ss) {
        string n1, n2, model_str; if (!(ss >> n1 >> n2 >> model_str)) {
            printError("Invalid 'add " + name + "' format. Missing parameters. Correct: add D<name> <n1> <n2> <model>");
            return;
        }
        addDiodeInternal(name, n1, n2, model_str);
    }
    void addIndependentVoltageSourceCommand(const string& name, stringstream& ss) {
        string n1, n2;
        if (!(ss >> n1 >> n2)) {
            printError("Invalid 'add " + name + "' format. Missing nodes. Correct: add V<name> <n1> <n2> <DCval_or_SIN(...)>");
            return;
        }
        string rest_of_line; getline(ss, rest_of_line); size_t first_char = rest_of_line.find_first_not_of(" ");
        if (string::npos == first_char) {
            printError("Invalid 'add " + name + "' format. Missing value/SIN parameters.");
            return;
        }
        string full_val_param_str = rest_of_line.substr(first_char);
        addIndependentVoltageSourceInternal(name, n1, n2, full_val_param_str);
    }

    void addIndependentCurrentSourceCommand(const string& name, stringstream& ss) {
        string n1, n2, val_str; if (!(ss >> n1 >> n2 >> val_str)) {
            printError("Invalid 'add " + name + "' format. Missing parameters. Correct: add I<name> <n1> <n2> <value>");
            return;
        }
        addIndependentCurrentSourceInternal(name, n1, n2, val_str);
    }
    void addVCVS_E_Command(const string& name, stringstream& ss) {
        string n_p, n_m, nc_p, nc_m, g_str; if (!(ss >> n_p >> n_m >> nc_p >> nc_m >> g_str)) {
            printError("Invalid 'add " + name + "' format. Correct: add E<name> <n+> <n-> <nc+> <nc-> <gain>");
            return;
        }
        addVCVS_E_Internal(name, n_p, n_m, nc_p, nc_m, g_str);
    }
    void addVCCS_G_Command(const string& name, stringstream& ss) {
        string n_p, n_m, nc_p, nc_m, g_str; if (!(ss >> n_p >> n_m >> nc_p >> nc_m >> g_str)) {
            printError("Invalid 'add " + name + "' format. Correct: add G<name> <n+> <n-> <nc+> <nc-> <gain>");
            return;
        }
        addVCCS_G_Internal(name, n_p, n_m, nc_p, nc_m, g_str);
    }
    void addCCVS_H_Command(const string& name, stringstream& ss) {
        string n_p, n_m, vctrl_n, g_str; if (!(ss >> n_p >> n_m >> vctrl_n >> g_str)) {
            printError("Invalid 'add " + name + "' format. Correct: add H<name> <n+> <n-> <VctrlName> <gain>");
            return;
        }
        addCCVS_H_Internal(name, n_p, n_m, vctrl_n, g_str);
    }

    void addCCCS_F_Command(const string& name, stringstream& ss) {
        string n_p, n_m, vctrl_n, g_str;
        if (!(ss >> n_p >> n_m >> vctrl_n >> g_str)) {
            printError("Invalid 'add " + name + "' format. Correct: add F<name> <n+> <n-> <VctrlName> <gain>");
            return;
        }
        addCCCS_F_Internal(name, n_p, n_m, vctrl_n, g_str);
    }


    void deleteResistorCommand(const string& name) {
        if (elements.count(name) && elements[name]->type == "Resistor") {
            delete elements[name];
            elements.erase(name);
            element_order.erase(remove(element_order.begin(), element_order.end(), name), element_order.end());
            cout << "Resistor " << name << " deleted successfully." << endl;
        }
        else {
            printError("Cannot delete resistor; component " + name + " not found or is not a resistor.");
        }
    }

    void deleteCapacitorCommand(const string& name) {
        if (elements.count(name) && elements[name]->type == "Capacitor") {
            delete elements[name]; elements.erase(name); element_order.erase(remove(element_order.begin(), element_order.end(), name), element_order.end());
            cout << "Capacitor " << name << " deleted successfully." << endl;
        }
        else { printError("Cannot delete capacitor; component " + name + " not found or is not a capacitor."); }
    }
    void deleteInductorCommand(const string& name) {
        if (elements.count(name) && elements[name]->type == "Inductor") {
            delete elements[name]; elements.erase(name); element_order.erase(remove(element_order.begin(), element_order.end(), name), element_order.end());
            cout << "Inductor " << name << " deleted successfully." << endl;
        }
        else {
            printError("Cannot delete inductor; component " + name + " not found or is not an inductor.");
        }
    }

    void deleteDiodeCommand(const string& name) {
        if (elements.count(name) && elements[name]->type == "Diode") {
            delete elements[name]; elements.erase(name); element_order.erase(remove(element_order.begin(), element_order.end(), name), element_order.end());
            cout << "Diode " << name << " deleted successfully." << endl;
        }
        else {
            printError("Cannot delete diode; component " + name + " not found or is not a diode.");
        }
    }
    void deleteIndependentVoltageSourceCommand(const string& name) {
        if (elements.count(name) && elements[name]->type == "IndependentVoltageSource") {
            delete elements[name]; elements.erase(name); element_order.erase(remove(element_order.begin(), element_order.end(), name), element_order.end());
            cout << "IndependentVoltageSource " << name << " deleted successfully." << endl;
        }
        else {
            printError("Cannot delete; component " + name + " not found or is not an IndependentVoltageSource.");
        }
    }
    void deleteIndependentCurrentSourceCommand(const string& name) {
        if (elements.count(name) && elements[name]->type == "IndependentCurrentSource") {
            delete elements[name]; elements.erase(name); element_order.erase(remove(element_order.begin(), element_order.end(), name), element_order.end());
            cout << "IndependentCurrentSource " << name << " deleted successfully." << endl;
        }
        else {
            printError("Cannot delete; component " + name + " not found or is not an IndependentCurrentSource.");
        }
    }


    void deleteVCVS_E_Command(const string& name) {
        if (elements.count(name) && elements[name]->type == "VCVS_E") {
            delete elements[name]; elements.erase(name); element_order.erase(remove(element_order.begin(), element_order.end(), name), element_order.end());
            cout << "VCVS_E " << name << " deleted successfully." << endl;
        }
        else {
            printError("Cannot delete; component " + name + " not found or is not a VCVS_E source.");
        }
    }
    void deleteVCCS_G_Command(const string& name) {
        if (elements.count(name) && elements[name]->type == "VCCS_G") {
            delete elements[name]; elements.erase(name); element_order.erase(remove(element_order.begin(), element_order.end(), name), element_order.end());
            cout << "VCCS_G " << name << " deleted successfully." << endl;
        }
        else {
            printError("Cannot delete; component " + name + " not found or is not a VCCS_G source.");
        }
    }

    void deleteCCVS_H_Command(const string& name) {
        if (elements.count(name) && elements[name]->type == "CCVS_H") {
            delete elements[name]; elements.erase(name); element_order.erase(remove(element_order.begin(), element_order.end(), name), element_order.end());
            cout << "CCVS_H " << name << " deleted successfully." << endl;
        }
        else {
            printError("Cannot delete; component " + name + " not found or is not a CCVS_H source.");
        }
    }

    void deleteCCCS_F_Command(const string& name) {
        if (elements.count(name) && elements[name]->type == "CCCS_F") {
            delete elements[name]; elements.erase(name); element_order.erase(remove(element_order.begin(), element_order.end(), name), element_order.end());
            cout << "CCCS_F " << name << " deleted successfully." << endl;
        }
        else {
            printError("Cannot delete; component " + name + " not found or is not a CCCS_F source.");
        }
    }
    void addGndCommand(stringstream& ss) {
        string node_name; ss >> node_name;
        if (node_name.empty()) {
            printError("Syntax error. Node name missing for 'add GND'."); return;
        }
        string extra_token;
        if (ss >> extra_token) {
            printError("Syntax error: Too many arguments for 'add GND " + node_name + "'. Expected format: add GND <node>");
            return;
        }
        getOrCreateNode(node_name); grounded_node_names.insert(node_name); cout << "Node " << node_name << " is now designated as ground." << endl;
    }
    void deleteGndCommand(stringstream& ss) {
        string node_name;
        ss >> node_name;
        if (node_name.empty()) {
            printError("Syntax error. Node name missing for 'delete GND'.");
            return;
        } string extra_token;
        if (ss >> extra_token) {
            printError("Syntax error: Too many arguments for 'delete GND " + node_name + "'. Expected format: delete GND <node>");
            return;
        }
        if (nodes.find(node_name) == nodes.end()) {
            printError("Node " + node_name + " does not exist.");
            return;
        }
        if (grounded_node_names.count(node_name)) {
            grounded_node_names.erase(node_name);
            cout << "Node " << node_name << " is no longer designated as ground." << endl;
        }
        else {
            cout << "Node " << node_name << " was not designated as ground." << endl;
        }
    }

    void displayNodesCommand(stringstream& ss) const {
        string extra_token; if (ss >> extra_token) {
            printError("Syntax error: '.nodes' command does not take any arguments.");
            return;
        }
        set<string> current_nodes_in_use = nodes_in_use();
        if (current_nodes_in_use.empty()) {
            cout << "Available nodes: (none)" << endl;
            return;
        }
        cout << "Available nodes:" << endl;
        bool first = true;
        for (const string& node_name : current_nodes_in_use) {
            if (!first) cout << ", ";
            cout << node_name;
            first = false;
        }
        cout << endl;
    }
    void listElementsCommand(stringstream& ss) const {
        string component_type_filter_str; ss >> component_type_filter_str; string extra_token;
        if (ss >> extra_token) {
            printError("Syntax error: Too many arguments for '.list' command.");
            return;
        }
        string target_type_full_name;
        bool filter_active = !component_type_filter_str.empty();
        if (filter_active) {
            char type_char = toupper(component_type_filter_str[0]);
            string lower_filter = component_type_filter_str;
            transform(lower_filter.begin(), lower_filter.end(), lower_filter.begin(), ::tolower);

            if (component_type_filter_str.length() == 1) {
                if (type_char == 'R') target_type_full_name = "Resistor";
                else if (type_char == 'C') target_type_full_name = "Capacitor";
                else if (type_char == 'L') target_type_full_name = "Inductor";
                else if (type_char == 'D') target_type_full_name = "Diode";
                else if (type_char == 'V') target_type_full_name = "IndependentVoltageSource";
                else if (type_char == 'I') target_type_full_name = "IndependentCurrentSource";
                else if (type_char == 'E') target_type_full_name = "VCVS_E";
                else if (type_char == 'G') target_type_full_name = "VCCS_G";
                else if (type_char == 'H') target_type_full_name = "CCVS_H";
                else if (type_char == 'F') target_type_full_name = "CCCS_F";
                else { printError("Invalid component type for .list: " + component_type_filter_str);
                    return;
                }
            }
            else {
                if (lower_filter == "resistor") target_type_full_name = "Resistor";
                else if (lower_filter == "capacitor") target_type_full_name = "Capacitor";
                else if (lower_filter == "inductor") target_type_full_name = "Inductor";
                else if (lower_filter == "diode") target_type_full_name = "Diode";
                else if (lower_filter == "independentvoltagesource" || lower_filter == "voltagesource") target_type_full_name = "IndependentVoltageSource";
                else if (lower_filter == "independentcurrentsource" || lower_filter == "currentsource") target_type_full_name = "IndependentCurrentSource";
                else if (lower_filter == "vcvs_e" || lower_filter == "e_source") target_type_full_name = "VCVS_E";
                else if (lower_filter == "vccs_g" || lower_filter == "g_source") target_type_full_name = "VCCS_G";
                else if (lower_filter == "ccvs_h" || lower_filter == "h_source") target_type_full_name = "CCVS_H";
                else if (lower_filter == "cccs_f" || lower_filter == "f_source") target_type_full_name = "CCCS_F";
                else {
                    printError("Invalid component type for .list: " + component_type_filter_str);
                    return;
                }
            }
        }
        cout << "Listing elements:" << endl;
        if (element_order.empty()) {
            cout << "(No elements in circuit)" << endl;
            return; }
        bool found_any = false;
        for (const string& elem_name : element_order) {
            if (elements.count(elem_name)) {
                Element* elem = elements.at(elem_name);
                if (!filter_active || elem->type == target_type_full_name) {
                    elem->display(); found_any = true;
                }
            }
        }
        if (filter_active && !found_any) {
            cout << "(No elements of type '" << component_type_filter_str << "' found)" << endl;
        }
    }
    void renameNodeCommand(stringstream& ss) {
        string node_keyword, old_name, new_name; ss >> node_keyword >> old_name >> new_name; string extra_token;
        if (ss >> extra_token || node_keyword != "node" || old_name.empty() || new_name.empty()) {
            printError("Invalid syntax - correct format:\n.rename node <old_name> <new_name>");
            return;
        }
        if (nodes.find(old_name) == nodes.end()) {
            printError("Node " + old_name + " does not exist in the circuit.");
            return;
        }
        if (old_name == new_name) {
            printSuccess("Node " + old_name + " renamed to " + new_name + " (no change).");
            return;
        }
        if (nodes.find(new_name) != nodes.end()) {
            printError("Duplicate node or component name detected. (Node name " + new_name + " already exists)");
            return;
        }
        Node* node_ptr = nodes[old_name]; nodes.erase(old_name); node_ptr->name = new_name; nodes[new_name] = node_ptr;
        for (const auto& pair : elements) {
            Element* elem = pair.second;
            if (elem->node1_name == old_name) elem->node1_name = new_name;
            if (!elem->node2_name.empty() && elem->node2_name == old_name) elem->node2_name = new_name;
            if (elem->type == "VCVS_E") {
                auto* vcvs = static_cast<VCVS_E_Source*>(elem);
                if (vcvs->ctrl_node1_name == old_name) vcvs->ctrl_node1_name = new_name;
                if (vcvs->ctrl_node2_name == old_name) vcvs->ctrl_node2_name = new_name;
            }
            else if (elem->type == "VCCS_G") {
                auto* vccs = static_cast<VCCS_G_Source*>(elem);
                if (vccs->ctrl_node1_name == old_name) vccs->ctrl_node1_name = new_name;
                if (vccs->ctrl_node2_name == old_name) vccs->ctrl_node2_name = new_name;
            }
        }
        if (grounded_node_names.count(old_name)) {
            grounded_node_names.erase(old_name);
            grounded_node_names.insert(new_name);
        }
        printSuccess("Node renamed from " + old_name + " to " + new_name);
    }
    bool parseOutputVariable(const string& var_str, char& type, string& name_token) const {
        if (var_str.length() < 4) return false; type = toupper(var_str[0]);
        if ((type != 'V' && type != 'I') || var_str[1] != '(' || var_str.back() != ')')
            return false;
        name_token = var_str.substr(2, var_str.length() - 3);
        return !name_token.empty();
    }


    set<string> nodes_in_use() const {
        set<string> in_use;
        for(const auto& pair : elements){
            if(!pair.second->node1_name.empty()) in_use.insert(pair.second->node1_name);
            if(!pair.second->node2_name.empty()) in_use.insert(pair.second->node2_name);
            if (pair.second->type == "VCVS_E") {
                auto* src = static_cast<VCVS_E_Source*>(pair.second);
                if(nodes.count(src->ctrl_node1_name)) in_use.insert(src->ctrl_node1_name);
                if(nodes.count(src->ctrl_node2_name)) in_use.insert(src->ctrl_node2_name);
            }
            else if (pair.second->type == "VCCS_G") {
                auto* src = static_cast<VCCS_G_Source*>(pair.second);
                if(nodes.count(src->ctrl_node1_name)) in_use.insert(src->ctrl_node1_name);
                if(nodes.count(src->ctrl_node2_name)) in_use.insert(src->ctrl_node2_name);
            }
        }
        for(const auto& gnd_node : grounded_node_names){
            if(nodes.count(gnd_node)) in_use.insert(gnd_node);
        }
        for(const auto& pair : nodes) {
            in_use.insert(pair.first);
        }
        return in_use;
    }

    bool isCircuitConnected() const {
        set<string> current_nodes_in_use = nodes_in_use();
        if (current_nodes_in_use.size() <= 1) return true;

        map<string, vector<string>> adj;
        for(const auto& pair : elements) {
            Element* elem = pair.second;
            if (!elem->node1_name.empty() && !elem->node2_name.empty() && current_nodes_in_use.count(elem->node1_name) && current_nodes_in_use.count(elem->node2_name) ) {
                adj[elem->node1_name].push_back(elem->node2_name);
                adj[elem->node2_name].push_back(elem->node1_name);
            }
        }
        if (adj.empty() && current_nodes_in_use.size() > 1) return false;
        if(adj.empty() && current_nodes_in_use.size() <=1) return true;

        queue<string> q;
        set<string> visited;
        q.push(adj.begin()->first);
        visited.insert(adj.begin()->first);

        while(!q.empty()) {
            string u = q.front(); q.pop();
            for(const string& v : adj[u]) {
                if (visited.find(v) == visited.end()) {
                    visited.insert(v);
                    q.push(v);
                }
            }
        }
        return visited.size() == current_nodes_in_use.size();
    }

    bool checkCircuitPrerequisites(string& error_message_out) const {
        set<string> current_nodes_in_use = nodes_in_use();
        if (!current_nodes_in_use.empty() && grounded_node_names.empty()) {
            error_message_out = "No ground node detected in the circuit.";
            return false;
        }
        if (!current_nodes_in_use.empty() && !isCircuitConnected()) {
            error_message_out = "Disconnected or multi-part circuit detected.";
            return false;
        }
        return true;
    }

    void handleDcPrint(stringstream& ss, PrintRequest& request) {
        string error_msg; if (!checkCircuitPrerequisites(error_msg)) {
            printError(error_msg);
            return;
        }
        if (!(ss >> request.dc_source_name >> request.dc_start_val >> request.dc_end_val >> request.dc_increment)) {
            printError("Syntax error in command: Missing parameters for .print DC.");
            cout << "Expected: .print DC <SourceName> <StartVal> <EndVal> <Increment> <Var1> ..." << endl;
            return;
        }
        if (elements.find(request.dc_source_name) == elements.end() ||
            (elements.at(request.dc_source_name)->type != "IndependentVoltageSource" && elements.at(request.dc_source_name)->type != "IndependentCurrentSource") ) {
            printError("Component " + request.dc_source_name + " (specified as DC source) not found or is not an independent source.");
        }
        string var_token; while(ss >> var_token) {
            char var_type; string var_name;
            if (parseOutputVariable(var_token, var_type, var_name)) {
                if (var_type == 'V' && nodes.find(var_name) == nodes.end() && !grounded_node_names.count(var_name)) {
                    printError("Node " + var_name + " not found in circuit (for V(" + var_name + ")).");
                    return;
                }
                if (var_type == 'I' && elements.find(var_name) == elements.end()) {
                    printError("Component " + var_name + " not found in circuit (for I(" + var_name + ")).");
                    return;
                }
                request.variables.push_back({var_type, var_name});
            }
            else { printError("Syntax error in command: Malformed variable '" + var_token + "'.");
                return;
            }
        }
        if (request.variables.empty()) {
            printError("Syntax error in command: No variables specified for .print DC.");
            return;
        }
        solveDCAnalysis(request);
    }

    void handleTranPrint(stringstream& ss, PrintRequest& request) {
        string error_msg; if (!checkCircuitPrerequisites(error_msg)) {
            printError(error_msg);
            return;
        }
        string tstep_str, tstop_str, tstart_str, tmax_str;

        if (!(ss >> tstep_str >> tstop_str)) {
            printError("Syntax error: Missing Tstep or Tstop for .print TRAN.");
            return;
        }
        if (!parseComponentValue(tstep_str, request.tstep, "T") || !parseComponentValue(tstop_str, request.tstop, "T")) {
            printError("Invalid Tstep or Tstop value for .print TRAN.");
            return;
        }

        string next_token;
        if(ss >> next_token) {
            if (next_token.find('(') == string::npos) {
                if (!parseComponentValue(next_token, request.tstart, "T")) {
                    printError("Invalid Tstart value for .print TRAN.");
                    return;
                }
                if (ss >> next_token && next_token.find('(') == string::npos) {
                    if (!parseComponentValue(next_token, request.tmaxstep, "T")) {
                        printError("Invalid Tmaxstep value for .print TRAN.");
                        return;
                    }
                } else {
                    ss.seekg(-(int)next_token.length(), ios_base::cur);
                }
            }
            else {
                ss.seekg(-(int)next_token.length(), ios_base::cur);
            }
        }

        string var_token;
        while(ss >> var_token) {
            char var_type; string var_name;
            if (parseOutputVariable(var_token, var_type, var_name)) {
                if (var_type == 'V' && nodes.find(var_name) == nodes.end() && !grounded_node_names.count(var_name)) {
                    printError("Node " + var_name + " not found.");
                    return;
                }
                if (var_type == 'I' && elements.find(var_name) == elements.end()) {
                    printError("Component " + var_name + " not found.");
                    return;
                }
                request.variables.push_back({var_type, var_name});
            }
            else { printError("Malformed variable '" + var_token + "'.");
                return;
            }
        }
        if (request.variables.empty()) {
            printError("No variables specified for .print TRAN.");
            return;
        }

        solveTranAnalysis(request);
    }


    void parsePrintCommand(stringstream& ss) {
        string analysis_type_str; if (!(ss >> analysis_type_str)) {
            printError("Syntax error in command: Missing analysis type for .print.");
            return;
        }
        transform(analysis_type_str.begin(), analysis_type_str.end(), analysis_type_str.begin(), ::toupper);
        PrintRequest request; request.analysis_type = analysis_type_str;
        if (analysis_type_str == "TRAN") handleTranPrint(ss, request);
        else if (analysis_type_str == "DC") handleDcPrint(ss, request);
        else printError("Syntax error in command: Unknown analysis type '" + request.analysis_type + "' for .print.");
    }

    void displayCircuit() const {
        if (elements.empty() && nodes_in_use().empty() && grounded_node_names.empty()) {
            cout << "Circuit is empty." << endl;
            return;
        }

        cout << "\n--- Current Circuit State ---" << endl;
        set<string> displayed_nodes = nodes_in_use();
        if (!displayed_nodes.empty()){
            cout << "Nodes:" << endl;
            for (const string& node_name : displayed_nodes) {
                cout << "- " << node_name;
                if (grounded_node_names.count(node_name)) {
                    cout << " (GND)";
                }
                cout << endl;
            }
        } else cout << "No nodes in the circuit." << endl;

        if(!elements.empty()){
            cout << "Elements (in order of addition):" << endl;
            for (const string& name : element_order) {
                if (elements.count(name)) elements.at(name)->display();
            }
        } else {
            cout << "No elements in the circuit." << endl;
        }
        cout << "---------------------------\n" << endl;
    }

    void processCommand(const string& command_line) {
        stringstream ss(command_line);
        string command_verb_orig; ss >> command_verb_orig;
        if (command_verb_orig.empty()) return;

        string command_verb_lower = command_verb_orig;
        transform(command_verb_lower.begin(), command_verb_lower.end(), command_verb_lower.begin(), ::tolower);

        if (command_verb_lower == "add") {
            string full_name_or_type;
            if (!(ss >> full_name_or_type)) {
                printError("Syntax error: 'add' command requires element name or 'GND'."); return;
            }

            if (full_name_or_type == "GND") { addGndCommand(ss); return; }
            if (full_name_or_type == "gnd") { printError("Element gnd not found in library."); return; }

            char type_char = toupper(full_name_or_type[0]);
            if (type_char == 'R') addResistorCommand(full_name_or_type, ss);
            else if (type_char == 'C') addCapacitorCommand(full_name_or_type, ss);
            else if (type_char == 'L') addInductorCommand(full_name_or_type, ss);
            else if (type_char == 'D') addDiodeCommand(full_name_or_type, ss);
            else if (type_char == 'V') addIndependentVoltageSourceCommand(full_name_or_type, ss);
            else if (type_char == 'I') addIndependentCurrentSourceCommand(full_name_or_type, ss);
            else if (type_char == 'E') addVCVS_E_Command(full_name_or_type, ss);
            else if (type_char == 'G') addVCCS_G_Command(full_name_or_type, ss);
            else if (type_char == 'H') addCCVS_H_Command(full_name_or_type, ss);
            else if (type_char == 'F') addCCCS_F_Command(full_name_or_type, ss);
            else printError("Unknown element type for interactive 'add' based on name: " + full_name_or_type);

        } else if (command_verb_lower == "delete") {
            string element_name_to_delete; ss >> element_name_to_delete;
            if (element_name_to_delete.empty()) {
                printError("Syntax error: 'delete' command requires element name or 'GND'.");
                return;
            }
            if (element_name_to_delete == "GND") {
                string node_to_unground;
                if (ss >> node_to_unground) {
                    stringstream temp_ss; temp_ss << node_to_unground;
                    deleteGndCommand(temp_ss);
                } else {
                    printError("Syntax error: 'delete GND' requires a node name.");
                }
                return;
            }

            if (elements.find(element_name_to_delete) == elements.end()) {
                printError("Cannot delete component " + element_name_to_delete + "; not found.");
                return;
            }
            string type_of_element_to_delete = elements.at(element_name_to_delete)->type;

            if (type_of_element_to_delete == "Resistor") deleteResistorCommand(element_name_to_delete);
            else if (type_of_element_to_delete == "Capacitor") deleteCapacitorCommand(element_name_to_delete);
            else if (type_of_element_to_delete == "Inductor") deleteInductorCommand(element_name_to_delete);
            else if (type_of_element_to_delete == "Diode") deleteDiodeCommand(element_name_to_delete);
            else if (type_of_element_to_delete == "IndependentVoltageSource") deleteIndependentVoltageSourceCommand(element_name_to_delete);
            else if (type_of_element_to_delete == "IndependentCurrentSource") deleteIndependentCurrentSourceCommand(element_name_to_delete);
            else if (type_of_element_to_delete == "VCVS_E") deleteVCVS_E_Command(element_name_to_delete);
            else if (type_of_element_to_delete == "VCCS_G") deleteVCCS_G_Command(element_name_to_delete);
            else if (type_of_element_to_delete == "CCVS_H") deleteCCVS_H_Command(element_name_to_delete);
            else if (type_of_element_to_delete == "CCCS_F") deleteCCCS_F_Command(element_name_to_delete);
            else printError("Unknown element type '" + type_of_element_to_delete + "' for deletion of " + element_name_to_delete);

        }
        else if (command_verb_orig == ".nodes") { displayNodesCommand(ss);
        }
        else if (command_verb_orig == ".list") { listElementsCommand(ss);
        }
        else if (command_verb_orig == ".rename") { renameNodeCommand(ss);
        }
        else if (command_verb_orig == ".print") { parsePrintCommand(ss);
        }
        else if (command_verb_lower == "display"){ displayCircuit();
        }
        else if (command_verb_lower == "exit" || command_verb_lower == "return" || command_verb_lower == "save" ){ /* Handled in main loops */ }
        else {
            string line_with_add = "add " + command_line;
            processCommand(line_with_add);
        }
    }
};

void Circuit::solveDCAnalysis(const PrintRequest& request) {
    map<string, int> node_to_index;
    int index_counter = 0;
    set<string> current_nodes_in_use = this->nodes_in_use();
    for (const string& node_name : current_nodes_in_use) {
        if (this->grounded_node_names.find(node_name) == this->grounded_node_names.end()) {
            node_to_index[node_name] = index_counter++;
        }
    }

    map<string, int> vsource_to_index;
    int vsource_counter = 0;
    for (const auto& pair : this->elements) {
        if (pair.second->type == "IndependentVoltageSource") {
            vsource_to_index[pair.first] = vsource_counter++;
        }
    }

    int num_nodes = node_to_index.size();
    int num_vsources = vsource_to_index.size();
    int matrix_size = num_nodes + num_vsources;

    if (matrix_size == 0) {
        cout << "DC Analysis: Circuit is empty." << endl;
        return;
    }

    vector<vector<double>> A(matrix_size, vector<double>(matrix_size, 0.0));
    vector<double> b(matrix_size, 0.0);

    for (const auto& pair : this->elements) {
        Element* elem = pair.second;
        const string& n1 = elem->node1_name;
        const string& n2 = elem->node2_name;

        bool n1_is_gnd = this->grounded_node_names.count(n1);
        bool n2_is_gnd = this->grounded_node_names.count(n2);

        int idx1 = n1_is_gnd ? -1 : (node_to_index.count(n1) ? node_to_index.at(n1) : -1);
        int idx2 = n2_is_gnd ? -1 : (node_to_index.count(n2) ? node_to_index.at(n2) : -1);

        if (elem->type == "Resistor") {
            auto* r = static_cast<Resistor*>(elem);
            double g = 1.0 / r->value;
            if (idx1 != -1) A[idx1][idx1] += g;
            if (idx2 != -1) A[idx2][idx2] += g;
            if (idx1 != -1 && idx2 != -1) { A[idx1][idx2] -= g; A[idx2][idx1] -= g; }
        } else if (elem->type == "IndependentCurrentSource") {
            auto* isrc = static_cast<IndependentCurrentSource*>(elem);
            if (idx1 != -1) b[idx1] -= isrc->dc_value;
            if (idx2 != -1) b[idx2] += isrc->dc_value;
        } else if (elem->type == "IndependentVoltageSource") {
            auto* vsrc = static_cast<IndependentVoltageSource*>(elem);
            int v_idx = num_nodes + vsource_to_index.at(vsrc->name);
            if (idx1 != -1) { A[idx1][v_idx] += 1.0; A[v_idx][idx1] += 1.0; }
            if (idx2 != -1) { A[idx2][v_idx] -= 1.0; A[v_idx][idx2] -= 1.0; }
            b[v_idx] += vsrc->dc_value;
        }
    }

    vector<double> x = solveLinearSystem(A, b);
    if (x.empty()) {
        printError("DC analysis failed."); return;
    }

    map<string, double> node_voltages;
    for (const auto& pair : node_to_index) {
        node_voltages[pair.first] = x[pair.second];
    }
    for (const string& gnd_node : this->grounded_node_names) {
        node_voltages[gnd_node] = 0.0;
    }

    map<string, double> element_currents;
    for(const auto& pair : vsource_to_index) {
        element_currents[pair.first] = x[num_nodes + pair.second];
    }
    for (const auto& pair : this->elements) {
        if(pair.second->type == "Resistor"){
            double v1 = node_voltages.at(pair.second->node1_name);
            double v2 = node_voltages.at(pair.second->node2_name);
            element_currents[pair.first] = (v1 - v2) / static_cast<Resistor*>(pair.second)->value;
        }
    }

    cout << "--- DC Analysis Results ---" << endl;
    cout << fixed << setprecision(6);
    for (const auto& var : request.variables) {
        if (var.first == 'V') {
            if (node_voltages.count(var.second)) {
                cout << "  V(" << var.second << ") = " << node_voltages.at(var.second) << " V" << endl;
            }
        } else if (var.first == 'I') {
            if (element_currents.count(var.second)) {
                cout << "  I(" << var.second << ") = " << element_currents.at(var.second) << " A" << endl;
            }
        }
    }
    cout << defaultfloat;
}


void Circuit::solveTranAnalysis(const PrintRequest& request) {
    map<string, int> node_to_index;
    int index_counter = 0;
    set<string> current_nodes_in_use = this->nodes_in_use();
    for (const string& node_name : current_nodes_in_use) {
        if (this->grounded_node_names.find(node_name) == this->grounded_node_names.end()) {
            node_to_index[node_name] = index_counter++;
        }
    }

    map<string, int> extra_vars_to_index;
    int extra_var_counter = 0;
    for (const auto& pair : this->elements) {
        if (pair.second->type == "IndependentVoltageSource" || pair.second->type == "Inductor") {
            extra_vars_to_index[pair.first] = extra_var_counter++;
        }
    }

    int num_nodes = node_to_index.size();
    int num_extra_vars = extra_vars_to_index.size();
    int matrix_size = num_nodes + num_extra_vars;

    if (matrix_size == 0) { cout << "TRAN Analysis: Circuit is empty." << endl; return; }

    double h = request.tstep;
    for (auto const& [name, node_ptr] : this->nodes) { node_ptr->voltage = 0.0; }
    for (auto const& [name, elem_ptr] : this->elements) { elem_ptr->voltage = 0.0; elem_ptr->current = 0.0; }

    cout << "--- Transient Analysis Results ---" << endl;
    cout << setw(12) << "Time(s)";
    for(const auto& var : request.variables) { cout << setw(15) << (var.first + string("(") + var.second + string(")")); }
    cout << endl;

    if (request.tstart == 0.0) {
        cout << setw(12) << scientific << setprecision(4) << 0.0;
        for (const auto& var : request.variables) {
            cout << setw(15) << scientific << setprecision(4) << 0.0;
        }
        cout << endl;
    }


    for (double t = h; t <= request.tstop + h/2; t += h) {
        vector<vector<double>> A(matrix_size, vector<double>(matrix_size, 0.0));
        vector<double> b(matrix_size, 0.0);

        map<string, double> old_voltages;
        for (auto const& [name, node_ptr] : this->nodes) {
            if (!grounded_node_names.count(name) && node_to_index.count(name)) old_voltages[name] = node_ptr->voltage;
        }

        for (const auto& pair : this->elements) {
            Element* elem = pair.second;
            const string& n1 = elem->node1_name;
            const string& n2 = elem->node2_name;
            bool n1_is_gnd = this->grounded_node_names.count(n1);
            bool n2_is_gnd = this->grounded_node_names.count(n2);
            int idx1 = n1_is_gnd ? -1 : (node_to_index.count(n1) ? node_to_index.at(n1) : -1);
            int idx2 = n2_is_gnd ? -1 : (node_to_index.count(n2) ? node_to_index.at(n2) : -1);

            if (elem->type == "Resistor") {
                auto* r = static_cast<Resistor*>(elem);
                double g = 1.0 / r->value;
                if (idx1 != -1) A[idx1][idx1] += g;
                if (idx2 != -1) A[idx2][idx2] += g;
                if (idx1 != -1 && idx2 != -1) { A[idx1][idx2] -= g; A[idx2][idx1] -= g; }
            } else if (elem->type == "Capacitor") {
                auto* c = static_cast<Capacitor*>(elem);
                double g_eq = c->value / h;
                double i_eq = g_eq * c->voltage;
                if (idx1 != -1) { A[idx1][idx1] += g_eq; b[idx1] += i_eq; }
                if (idx2 != -1) { A[idx2][idx2] += g_eq; b[idx2] -= i_eq; }
                if (idx1 != -1 && idx2 != -1) { A[idx1][idx2] -= g_eq; A[idx2][idx1] -= g_eq; }
            } else if (elem->type == "Inductor") {
                auto* l = static_cast<Inductor*>(elem);
                int l_idx = num_nodes + extra_vars_to_index.at(l->name);
                double r_eq = l->value / h;
                double v_eq = r_eq * l->current;
                A[l_idx][l_idx] = -r_eq;
                if (idx1 != -1) { A[idx1][l_idx] += 1.0; A[l_idx][idx1] += 1.0; }
                if (idx2 != -1) { A[idx2][l_idx] -= 1.0; A[l_idx][idx2] -= 1.0; }
                b[l_idx] -= v_eq;
            } else if (elem->type == "IndependentVoltageSource") {
                auto* vsrc = static_cast<IndependentVoltageSource*>(elem);
                int v_idx = num_nodes + extra_vars_to_index.at(vsrc->name);
                double val = vsrc->dc_value;
                if(vsrc->source_form == "SIN") {
                    val = vsrc->v_offset + vsrc->v_amplitude * sin(2 * M_PI * vsrc->frequency * t);
                }
                if (idx1 != -1) { A[idx1][v_idx] += 1.0; A[v_idx][idx1] += 1.0; }
                if (idx2 != -1) { A[idx2][v_idx] -= 1.0; A[v_idx][idx2] -= 1.0; }
                b[v_idx] += val;
            }
        }

        vector<double> x = solveLinearSystem(A, b);
        if (x.empty()) { printError("Transient analysis failed at t = " + to_string(t)); return; }

        for (auto const& [name, index] : node_to_index) {
            this->nodes[name]->voltage = x[index];
        }

        for (auto const& [name, elem_ptr] : this->elements) {
            double v1_new = this->grounded_node_names.count(elem_ptr->node1_name) ? 0.0 : this->nodes.at(elem_ptr->node1_name)->voltage;
            double v2_new = this->grounded_node_names.count(elem_ptr->node2_name) ? 0.0 : this->nodes.at(elem_ptr->node2_name)->voltage;
            double old_voltage = elem_ptr->voltage;
            elem_ptr->voltage = v1_new - v2_new;

            if (elem_ptr->type == "Inductor" || elem_ptr->type == "IndependentVoltageSource") {
                elem_ptr->current = x[num_nodes + extra_vars_to_index.at(name)];
            } else if (elem_ptr->type == "Capacitor"){
                static_cast<Capacitor*>(elem_ptr)->current = static_cast<Capacitor*>(elem_ptr)->value * (elem_ptr->voltage - old_voltage)/h;
            }
        }

        if (t >= request.tstart) {
            cout << setw(12) << scientific << setprecision(4) << t;
            for(const auto& var : request.variables) {
                double val_to_print = NAN;
                if(var.first == 'V'){
                    val_to_print = this->grounded_node_names.count(var.second) ? 0.0 : this->nodes.at(var.second)->voltage;
                }
                else if (var.first == 'I') {
                    Element* el = this->elements.at(var.second);
                    if(el->type == "Resistor"){
                        double v1 = this->grounded_node_names.count(el->node1_name) ? 0.0 : this->nodes.at(el->node1_name)->voltage;
                        double v2 = this->grounded_node_names.count(el->node2_name) ? 0.0 : this->nodes.at(el->node2_name)->voltage;
                        val_to_print = (v1-v2) / static_cast<Resistor*>(el)->value;
                    }
                    else {
                        val_to_print = el->current;
                    }
                }
                cout << setw(15) << scientific << setprecision(4) << val_to_print;
            }
            cout << endl;
        }
    }
    cout << defaultfloat;
}


void showMainMenu() {
    cout << "\n===== Main Menu =====\n";
    cout << "1. Add/Load a schematic file (add file <filepath>)\n";
    cout << "2. Create a new schematic file (new file <name.txt>)\n";
    cout << "3. Show existing schematics (-show existing schematics)\n";
    cout << "4. Help (helpme)\n";
    cout << "5. Exit (exit)\n";
    cout << "=====================\n";
}

void showHelp() {
    cout << "\n===== Help =====\n";
    cout << "This is a circuit simulator with a file management menu.\n";
    cout << " - 'add file <filepath>': Loads a circuit from the specified text file and adds it to the list.\n";
    cout << " - 'new file <filepath>': Creates a new empty text file and immediately enters the editing session.\n";
    cout << " - '-show existing schematics': Shows a list of loaded files. You can select one to work on.\n";
    cout << " - 'helpme': Shows this help message.\n";
    cout << " - 'exit': Exits the simulator.\n";
    cout << "Inside a schematic session, you can use commands like 'add R1 ...', '.print', etc.\n";
    cout << "Use 'return' to save the current schematic and go back to the main menu.\n";
}


class Simulator {
private:
    map<string, Circuit> circuits;
    map<string, string> schematic_paths;
    vector<string> schematic_order;

public:

    void loadCircuitFromFile(const string& filepath) {
        ifstream infile(filepath);
        if (!infile.is_open()) {
            printError("Cannot open file: " + filepath);
            return;
        }

        string schematic_name = getSchematicName(filepath);
        if (circuits.find(schematic_name) == circuits.end()) {
            schematic_order.push_back(schematic_name);
            schematic_paths[schematic_name] = filepath;
        }

        circuits[schematic_name].clear();
        circuits[schematic_name].filepath = filepath;


        cout << "Loading circuit from file: " << filepath << endl;
        string line;
        while (getline(infile, line)) {
            line.erase(0, line.find_first_not_of(" \t\n\r\f\v"));
            line.erase(line.find_last_not_of(" \t\n\r\f\v") + 1);
            if (line.empty() || line[0] == '*' || line[0] == ';') continue;

            circuits[schematic_name].processCommand(line);
        }
        cout << "Finished loading from file: " << filepath << ". Use '-show existing schematics' to work on it." << endl;
        infile.close();
    }

    void createNewCircuit(const string& filepath) {
        ofstream outfile(filepath);
        if (!outfile.is_open()) {
            printError("Could not create file: " + filepath);
            return;
        }
        outfile.close();
        cout << "New file created: " << filepath << endl;

        string schematic_name = getSchematicName(filepath);
        if (circuits.find(schematic_name) == circuits.end()) {
            schematic_order.push_back(schematic_name);
            schematic_paths[schematic_name] = filepath;
        }
        circuits[schematic_name].clear();
        circuits[schematic_name].filepath = filepath;

        enterCircuitSession(schematic_name);
    }

    void showAndSelectSchematics() {
        if (schematic_order.empty()) {
            cout << "No schematics loaded. Use 'add file' or 'new file' first." << endl;
            return;
        }

        string choice_line;
        while (true) {
            cout << "\n-choose existing schematic:" << endl;
            for (size_t i = 0; i < schematic_order.size(); ++i) {
                cout << (i + 1) << "- " << schematic_order[i] << " (" << schematic_paths[schematic_order[i]] << ")" << endl;
            }
            cout << "Enter number or 'return' to go back: " << flush;

            if (!getline(cin, choice_line) || choice_line == "return") {
                cout << endl;
                break;
            }
            if (choice_line.empty()) continue;

            stringstream choice_ss(choice_line);
            int choice_num;
            if (choice_ss >> choice_num && choice_num >= 1 && choice_num <= schematic_order.size()) {
                string selected_schematic = schematic_order[choice_num - 1];
                cout << "Selected schematic: " << selected_schematic << endl;
                cout << selected_schematic << ":" << endl;
                ifstream infile(schematic_paths[selected_schematic]);
                string line;
                while(getline(infile, line)){
                    cout << line << endl;
                }
                infile.close();
                enterCircuitSession(selected_schematic);
                break;
            }
            else {
                cout << "-Error : Inappropriate input" << endl;
            }
        }
    }

private:
    string getSchematicName(const string& filepath) {
        size_t last_slash = filepath.find_last_of("/\\");
        size_t last_dot = filepath.find_last_of(".");
        string name = (last_slash == string::npos) ? filepath : filepath.substr(last_slash + 1);
        if (last_dot != string::npos && last_dot > last_slash) {
            name = name.substr(0, name.find_last_of("."));
        }
        return name;
    }

    void saveCircuit(const string& schematic_name) {
        if (circuits.find(schematic_name) == circuits.end()) {
            printError("Cannot save, schematic not found: " + schematic_name);
            return;
        }

        Circuit& circuit_to_save = circuits.at(schematic_name);
        ofstream outfile(circuit_to_save.filepath);
        if(!outfile.is_open()) {
            printError("Could not open file for saving: " + circuit_to_save.filepath);
            return;
        }

        for(const auto& elem_name : circuit_to_save.element_order) {
            if(circuit_to_save.elements.count(elem_name)){
                outfile << circuit_to_save.elements.at(elem_name)->toNetlistSting() << endl;
            }
        }

        for(const auto& gnd_node : circuit_to_save.grounded_node_names){
            outfile << "add GND " << gnd_node << endl;
        }

        outfile.close();
        printSuccess("Circuit '" + schematic_name + "' saved to " + circuit_to_save.filepath);
    }


    void enterCircuitSession(const string& schematic_name) {
        if (circuits.find(schematic_name) == circuits.end()) {
            printError("Cannot start session, schematic not found: " + schematic_name);
            return;
        }

        cout << "\n--- Entering session for '" << schematic_name << "' ---" << endl;
        cout << "Enter commands or 'return' to save and go back to the main menu." << endl;

        Circuit& active_circuit = circuits.at(schematic_name);
        string line;
        while (true) {
            cout << ">>> " << flush;

            if (!getline(cin, line)) {
                cout << endl;
                break; // EOF
            }
            if (line == "return" || line == "exit") {
                saveCircuit(schematic_name);
                break;
            }
            if (line == "save") {
                saveCircuit(schematic_name);
                continue;
            }
            if (!line.empty()) {
                active_circuit.processCommand(line);
            }
        }
        cout << "--- Exiting session for '" << schematic_name << "' ---" << endl;
    }
};


int main() {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    Simulator sim;
    string line;

    cout << "Welcome to the Circuit Simulator!" << endl;

    while (true) {
        showMainMenu();
        cout << "Main> " << flush;

        if (!getline(cin, line)) {
            cout << endl;
            cout << "Exiting due to EOF." << endl;
            break;
        }
        if (line.empty()) continue;

        stringstream ss(line);
        string command;
        ss >> command;

        string command_lower = command;
        transform(command_lower.begin(), command_lower.end(), command_lower.begin(), ::tolower);


        if (command_lower == "exit" || command == "5") {
            cout << "Exiting simulator." << endl;
            break;
        }
        else if (command_lower == "add" || command == "1") {
            string file_keyword, filepath;
            if(ss >> file_keyword && file_keyword == "file" && ss >> filepath) {
                sim.loadCircuitFromFile(filepath);
            }
            else {
                printError("Invalid command. Use: add file <filepath>");
            }
        }
        else if (command_lower == "new" || command == "2") {
            string file_keyword, filepath;
            if(ss >> file_keyword && file_keyword == "file" && ss >> filepath) {
                sim.createNewCircuit(filepath);
            }
            else {
                printError("Invalid command. Use: new file <filepath>");
            }
        }
        else if (line == "-show existing schematics" || command == "3") {
            sim.showAndSelectSchematics();
        }
        else if (command_lower == "helpme" || command == "4") {
            showHelp();
        }
        else {
            printError("Unknown command in Main Menu. Type 'helpme' for options.");
        }
    }
    return 0;
}
