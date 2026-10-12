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
#include <complex>

#include <numeric>


#include <SDL.h>
#include <SDL_ttf.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;

struct OutputVariableRequest {
    string original_request;
    string type;
    string name;
};

struct PlottableExpression {
    string original_expression;
    OutputVariableRequest var1;
    OutputVariableRequest var2;
    char op = ' ';
};


void printError(const string& message) {
    cerr << "Error:  " << message << endl;
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
public:
    double value;
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

class ACVoltageSource : public Element {
public:
    double ac_magnitude;
    ACVoltageSource(string n, string n1, string n2, double mag)
        : Element(std::move(n), "ACVoltageSource", std::move(n1), std::move(n2)), ac_magnitude(mag) {}

    void display() const override {
        Element::display();
        cout << ", Type: AC, Magnitude: " << ac_magnitude << "V" << defaultfloat << endl;
    }

    string toNetlistSting() const override {

        return name + " " + node1_name + " " + node2_name + " " + to_string(ac_magnitude);
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

    int x = 0;
    int y = 0;
    Node(string n) : name(std::move(n)) {}
};

struct SimulationResult {
    bool success = false;
    string error_message;
    vector<string> headers;
    vector<vector<double>> data;
};



struct PrintRequest {
    string analysis_type;
    double tstep = 0, tstop = 0, tstart = 0, tmaxstep = 0;
    string dc_source_name;
    double dc_start_val = 0, dc_end_val = 0, dc_increment = 0;


    string ac_source_name;
    int num_points = 0;
    double start_freq = 0, end_freq = 0;

    vector<PlottableExpression> expressions;

};


string formatFrequency(double freq) {
    stringstream ss;
    if (abs(freq) < 1e-9) {
        ss << "0";
    } else if (freq >= 1e9) {
        ss << fixed << setprecision(2) << (freq / 1e9) << "G";
    } else if (freq >= 1e6) {
        ss << fixed << setprecision(2) << (freq / 1e6) << "M";
    } else if (freq >= 1e3) {
        ss << fixed << setprecision(2) << (freq / 1e3) << "k";
    } else {
        ss << fixed << setprecision(2) << freq;
    }
    return ss.str();
}



vector<complex<double>> solveComplexLinearSystem(vector<vector<complex<double>>> A, vector<complex<double>> b) {
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
            printError("Complex matrix is singular or nearly singular.");
            return {};
        }

        for (int k = i + 1; k < n; k++) {
            complex<double> factor = A[k][i] / A[i][i];
            b[k] -= factor * b[i];
            for (int j = i; j < n; j++) {
                A[k][j] -= factor * A[i][j];
            }
        }
    }

    vector<complex<double>> x(n);
    for (int i = n - 1; i >= 0; i--) {
        complex<double> sum = {0,0};
        for (int j = i + 1; j < n; j++) {
            sum += A[i][j] * x[j];
        }
        x[i] = (b[i] - sum) / A[i][i];
    }
    return x;
}


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


class Simulator;
class CircuitRenderer;


class Circuit {
public:
    map<string, Node*> nodes;
    map<string, Element*> elements;
    vector<string> element_order;
    set<string> grounded_node_names;
    string filepath;


    map<string, pair<int, int>> node_positions;


    int node_counter = 0;
    map<char, int> component_counters;

    Circuit(string path = "") : filepath(std::move(path)) {
        component_counters['R'] = 0;
        component_counters['C'] = 0;
        component_counters['L'] = 0;
        component_counters['D'] = 0;
        component_counters['V'] = 0;
        component_counters['I'] = 0;
        component_counters['E'] = 0;
        component_counters['G'] = 0;
        component_counters['H'] = 0;
        component_counters['F'] = 0;
        component_counters['W'] = 0;
    }
    ~Circuit();
    void clear();


    string generateUniqueNodeName();
    string generateUniqueComponentName(char prefix);
    void resetCounters();




    bool mergeNodes(const string& nodeToKeep, const string& nodeToRemove);

    void deleteElementAndCleanNodes(const string& elementName);


    bool parseComponentValue(const string& s_val, double& out_val, const string& component_type_char_str) const;
    Node* getOrCreateNode(const string& node_name);


    bool addResistorInternal(const string& name, const string& n1, const string& n2, const string& val_str);
    bool addCapacitorInternal(const string& name, const string& n1, const string& n2, const string& val_str);
    bool addInductorInternal(const string& name, const string& n1, const string& n2, const string& val_str);
    bool addDiodeInternal(const string& name, const string& n1, const string& n2, const string& model_str);
    bool addIndependentVoltageSourceInternal(const string& name, const string& n1, const string& n2, const string& val_params_str);
    bool addIndependentCurrentSourceInternal(const string& name, const string& n1, const string& n2, const string& val_str);
    bool addVCVS_E_Internal(const string& name, const string& n_plus, const string& n_minus, const string& nc_plus, const string& nc_minus, const string& gain_str);
    bool addVCCS_G_Internal(const string& name, const string& n_plus, const string& n_minus, const string& nc_plus, const string& nc_minus, const string& gain_str);
    bool addCCVS_H_Internal(const string& name, const string& n_plus, const string& n_minus, const string& v_ctrl_name, double gain);
    bool addCCCS_F_Internal(const string& name, const string& n_plus, const string& n_minus, const string& v_ctrl_name, double gain);
    bool addACVoltageSourceInternal(const string& name, const string& n1, const string& n2, const string& val_str);
    bool addACVoltageSourceCommand(const string& name, stringstream& ss);


    bool addResistorCommand(const string& name, stringstream& ss);
    bool addCapacitorCommand(const string& name, stringstream& ss);
    bool addInductorCommand(const string& name, stringstream& ss);
    bool addDiodeCommand(const string& name, stringstream& ss);
    bool addIndependentVoltageSourceCommand(const string& name, stringstream& ss);
    bool addIndependentCurrentSourceCommand(const string& name, stringstream& ss);
    bool addVCVS_E_Command(const string& name, stringstream& ss);
    bool addVCCS_G_Command(const string& name, stringstream& ss);
    bool addCCVS_H_Command(const string& name, stringstream& ss);
    bool addCCCS_F_Command(const string& name, stringstream& ss);
    bool parsePlottableExpression(const string& expr_str, PlottableExpression& out_expr) const;


    void deleteResistorCommand(const string& name);
    void deleteCapacitorCommand(const string& name);
    void deleteInductorCommand(const string& name);
    void deleteDiodeCommand(const string& name);
    void deleteIndependentVoltageSourceCommand(const string& name);
    void deleteIndependentCurrentSourceCommand(const string& name);
    void deleteVCVS_E_Command(const string& name);
    void deleteVCCS_G_Command(const string& name);
    void deleteCCVS_H_Command(const string& name);
    void deleteCCCS_F_Command(const string& name);

    void addGndCommand(stringstream& ss);
    void deleteGndCommand(stringstream& ss);

    void displayNodesCommand(stringstream& ss) const;
    void listElementsCommand(stringstream& ss) const;
    void renameNodeCommand(stringstream& ss);

    bool parseOutputVariable(const string& var_str, OutputVariableRequest& out_req) const;

    set<string> nodes_in_use() const;
    bool isCircuitConnected() const;
    bool checkCircuitPrerequisites(string& error_message_out) const;

    SimulationResult  handleDcAnalysis(const PrintRequest& request);
    SimulationResult  handleTranAnalysis(const PrintRequest& request);
    SimulationResult  handleAcAnalysis(const PrintRequest& request);

    void displayCircuit() const;
    bool processCommand(const string& command_line);
};



class Simulator {
private:
    map<string, Circuit> circuits;
    map<string, string> schematic_paths;
    vector<string> schematic_order;
    CircuitRenderer* renderer_ptr;





public:
    string getSchematicName(const string& filepath);
    void saveCircuit(const string& schematic_name);

    Simulator(CircuitRenderer* renderer);


    const vector<string>& getSchematicOrder() const;
    const map<string, string>& getSchematicPaths() const;

    void loadCircuitFromFile(const string& filepath);
    void createNewCircuit(const string& filepath);
    void showAndSelectSchematics();
};



class CircuitRenderer {
public:
    enum RendererState {
        MAIN_MENU,
        FILE_SELECT,
        CIRCUIT_VIEW_IDLE,
        CIRCUIT_VIEW_PLACING_COMPONENT,
        CIRCUIT_VIEW_DRAWING_WIRE,
        INPUT_FILENAME_ADD,
        INPUT_FILENAME_NEW,
        INPUT_COMPONENT_PARAMS,
        SIMULATION_SETUP,
        PLOT_VIEW
    };



    enum ComponentTypeToAdd {
        NONE,
        RESISTOR,
        CAPACITOR,
        INDUCTOR,
        VSOURCE_DC,
        VSOURCE_SIN,
        VSOURCE_AC,
        ISOURCE,
        DIODE,
        VCVS,
        VCCS,
        CCVS,
        CCCS,
        GROUND
    };


    enum InputFieldFocus {
        NO_FOCUS,
        NAME_FIELD,
        NODE1_FIELD,
        NODE2_FIELD,
        VALUE_FIELD,
        VOFF_FIELD,
        VAMP_FIELD,
        FREQ_FIELD,
        MODEL_FIELD,
        CTRL_NODE1_FIELD,
        CTRL_NODE2_FIELD,
        CTRL_VSOURCE_FIELD,
        GAIN_FIELD,
        DC_SOURCE_FIELD,
        DC_START_FIELD,
        DC_END_FIELD,
        DC_INCREMENT_FIELD,
        TRAN_TSTEP_FIELD,
        TRAN_TSTOP_FIELD,
        TRAN_TSTART_FIELD,
        TRAN_TMAXSTEP_FIELD,
        VAR_INPUT_FIELD,
        AC_SOURCE_FIELD,
       AC_POINTS_FIELD,
       AC_START_FREQ_FIELD,
       AC_END_FREQ_FIELD
    };


private:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    TTF_Font* font = nullptr;
    Circuit* active_circuit = nullptr;
    RendererState current_state = MAIN_MENU;

    double plot_view_xmin, plot_view_xmax;
    double plot_view_ymin, plot_view_ymax;

    int window_width = 800;
    int window_height = 600;

    SimulationResult plot_data;

    bool plot_cursor_active = false;
    size_t plot_cursor_data_index = 0;

    string current_input_text;
    bool text_input_active = false;
    string text_input_prompt;

    string command_input_text;
    bool command_input_active = false;
    vector<string> console_output_buffer;
    const size_t MAX_CONSOLE_LINES = 10;

    ComponentTypeToAdd component_type_to_place = NONE;
    SDL_Point component_placement_coords;
    string current_param_input_name;
    string current_param_input_node1;
    string current_param_input_node2;
    string current_param_input_value;
    string current_param_input_voff;
    string current_param_input_vamp;
    string current_param_input_freq;
    string current_param_input_model;
    string current_param_input_ctrl_node1;
    string current_param_input_ctrl_node2;
    string current_param_input_ctrl_vsource;
    string current_param_input_gain;
    InputFieldFocus current_input_focus = NO_FOCUS;

    string first_wire_node_name;
    SDL_Point mouse_pos;

    string selected_element_name;
    string selected_node_name;

    string sim_analysis_type = "DC";
    string sim_dc_source_name;
    string sim_dc_start_val;
    string sim_dc_end_val;
    string sim_dc_increment;
    string sim_tran_tstep;
    string sim_tran_tstop;
    string sim_tran_tstart;
    string sim_tran_tmaxstep;
    string sim_var_input_text;
    vector<PlottableExpression> sim_plottable_expressions;
    string sim_ac_source_name;
    string sim_ac_points;
    string sim_ac_start_freq;
    string sim_ac_end_freq;

    Simulator* simulator_ptr = nullptr;

    void drawText(const string& text, int x, int y, SDL_Color color);
    void drawButton(const string& text, int x, int y, int w, int h, SDL_Color bgColor, SDL_Color textColor);
    bool checkButtonClick(const SDL_Event& e, int x, int y, int w, int h);
    void drawInputField(const string& label, int x, int y, int w, int h, string& value_ref, InputFieldFocus field_focus_id);
    void handleTextInput(SDL_Event& e);

    void runAndPlotSimulation();
    void drawPlotView();
    void resetPlotView();

    void autoLayoutNodes();
    pair<int, int> getNodePos(const string& node_name);

    void drawLine(int x1, int y1, int x2, int y2);
    void drawRect(int x, int y, int w, int h);
    void fillRect(int x, int y, int w, int h);
    void drawCircle(int centerX, int centerY, int radius);

    void drawResistor(int x1, int y1, int x2, int y2);
    void drawCapacitor(int x1, int y1, int x2, int y2);
    void drawInductor(int x1, int y1, int x2, int y2);
    void drawVoltageSource(int x1, int y1, int x2, int y2, bool is_sin);
    void drawCurrentSource(int x1, int y1, int x2, int y2);
    void drawGround(int x, int y);

    void drawMainMenu();
    void drawFileSelectionMenu(const vector<string>& schematic_order, const map<string, string>& schematic_paths);
    void drawCircuitView();
    void drawTextInputBox();
    void drawComponentParamsInputModal();
    void drawSimulationSetup();

    bool executeCommand(const string& command);

public:
    CircuitRenderer();
    ~CircuitRenderer();

    void setSimulator(Simulator* sim);
    void setActiveCircuit(Circuit* circuit);
    void render();
    void handleEvent(SDL_Event& e);
    void setState(RendererState newState);
};



Circuit::~Circuit() {
    for (auto& pair : nodes) delete pair.second;
    for (auto& pair : elements) delete pair.second;
}

void Circuit::clear() {
    for (auto& pair : nodes) delete pair.second;
    for (auto& pair : elements) delete pair.second;
    nodes.clear();
    elements.clear();
    element_order.clear();
    grounded_node_names.clear();
    node_positions.clear();
    resetCounters();
}

void Circuit::resetCounters() {
    node_counter = 0;
    for (auto& pair : component_counters) {
        pair.second = 0;
    }
}

string Circuit::generateUniqueNodeName() {
    return "N" + to_string(node_counter++);
}

string Circuit::generateUniqueComponentName(char prefix) {
    if (component_counters.find(prefix) == component_counters.end()) {
        component_counters[prefix] = 0;
    }
    return string(1, prefix) + to_string(++component_counters[prefix]);
}




















bool Circuit::mergeNodes(const string& nodeToKeep, const string& nodeToRemove) {
    if (nodeToKeep == nodeToRemove) {
        return false;
    }
    if (nodes.find(nodeToKeep) == nodes.end() || nodes.find(nodeToRemove) == nodes.end()) {
        printError("One or both nodes for merging do not exist.");
        return false;
    }



    if(node_positions.count(nodeToRemove)) {


        if (nodes[nodeToKeep]->x == 0 && nodes[nodeToKeep]->y == 0) {
            node_positions[nodeToKeep] = node_positions[nodeToRemove];
            nodes[nodeToKeep]->x = node_positions[nodeToRemove].first;
            nodes[nodeToKeep]->y = node_positions[nodeToRemove].second;
        }
    }


    for (auto& pair : elements) {
        Element* elem = pair.second;
        if (elem->node1_name == nodeToRemove) { elem->node1_name = nodeToKeep; }
        if (!elem->node2_name.empty() && elem->node2_name == nodeToRemove) { elem->node2_name = nodeToKeep; }
    }

    if (grounded_node_names.count(nodeToRemove)) {
        grounded_node_names.insert(nodeToKeep);
        grounded_node_names.erase(nodeToRemove);
    }

    delete nodes[nodeToRemove];
    nodes.erase(nodeToRemove);
    node_positions.erase(nodeToRemove);

    return true;
}
void Circuit::deleteElementAndCleanNodes(const string& elementName) {
    if (elements.find(elementName) == elements.end()) {
        printError("Cannot delete element " + elementName + "; not found.");
        return;
    }

    Element* elem_to_delete = elements.at(elementName);
    string n1_name = elem_to_delete->node1_name;
    string n2_name = elem_to_delete->node2_name;

    delete elements[elementName];
    elements.erase(elementName);
    element_order.erase(remove(element_order.begin(), element_order.end(), elementName), element_order.end());
    printSuccess("Element " + elementName + " deleted successfully.");

    set<string> nodes_still_in_use;
    for (const auto& pair : elements) {
        nodes_still_in_use.insert(pair.second->node1_name);
        if (!pair.second->node2_name.empty()) {
            nodes_still_in_use.insert(pair.second->node2_name);
        }
        if (pair.second->type == "VCVS_E") {
            auto* src = static_cast<VCVS_E_Source*>(pair.second);
            nodes_still_in_use.insert(src->ctrl_node1_name);
            nodes_still_in_use.insert(src->ctrl_node2_name);
        } else if (pair.second->type == "VCCS_G") {
            auto* src = static_cast<VCCS_G_Source*>(pair.second);
            nodes_still_in_use.insert(src->ctrl_node1_name);
            nodes_still_in_use.insert(src->ctrl_node2_name);
        }
    }

    if (nodes.count(n1_name) && nodes_still_in_use.find(n1_name) == nodes_still_in_use.end() && grounded_node_names.find(n1_name) == grounded_node_names.end()) {
        delete nodes[n1_name];
        nodes.erase(n1_name);
        node_positions.erase(n1_name);
        printSuccess("Node " + n1_name + " removed (isolated).");
    }
    if (nodes.count(n2_name) && nodes_still_in_use.find(n2_name) == nodes_still_in_use.end() && grounded_node_names.find(n2_name) == grounded_node_names.end()) {
        delete nodes[n2_name];
        nodes.erase(n2_name);
        node_positions.erase(n2_name);
        printSuccess("Node " + n2_name + " removed (isolated).");
    }
}


bool Circuit::parseComponentValue(const string& s_val, double& out_val, const string& component_type_char_str) const {
    if (s_val.empty()) {
        printError("Empty value provided for component.");
        return false;
    }
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
    else if (!num_part.empty() && !isdigit(lower_s_val.back()) && lower_s_val.back() != '.' && lower_s_val.back() != '-') {
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
        size_t pos;
        out_val = stod(num_part, &pos) * multiplier;
        if (pos != num_part.length()) {
            printError("Invalid characters in numeric value: '" + s_val + "'");
            return false;
        }
        return true;
    } catch (const invalid_argument&) {
        printError("Invalid numeric format for value: '" + s_val + "'");
        return false;
    }
    catch (const out_of_range&) {
        printError("Value out of range: '" + s_val + "'");
        return false;
    }
}

Node* Circuit::getOrCreateNode(const string& node_name) {
    if (nodes.find(node_name) == nodes.end()) {
        nodes[node_name] = new Node(node_name);
        nodes[node_name]->x = 0;
        nodes[node_name]->y = 0;
        node_positions[node_name] = {0,0};
    }
    if (node_positions.count(node_name)) {
        nodes[node_name]->x = node_positions[node_name].first;
        nodes[node_name]->y = node_positions[node_name].second;
    }
    return nodes[node_name];
}


bool Circuit::addResistorInternal(const string& name, const string& n1, const string& n2, const string& val_str) {


    if (name.empty() || (toupper(name[0]) != 'R' && toupper(name[0]) != 'W')) {
        printError("Resistor name must start with 'R'.");
        return false;
    }


    if (elements.count(name)) {

        if (toupper(name[0]) != 'W') {
            printError("Resistor " + name + " already exists in the circuit.");
        }
        return false;
    }
    double r_value;
    if (!parseComponentValue(val_str, r_value, "R")) {
        return false;
    }

    if (r_value <= 0 && toupper(name[0]) != 'W') {
        printError("Negative or zero value for a component is invalid. (Resistor " + name + ")");
        return false;
    }
    getOrCreateNode(n1);
    getOrCreateNode(n2);
    elements[name] = new Resistor(name, n1, n2, r_value);
    element_order.push_back(name);


    if (toupper(name[0]) != 'W') {
        cout << "Resistor " << name << " added successfully." << endl;
        elements[name]->display();
    }
    return true;
}

bool Circuit::addCapacitorInternal(const string& name, const string& n1, const string& n2, const string& val_str) {
    if (name.empty() || toupper(name[0]) != 'C') {
        printError("Capacitor name must start with 'C'."); return false;
    }
    if (elements.count(name)) {
        printError("Capacitor " + name + " already exists in the circuit."); return false;
    }
    double c_value;
    if (!parseComponentValue(val_str, c_value, "C")) { return false; }
    if (c_value <= 0) {
        printError("Negative or zero value for a component is invalid. (Capacitor " + name + ")"); return false;
    }
    getOrCreateNode(n1);
    getOrCreateNode(n2);
    elements[name] = new Capacitor(name, n1, n2, c_value); element_order.push_back(name);
    cout << "Capacitor " << name << " added successfully." << endl; elements[name]->display();
    return true;
}


bool Circuit::addInductorInternal(const string& name, const string& n1, const string& n2, const string& val_str) {
    if (name.empty() || toupper(name[0]) != 'L') {
        printError("Inductor name must start with 'L'."); return false;
    }
    if (elements.count(name)) {
        printError("inductor " + name + " already exists in the circuit."); return false;
    }
    double l_value;
    if (!parseComponentValue(val_str, l_value, "L")) { return false; }
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


bool Circuit::addDiodeInternal(const string& name, const string& n1, const string& n2, const string& model_str) {
    if (name.empty() || toupper(name[0]) != 'D') {
        printError("Diode name must start with 'D'.");
        return false;
    }
    if (elements.count(name)) {
        printError("diode " + name + " already exists in the circuit.");
        return false;
    }
    if (model_str.empty()) {
        printError("Diode " + name + " requires a model name."); return false;
    }
    getOrCreateNode(n1);
    getOrCreateNode(n2);
    elements[name] = new Diode(name, n1, n2, model_str);
    element_order.push_back(name);
    cout << "Diode " << name << " added successfully." << endl; elements[name]->display();
    return true;
}




bool Circuit::addIndependentVoltageSourceInternal(const string& name, const string& n1, const string& n2, const string& val_params_str) {
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
        if (!parseComponentValue(voff_str, v_offset, "V") ||
            !parseComponentValue(vamp_str, v_amplitude, "V") ||
            !parseComponentValue(freq_str, frequency, "V")) {
            return false;
        }
        elements[name] = new IndependentVoltageSource(name, n1, n2, v_offset, v_amplitude, frequency);
    } else {
        double dc_value;
        if (!parseComponentValue(val_params_str, dc_value, "V")) {
            return false;
        }
        elements[name] = new IndependentVoltageSource(name, n1, n2, dc_value);
    }
    element_order.push_back(name);
    cout << "IndependentVoltageSource " << name << " added successfully." << endl;
    elements[name]->display();
    return true;
}
bool Circuit::addACVoltageSourceInternal(const string& name, const string& n1, const string& n2, const string& val_str) {
    if (name.empty() || toupper(name[0]) != 'U') {
        printError("AC Voltage Source name must start with 'U'.");
        return false;
    }
    if (elements.count(name)) {
        printError("Element " + name + " already exists in the circuit.");
        return false;
    }
    double ac_mag;
    if (!parseComponentValue(val_str, ac_mag, "U")) {
        return false;
    }
    getOrCreateNode(n1);
    getOrCreateNode(n2);
    elements[name] = new ACVoltageSource(name, n1, n2, ac_mag);
    element_order.push_back(name);
    cout << "ACVoltageSource " << name << " added successfully." << endl;
    elements[name]->display();
    return true;
}

bool Circuit::addACVoltageSourceCommand(const string& name, stringstream& ss) {
    string n1, n2, val_str;
    if (!(ss >> n1 >> n2 >> val_str)) {
        printError("Invalid 'add " + name + "' format. Missing parameters. Correct: add U<name> <n1> <n2> <magnitude>");
        return false;
    }
    return addACVoltageSourceInternal(name, n1, n2, val_str);
}


bool Circuit::addIndependentCurrentSourceInternal(const string& name, const string& n1, const string& n2, const string& val_str) {
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
        return false;
    }
    getOrCreateNode(n1);
    getOrCreateNode(n2);
    elements[name] = new IndependentCurrentSource(name, n1, n2, dc_value);
    element_order.push_back(name);
    cout << "IndependentCurrentSource " << name << " added successfully." << endl; elements[name]->display();
    return true;
}


bool Circuit::addVCVS_E_Internal(const string& name, const string& n_plus, const string& n_minus, const string& nc_plus, const string& nc_minus, const string& gain_str) {
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

bool Circuit::addVCCS_G_Internal(const string& name, const string& n_plus, const string& n_minus, const string& nc_plus, const string& nc_minus, const string& gain_str) {
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
        return false;
    }
    getOrCreateNode(n_plus); getOrCreateNode(n_minus);
    elements[name] = new VCCS_G_Source(name, n_plus, n_minus, nc_plus, nc_minus, gain);
    element_order.push_back(name);
    cout << "VCCS (G-source) " << name << " added successfully." << endl; elements[name]->display(); return true;
}


bool Circuit::addCCVS_H_Internal(const string& name, const string& n_plus, const string& n_minus, const string& v_ctrl_name, double gain) {
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

    getOrCreateNode(n_plus);
    getOrCreateNode(n_minus);
    elements[name] = new CCVS_H_Source(name, n_plus, n_minus, v_ctrl_name, gain);
    element_order.push_back(name);
    cout << "CCVS (H-source) " << name << " added successfully." << endl; elements[name]->display();
    return true;
}


bool Circuit::addCCCS_F_Internal(const string& name, const string& n_plus, const string& n_minus, const string& v_ctrl_name, double gain) {
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

    getOrCreateNode(n_plus);
    getOrCreateNode(n_minus);
    elements[name] = new CCCS_F_Source(name, n_plus, n_minus, v_ctrl_name, gain);
    element_order.push_back(name);
    cout << "CCCS (F-source) " << name << " added successfully." << endl;
    elements[name]->display();
    return true;
}

bool Circuit::addResistorCommand(const string& name, stringstream& ss) {
    string n1, n2, val_str; if (!(ss >> n1 >> n2 >> val_str)) {
        printError("Invalid 'add " + name + "' format. Missing parameters. Correct: add R<name> <n1> <n2> <value>");
        return false;
    }
    return addResistorInternal(name, n1, n2, val_str);
}

bool Circuit::addCapacitorCommand(const string& name, stringstream& ss) {
    string n1, n2, val_str;
    if (!(ss >> n1 >> n2 >> val_str)) {
        printError("Invalid 'add " + name + "' format. Missing parameters. Correct: add C<name> <n1> <n2> <value>");
        return false;
    }
    return addCapacitorInternal(name, n1, n2, val_str);
}


bool Circuit::addInductorCommand(const string& name, stringstream& ss) {
    string n1, n2, val_str;
    if (!(ss >> n1 >> n2 >> val_str)) {
        printError("Invalid 'add " + name + "' format. Missing parameters. Correct: add L<name> <n1> <n2> <value>");
        return false;
    }
    return addInductorInternal(name, n1, n2, val_str);
}


bool Circuit::addDiodeCommand(const string& name, stringstream& ss) {
    string n1, n2, model_str; if (!(ss >> n1 >> n2 >> model_str)) {
        printError("Invalid 'add " + name + "' format. Missing parameters. Correct: add D<name> <n1> <n2> <model>");
        return false;
    }
    return addDiodeInternal(name, n1, n2, model_str);
}


bool Circuit::addIndependentVoltageSourceCommand(const string& name, stringstream& ss) {
    string n1, n2;
    if (!(ss >> n1 >> n2)) {
        printError("Invalid 'add " + name + "' format. Missing nodes. Correct: add V<name> <n1> <n2> <DCval_or_SIN(...)>");
        return false;
    }
    string rest_of_line; getline(ss, rest_of_line); size_t first_char = rest_of_line.find_first_not_of(" ");
    if (string::npos == first_char) {
        printError("Invalid 'add " + name + "' format. Missing value/SIN parameters.");
        return false;
    }
    string full_val_param_str = rest_of_line.substr(first_char);
    return addIndependentVoltageSourceInternal(name, n1, n2, full_val_param_str);
}


bool Circuit::addIndependentCurrentSourceCommand(const string& name, stringstream& ss) {
    string n1, n2, val_str; if (!(ss >> n1 >> n2 >> val_str)) {
        printError("Invalid 'add " + name + "' format. Missing parameters. Correct: add I<name> <n1> <n2> <value>");
        return false;
    }
    return addIndependentCurrentSourceInternal(name, n1, n2, val_str);
}


bool Circuit::addVCVS_E_Command(const string& name, stringstream& ss) {
    string n_p, n_m, nc_p, nc_m, g_str; if (!(ss >> n_p >> n_m >> nc_p >> nc_m >> g_str)) {
        printError("Invalid 'add " + name + "' format. Correct: add E<name> <n+> <n-> <nc+> <nc-> <gain>");
        return false;
    }
    return addVCVS_E_Internal(name, n_p, n_m, nc_p, nc_m, g_str);
}


bool Circuit::addVCCS_G_Command(const string& name, stringstream& ss) {
    string n_p, n_m, nc_p, nc_m, g_str; if (!(ss >> n_p >> n_m >> nc_p >> nc_m >> g_str)) {
        printError("Invalid 'add " + name + "' format. Correct: add G<name> <n+> <n-> <nc+> <nc-> <gain>");
        return false;
    }
    return addVCCS_G_Internal(name, n_p, n_m, nc_p, nc_m, g_str);
}


bool Circuit::addCCVS_H_Command(const string& name, stringstream& ss) {
    string n_p, n_m, vctrl_n, g_str; if (!(ss >> n_p >> n_m >> vctrl_n >> g_str)) {
        printError("Invalid 'add " + name + "' format. Correct: add H<name> <n+> <n-> <VctrlName> <gain>");
        return false;
    }
    double gain_val;
    if (!parseComponentValue(g_str, gain_val, "H")) { return false; }
    return addCCVS_H_Internal(name, n_p, n_m, vctrl_n, gain_val);
}


bool Circuit::addCCCS_F_Command(const string& name, stringstream& ss) {
    string n_p, n_m, vctrl_n, g_str;
    if (!(ss >> n_p >> n_m >> vctrl_n >> g_str)) {
        printError("Invalid 'add " + name + "' format. Correct: add F<name> <n+> <n-> <VctrlName> <gain>");
        return false;
    }
    double gain_val;
    if (!parseComponentValue(g_str, gain_val, "F")) { return false; }
    return addCCCS_F_Internal(name, n_p, n_m, vctrl_n, gain_val);
}

void Circuit::deleteResistorCommand(const string& name) {
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

void Circuit::deleteCapacitorCommand(const string& name) {
    if (elements.count(name) && elements[name]->type == "Capacitor") {
        delete elements[name]; elements.erase(name); element_order.erase(remove(element_order.begin(), element_order.end(), name), element_order.end());
        cout << "Capacitor " << name << " deleted successfully." << endl;
    }
    else { printError("Cannot delete capacitor; component " + name + " not found or is not a capacitor."); }
}
void Circuit::deleteInductorCommand(const string& name) {
    if (elements.count(name) && elements[name]->type == "Inductor") {
        delete elements[name]; elements.erase(name); element_order.erase(remove(element_order.begin(), element_order.end(), name), element_order.end());
        cout << "Inductor " << name << " deleted successfully." << endl;
    }
    else {
        printError("Cannot delete inductor; component " + name + " not found or is not an inductor.");
    }
}

void Circuit::deleteDiodeCommand(const string& name) {
    if (elements.count(name) && elements[name]->type == "Diode") {
        delete elements[name]; elements.erase(name); element_order.erase(remove(element_order.begin(), element_order.end(), name), element_order.end());
        cout << "Diode " << name << " deleted successfully." << endl;
    }
    else {
        printError("Cannot delete diode; component " + name + " not found or is not a diode.");
    }
}
void Circuit::deleteIndependentVoltageSourceCommand(const string& name) {
    if (elements.count(name) && elements[name]->type == "IndependentVoltageSource") {
        delete elements[name]; elements.erase(name); element_order.erase(remove(element_order.begin(), element_order.end(), name), element_order.end());
        cout << "IndependentVoltageSource " << name << " deleted successfully." << endl;
    }
    else {
        printError("Cannot delete; component " + name + " not found or is not an IndependentVoltageSource.");
    }
}
void Circuit::deleteIndependentCurrentSourceCommand(const string& name) {
    if (elements.count(name) && elements[name]->type == "IndependentCurrentSource") {
        delete elements[name]; elements.erase(name); element_order.erase(remove(element_order.begin(), element_order.end(), name), element_order.end());
        cout << "IndependentCurrentSource " << name << " deleted successfully." << endl;
    }
    else {
        printError("Cannot delete; component " + name + " not found or is not an IndependentCurrentSource.");
    }
}

void Circuit::deleteVCVS_E_Command(const string& name) {
    if (elements.count(name) && elements[name]->type == "VCVS_E") {
        delete elements[name]; elements.erase(name); element_order.erase(remove(element_order.begin(), element_order.end(), name), element_order.end());
        cout << "VCVS_E " << name << " deleted successfully." << endl;
    }
    else {
        printError("Cannot delete; component " + name + " not found or is not a VCVS_E source.");
    }
}
void Circuit::deleteVCCS_G_Command(const string& name) {
    if (elements.count(name) && elements[name]->type == "VCCS_G") {
        delete elements[name]; elements.erase(name); element_order.erase(remove(element_order.begin(), element_order.end(), name), element_order.end());
        cout << "VCCS_G " << name << " deleted successfully." << endl;
    }
    else {
        printError("Cannot delete; component " + name + " not found or is not a VCCS_G source.");
    }
}

void Circuit::deleteCCVS_H_Command(const string& name) {
    if (elements.count(name) && elements[name]->type == "CCVS_H") {
        delete elements[name]; elements.erase(name); element_order.erase(remove(element_order.begin(), element_order.end(), name), element_order.end());
        cout << "CCVS_H " << name << " deleted successfully." << endl;
    }
    else {
        printError("Cannot delete; component " + name + " not found or is not a CCVS_H source.");
    }
}

void Circuit::deleteCCCS_F_Command(const string& name) {
    if (elements.count(name) && elements[name]->type == "CCCS_F") {
        delete elements[name]; elements.erase(name); element_order.erase(remove(element_order.begin(), element_order.end(), name), element_order.end());
        cout << "CCCS_F " << name << " deleted successfully." << endl;
    }
    else {
        printError("Cannot delete; component " + name + " not found or is not a CCCS_F source.");
    }
}
void Circuit::addGndCommand(stringstream& ss) {
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
void Circuit::deleteGndCommand(stringstream& ss) {
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
bool Circuit::parsePlottableExpression(const string& expr_str, PlottableExpression& out_expr) const {
    out_expr.original_expression = expr_str;


    size_t op_pos = expr_str.find_first_of("+-*/");


    if (op_pos == string::npos) {
        if (parseOutputVariable(expr_str, out_expr.var1)) {
            out_expr.op = ' ';
            return true;
        }
        return false;
    }


    out_expr.op = expr_str[op_pos];


    string lhs_str = expr_str.substr(0, op_pos);
    string rhs_str = expr_str.substr(op_pos + 1);


    lhs_str.erase(lhs_str.find_last_not_of(" \t") + 1);
    rhs_str.erase(0, rhs_str.find_first_not_of(" \t"));


    if (parseOutputVariable(lhs_str, out_expr.var1) && parseOutputVariable(rhs_str, out_expr.var2)) {
        return true;
    }

    return false;
}

void Circuit::displayNodesCommand(stringstream& ss) const {
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
void Circuit::listElementsCommand(stringstream& ss) const {
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
void Circuit::renameNodeCommand(stringstream& ss) {
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

    if (node_positions.count(old_name)) {
        node_positions[new_name] = node_positions[old_name];
        node_positions.erase(old_name);
    }

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
bool Circuit::parseOutputVariable(const string& var_str, OutputVariableRequest& out_req) const {
    if (var_str.length() < 4 || var_str.back() != ')') return false;

    size_t open_paren = var_str.find('(');
    if (open_paren == string::npos) return false;

    string type_str = var_str.substr(0, open_paren);
    transform(type_str.begin(), type_str.end(), type_str.begin(), ::toupper);

    out_req.type = type_str;
    out_req.name = var_str.substr(open_paren + 1, var_str.length() - open_paren - 2);
    out_req.original_request = var_str;

    return !out_req.type.empty() && !out_req.name.empty();
}

set<string> Circuit::nodes_in_use() const {
    set<string> in_use;
    for(const auto& pair : elements){
        if(!pair.second->node1_name.empty()) in_use.insert(pair.second->node1_name);
        if(!pair.second->node2_name.empty()) in_use.insert(pair.second->node2_name);
        if (pair.second->type == "VCVS_E") {
            auto* vcvs_src = static_cast<VCVS_E_Source*>(pair.second);
            if(nodes.count(vcvs_src->ctrl_node1_name)) in_use.insert(vcvs_src->ctrl_node1_name);
            if(nodes.count(vcvs_src->ctrl_node2_name)) in_use.insert(vcvs_src->ctrl_node2_name);
        }
        else if (pair.second->type == "VCCS_G") {
            auto* vccs_src = static_cast<VCCS_G_Source*>(pair.second);
            if(nodes.count(vccs_src->ctrl_node1_name)) in_use.insert(vccs_src->ctrl_node1_name);
            if(nodes.count(vccs_src->ctrl_node2_name)) in_use.insert(vccs_src->ctrl_node2_name);
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

bool Circuit::isCircuitConnected() const {
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

bool Circuit::checkCircuitPrerequisites(string& error_message_out) const {
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








SimulationResult Circuit::handleDcAnalysis(const PrintRequest& request) {
    SimulationResult result;
    Element* dc_source_element = nullptr;
    if (elements.count(request.dc_source_name)) {
        dc_source_element = elements.at(request.dc_source_name);
    }
    if (!dc_source_element || (dc_source_element->type != "IndependentVoltageSource" && dc_source_element->type != "IndependentCurrentSource")) {
        result.success = false;
        result.error_message = "DC source '" + request.dc_source_name + "' not found or is not an independent source.";
        return result;
    }


    result.headers.push_back(request.dc_source_name);
    for (const auto& expr : request.expressions) {
        result.headers.push_back(expr.original_expression);
    }

    map<string, int> node_to_index;
    int index_counter = 0;
    for (const auto& node_name : nodes_in_use()) {
        if (grounded_node_names.find(node_name) == grounded_node_names.end()) {
            node_to_index[node_name] = index_counter++;
        }
    }

    map<string, int> vsource_to_index;
    int vsource_counter = 0;
    for (const auto& pair : elements) {
        if (pair.second->type == "IndependentVoltageSource") {
            vsource_to_index[pair.first] = vsource_counter++;
        }
    }
    int num_nodes = node_to_index.size();
    int num_vsources = vsource_to_index.size();
    int matrix_size = num_nodes + num_vsources;
    if (matrix_size == 0) {
        result.success = false; result.error_message = "Circuit is empty."; return result;
    }

    double start = request.dc_start_val, end = request.dc_end_val, step = request.dc_increment;
    if (step == 0) { result.success = false; result.error_message = "DC increment cannot be zero."; return result; }

    for (double sweep_val = start; (step > 0 ? sweep_val <= end + 1e-9 : sweep_val >= end - 1e-9); sweep_val += step) {
        if (dc_source_element->type == "IndependentVoltageSource") {
            static_cast<IndependentVoltageSource*>(dc_source_element)->dc_value = sweep_val;
        } else {
            static_cast<IndependentCurrentSource*>(dc_source_element)->dc_value = sweep_val;
        }


        vector<vector<double>> A(matrix_size, vector<double>(matrix_size, 0.0));
        vector<double> b(matrix_size, 0.0);
        for (const auto& pair : elements) {
            Element* elem = pair.second;

            const string& n1 = elem->node1_name, &n2 = elem->node2_name;

            bool n1_is_gnd = grounded_node_names.count(n1), n2_is_gnd = grounded_node_names.count(n2);

            int idx1 = n1_is_gnd ? -1 : (node_to_index.count(n1) ? node_to_index.at(n1) : -1);

            int idx2 = n2_is_gnd ? -1 : (node_to_index.count(n2) ? node_to_index.at(n2) : -1);



            if (elem->type == "Resistor") {

                auto* r = static_cast<Resistor*>(elem); double g = 1.0 / r->value;

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
        if (x.empty()) continue;

        vector<double> current_step_results;
        current_step_results.push_back(sweep_val);

        map<string, double> node_voltages;
        for (const auto& pair : node_to_index) { node_voltages[pair.first] = x[pair.second]; }
        for (const string& gnd_node : grounded_node_names) { node_voltages[gnd_node] = 0.0; }

        map<string, double> element_currents;
        for(const auto& pair : vsource_to_index) { element_currents[pair.first] = x[num_nodes + pair.second]; }
        for (const auto& pair : elements) {
            if(pair.second->type == "Resistor"){
                double v1 = node_voltages.count(pair.second->node1_name) ? node_voltages.at(pair.second->node1_name) : 0.0;
                double v2 = node_voltages.count(pair.second->node2_name) ? node_voltages.at(pair.second->node2_name) : 0.0;
                element_currents[pair.first] = (v1 - v2) / static_cast<Resistor*>(pair.second)->value;
            }
        }


        for (const auto& expr : request.expressions) {
            auto get_value = [&](const OutputVariableRequest& var_req) -> double {
                if (var_req.type == "V" && node_voltages.count(var_req.name)) {
                    return node_voltages.at(var_req.name);
                }
                if (var_req.type == "I" && element_currents.count(var_req.name)) {
                    return element_currents.at(var_req.name);
                }
                return NAN;
            };

            double val1 = get_value(expr.var1);
            double final_val = val1;

            if (expr.op != ' ' && !isnan(val1)) {
                double val2 = get_value(expr.var2);
                if (!isnan(val2)) {
                    switch (expr.op) {
                        case '+': final_val = val1 + val2; break;
                        case '-': final_val = val1 - val2; break;
                        case '*': final_val = val1 * val2; break;
                        case '/': final_val = (abs(val2) > 1e-12) ? (val1 / val2) : NAN; break;
                        default:  final_val = NAN; break;
                    }
                } else {
                    final_val = NAN;
                }
            }
            current_step_results.push_back(final_val);
        }
        result.data.push_back(current_step_results);
    }

    result.success = true;
    return result;
}






SimulationResult Circuit::handleAcAnalysis(const PrintRequest& request) {
    SimulationResult result;
    string error_msg;
    if (!checkCircuitPrerequisites(error_msg)) {
        result.success = false;
        result.error_message = error_msg;
        return result;
    }


    Element* ac_source_element = nullptr;
    int ac_source_count = 0;
    for (const auto& pair : elements) {
        if (pair.second->type == "ACVoltageSource") {
            ac_source_element = pair.second;
            ac_source_count++;
        }
    }

    if (ac_source_count == 0) {
        result.success = false;
        result.error_message = "AC analysis requires one AC voltage source (U-type), but none were found.";
        return result;
    }
    if (ac_source_count > 1) {
        result.success = false;
        result.error_message = "AC analysis requires exactly one AC source (U-type), but " + to_string(ac_source_count) + " were found.";
        return result;
    }


    map<string, int> node_to_index;
    int index_counter = 0;
    for (const auto& node_pair : nodes) {
        if (grounded_node_names.find(node_pair.first) == grounded_node_names.end()) {
            node_to_index[node_pair.first] = index_counter++;
        }
    }

    map<string, int> extra_vars_to_index;
    int extra_var_counter = 0;
    for (const auto& pair : elements) {
        const string& type = pair.second->type;

        if (type == "IndependentVoltageSource" || type == "ACVoltageSource" || type == "Inductor" || type == "VCVS_E" || type == "CCVS_H") {
             extra_vars_to_index[pair.first] = extra_var_counter++;
        }
    }

    int num_nodes = node_to_index.size();
    int num_extra_vars = extra_vars_to_index.size();
    int matrix_size = num_nodes + num_extra_vars;

    if (matrix_size == 0) {
        result.success = false;
        result.error_message = "Circuit is empty or contains only ground nodes.";
        return result;
    }


    result.headers.push_back("Frequency (Hz)");
    for (const auto& expr : request.expressions) {
        result.headers.push_back(expr.original_expression);
    }


    if (request.start_freq <= 0 || request.end_freq <= 0 || request.start_freq > request.end_freq) {
        result.success = false;
        result.error_message = "Invalid frequency range for AC sweep. Frequencies must be positive.";
        return result;
    }

    double num_decades = log10(request.end_freq / request.start_freq);
    int total_points = static_cast<int>(request.num_points * num_decades);
    if (total_points <= 0) total_points = 1;


    for (int i = 0; i <= total_points; ++i) {
        double freq;
        if (total_points < 1) {
             freq = request.start_freq;
        } else {

            double decade_span = log10(request.end_freq) - log10(request.start_freq);
            freq = request.start_freq * pow(10.0, (double)i * decade_span / total_points);
        }


        if (freq > request.end_freq) {
            freq = request.end_freq;
        }


        if (freq <= 0) continue;
        double omega = 2.0 * M_PI * freq;
        const complex<double> j(0, 1);


        vector<vector<complex<double>>> A(matrix_size, vector<complex<double>>(matrix_size, 0.0));
        vector<complex<double>> b(matrix_size, 0.0);

        for (const auto& pair : elements) {
            Element* elem = pair.second;
            const string& n1 = elem->node1_name;
            const string& n2 = elem->node2_name;

            int idx1 = grounded_node_names.count(n1) ? -1 : (node_to_index.count(n1) ? node_to_index.at(n1) : -1);
            int idx2 = n2.empty() || grounded_node_names.count(n2) ? -1 : (node_to_index.count(n2) ? node_to_index.at(n2) : -1);


            if (elem->type == "Resistor") {
                complex<double> g = 1.0 / static_cast<Resistor*>(elem)->value;
                if (idx1 != -1) A[idx1][idx1] += g;
                if (idx2 != -1) A[idx2][idx2] += g;
                if (idx1 != -1 && idx2 != -1) { A[idx1][idx2] -= g; A[idx2][idx1] -= g; }
            } else if (elem->type == "Capacitor") {
                complex<double> y = j * omega * static_cast<Capacitor*>(elem)->value;
                if (idx1 != -1) A[idx1][idx1] += y;
                if (idx2 != -1) A[idx2][idx2] += y;
                if (idx1 != -1 && idx2 != -1) { A[idx1][idx2] -= y; A[idx2][idx1] -= y; }
            } else if (elem->type == "Inductor") {
                int l_idx = num_nodes + extra_vars_to_index.at(elem->name);
                complex<double> z = j * omega * static_cast<Inductor*>(elem)->value;
                if (idx1 != -1) { A[idx1][l_idx] += 1.0; A[l_idx][idx1] += 1.0; }
                if (idx2 != -1) { A[idx2][l_idx] -= 1.0; A[l_idx][idx2] -= 1.0; }
                A[l_idx][l_idx] -= z;
            } else if (elem->type == "IndependentVoltageSource") {
                int v_idx = num_nodes + extra_vars_to_index.at(elem->name);
                if (idx1 != -1) { A[idx1][v_idx] += 1.0; A[v_idx][idx1] += 1.0; }
                if (idx2 != -1) { A[idx2][v_idx] -= 1.0; A[v_idx][idx2] -= 1.0; }
            } else if (elem->type == "ACVoltageSource") {
                auto* ac_vsrc = static_cast<ACVoltageSource*>(elem);
                int v_idx = num_nodes + extra_vars_to_index.at(ac_vsrc->name);
                if (idx1 != -1) { A[idx1][v_idx] += 1.0; A[v_idx][idx1] += 1.0; }
                if (idx2 != -1) { A[idx2][v_idx] -= 1.0; A[v_idx][idx2] -= 1.0; }
                b[v_idx] += ac_vsrc->ac_magnitude;
            } else if (elem->type == "IndependentCurrentSource") {

            } else if (elem->type == "VCVS_E") {
                auto* vcvs = static_cast<VCVS_E_Source*>(elem);
                int v_idx = num_nodes + extra_vars_to_index.at(vcvs->name);
                int c_idx1 = grounded_node_names.count(vcvs->ctrl_node1_name) ? -1 : node_to_index.at(vcvs->ctrl_node1_name);
                int c_idx2 = grounded_node_names.count(vcvs->ctrl_node2_name) ? -1 : node_to_index.at(vcvs->ctrl_node2_name);
                if (idx1 != -1) A[idx1][v_idx] += 1.0;
                if (idx2 != -1) A[idx2][v_idx] -= 1.0;
                if (idx1 != -1) A[v_idx][idx1] += 1.0;
                if (idx2 != -1) A[v_idx][idx2] -= 1.0;
                if (c_idx1 != -1) A[v_idx][c_idx1] -= vcvs->gain;
                if (c_idx2 != -1) A[v_idx][c_idx2] += vcvs->gain;
            } else if (elem->type == "VCCS_G") {
                auto* vccs = static_cast<VCCS_G_Source*>(elem);
                double g = vccs->gain;
                int c_idx1 = grounded_node_names.count(vccs->ctrl_node1_name) ? -1 : node_to_index.at(vccs->ctrl_node1_name);
                int c_idx2 = grounded_node_names.count(vccs->ctrl_node2_name) ? -1 : node_to_index.at(vccs->ctrl_node2_name);
                if (idx1 != -1 && c_idx1 != -1) A[idx1][c_idx1] += g;
                if (idx1 != -1 && c_idx2 != -1) A[idx1][c_idx2] -= g;
                if (idx2 != -1 && c_idx1 != -1) A[idx2][c_idx1] -= g;
                if (idx2 != -1 && c_idx2 != -1) A[idx2][c_idx2] += g;
            } else if (elem->type == "CCVS_H") {
                 auto* ccvs = static_cast<CCVS_H_Source*>(elem);
                 int h_idx = num_nodes + extra_vars_to_index.at(ccvs->name);
                 int vctrl_idx = num_nodes + extra_vars_to_index.at(ccvs->controlling_vsource_name);
                 if (idx1 != -1) A[idx1][h_idx] += 1.0;
                 if (idx2 != -1) A[idx2][h_idx] -= 1.0;
                 if (idx1 != -1) A[h_idx][idx1] += 1.0;
                 if (idx2 != -1) A[h_idx][idx2] -= 1.0;
                 A[h_idx][vctrl_idx] -= ccvs->gain;
            } else if (elem->type == "CCCS_F") {
                auto* cccs = static_cast<CCCS_F_Source*>(elem);
                int vctrl_idx = num_nodes + extra_vars_to_index.at(cccs->controlling_vsource_name);
                if (idx1 != -1) A[idx1][vctrl_idx] += cccs->gain;
                if (idx2 != -1) A[idx2][vctrl_idx] -= cccs->gain;
            }
        }

        vector<complex<double>> x = solveComplexLinearSystem(A, b);
        if (x.empty()) continue;


        map<string, complex<double>> node_phasors;
        for (const auto& pair : node_to_index) node_phasors[pair.first] = x[pair.second];
        for (const string& gnd_node : grounded_node_names) node_phasors[gnd_node] = 0.0;

        map<string, complex<double>> element_current_phasors;
        for(const auto& pair : extra_vars_to_index) element_current_phasors[pair.first] = x[num_nodes + pair.second];

        for (const auto& pair : elements) {
            if (element_current_phasors.count(pair.first)) continue;
            Element* elem = pair.second;
            complex<double> v1 = node_phasors.at(elem->node1_name);
            complex<double> v2 = elem->node2_name.empty() ? 0.0 : node_phasors.at(elem->node2_name);
            complex<double> voltage_phasor = v1 - v2;
            complex<double> current_phasor = {0.0, 0.0};
            if(elem->type == "Resistor") {
                current_phasor = voltage_phasor / static_cast<Resistor*>(elem)->value;
            } else if(elem->type == "Capacitor") {
                current_phasor = voltage_phasor * (j * omega * static_cast<Capacitor*>(elem)->value);
            } else if (elem->type == "IndependentCurrentSource") {
                current_phasor = 0.0;
            } else if (elem->type == "VCCS_G") {
                auto* vccs = static_cast<VCCS_G_Source*>(elem);
                complex<double> vc1 = node_phasors.at(vccs->ctrl_node1_name);
                complex<double> vc2 = node_phasors.at(vccs->ctrl_node2_name);
                current_phasor = vccs->gain * (vc1 - vc2);
            } else if (elem->type == "CCCS_F") {
                auto* cccs = static_cast<CCCS_F_Source*>(elem);
                complex<double> i_ctrl = element_current_phasors.at(cccs->controlling_vsource_name);
                current_phasor = cccs->gain * i_ctrl;
            }
            element_current_phasors[pair.first] = current_phasor;
        }


        vector<double> current_step_results;
        current_step_results.push_back(freq);

        for (const auto& expr : request.expressions) {
            auto get_phasor_value = [&](const OutputVariableRequest& var_req) -> complex<double> {
                if ((var_req.type == "V" || var_req.type == "VP" || var_req.type == "VDB") && node_phasors.count(var_req.name)) {
                    return node_phasors.at(var_req.name);
                }
                if ((var_req.type == "I" || var_req.type == "IP" || var_req.type == "IDB") && element_current_phasors.count(var_req.name)) {
                    return element_current_phasors.at(var_req.name);
                }
                return {NAN, NAN};
            };

            complex<double> phasor1 = get_phasor_value(expr.var1);
            complex<double> final_phasor = phasor1;

            if (expr.op != ' ' && !isnan(phasor1.real())) {
                complex<double> phasor2 = get_phasor_value(expr.var2);
                if (!isnan(phasor2.real())) {
                    switch (expr.op) {
                        case '+': final_phasor = phasor1 + phasor2; break;
                        case '-': final_phasor = phasor1 - phasor2; break;
                        case '*': final_phasor = phasor1 * phasor2; break;
                        case '/': final_phasor = (abs(phasor2) > 1e-12) ? (phasor1 / phasor2) : complex<double>(NAN, NAN); break;
                        default:  final_phasor = {NAN, NAN}; break;
                    }
                } else {
                    final_phasor = {NAN, NAN};
                }
            }

            double val_to_print = NAN;
            if (!isnan(final_phasor.real())) {
                if (expr.var1.type == "VDB" || expr.var1.type == "IDB") {
                    val_to_print = (abs(final_phasor) > 1e-12) ? 20.0 * log10(abs(final_phasor)) : -240.0;
                } else if (expr.var1.type == "VP" || expr.var1.type == "IP") {
                    val_to_print = arg(final_phasor) * 180.0 / M_PI;
                } else {
                    val_to_print = abs(final_phasor);
                }
            }
            current_step_results.push_back(val_to_print);
        }
        result.data.push_back(current_step_results);

        if (freq >= request.end_freq || total_points < 1) break;
    }

    result.success = true;
    return result;
}

SimulationResult Circuit::handleTranAnalysis(const PrintRequest& request) {
    SimulationResult result;

    result.headers.push_back("Time (s)");
    for(const auto& expr : request.expressions) {
        result.headers.push_back(expr.original_expression);
    }

    if (request.tstep <= 0) {
        result.success = false; result.error_message = "Tstep must be positive."; return result;
    }


    map<string, int> node_to_index;
    int index_counter = 0;
    for (const string& node_name : nodes_in_use()) {
        if (grounded_node_names.find(node_name) == grounded_node_names.end()) {

            node_to_index[node_name] = index_counter++;

        }

    }
    map<string, int> extra_vars_to_index;
    int extra_var_counter = 0;
    for (const auto& pair : elements) {
        if (pair.second->type == "IndependentVoltageSource" || pair.second->type == "Inductor") {

            extra_vars_to_index[pair.first] = extra_var_counter++;

        }

    }
    int num_nodes = node_to_index.size();
    int num_extra_vars = extra_vars_to_index.size();
    int matrix_size = num_nodes + num_extra_vars;

    if (matrix_size == 0) {
        result.success = false; result.error_message = "Circuit is empty."; return result;
    }

    for (auto const& pair : nodes) { pair.second->voltage = 0.0; }
    for (auto const& pair : elements) { pair.second->voltage = 0.0; pair.second->current = 0.0; }

    if (request.tstart == 0.0) {
        vector<double> initial_step_results;
        initial_step_results.push_back(0.0);

        for(const auto& expr : request.expressions) { initial_step_results.push_back(0.0); }
        result.data.push_back(initial_step_results);
    }

    for (double t = request.tstep; t <= request.tstop + request.tstep/2; t += request.tstep) {

        vector<vector<double>> A(matrix_size, vector<double>(matrix_size, 0.0));
        vector<double> b(matrix_size, 0.0);
        for (const auto& pair : elements) {
            Element* elem = pair.second;

            const string& n1 = elem->node1_name, &n2 = elem->node2_name;

            bool n1_is_gnd = grounded_node_names.count(n1), n2_is_gnd = grounded_node_names.count(n2);

            int idx1 = n1_is_gnd ? -1 : (node_to_index.count(n1) ? node_to_index.at(n1) : -1);

            int idx2 = n2_is_gnd ? -1 : (node_to_index.count(n2) ? node_to_index.at(n2) : -1);



            if (elem->type == "Resistor") {

                auto* r = static_cast<Resistor*>(elem); double g = 1.0 / r->value;

                if (idx1 != -1) A[idx1][idx1] += g;

                if (idx2 != -1) A[idx2][idx2] += g;

                if (idx1 != -1 && idx2 != -1) { A[idx1][idx2] -= g; A[idx2][idx1] -= g; }

            } else if (elem->type == "Capacitor") {

                auto* c = static_cast<Capacitor*>(elem); double g_eq = c->value / request.tstep; double i_eq = g_eq * c->voltage;

                if (idx1 != -1) { A[idx1][idx1] += g_eq; b[idx1] -= i_eq; }

                if (idx2 != -1) { A[idx2][idx2] += g_eq; b[idx2] += i_eq; }

                if (idx1 != -1 && idx2 != -1) { A[idx1][idx2] -= g_eq; A[idx2][idx1] -= g_eq; }

            } else if (elem->type == "Inductor") {

                auto* l = static_cast<Inductor*>(elem); int l_idx = num_nodes + extra_vars_to_index.at(l->name);

                double r_eq = l->value / request.tstep; double v_eq = r_eq * l->current;

                if (idx1 != -1) A[idx1][l_idx] += 1.0;

                if (idx2 != -1) A[idx2][l_idx] -= 1.0;

                A[l_idx][l_idx] = -r_eq;

                if (idx1 != -1) A[l_idx][idx1] += 1.0;

                if (idx2 != -1) A[l_idx][idx2] -= 1.0;

                b[l_idx] -= v_eq;

            } else if (elem->type == "IndependentVoltageSource") {

                auto* vsrc = static_cast<IndependentVoltageSource*>(elem);

                int v_idx = num_nodes + extra_vars_to_index.at(vsrc->name);

                double val = vsrc->dc_value;

                if(vsrc->source_form == "SIN") { val = vsrc->v_offset + vsrc->v_amplitude * sin(2 * M_PI * vsrc->frequency * t); }

                if (idx1 != -1) { A[idx1][v_idx] += 1.0; A[v_idx][idx1] += 1.0; }

                if (idx2 != -1) { A[idx2][v_idx] -= 1.0; A[v_idx][idx2] -= 1.0; }

                b[v_idx] += val;

            }

        }
        vector<double> x = solveLinearSystem(A, b);
        if (x.empty()) {
            result.success = false;

            result.error_message = "Solver failed at t = " + to_string(t);

            return result;

        }








        for (auto const& pair : node_to_index) {

            nodes[pair.first]->voltage = x[pair.second];

        }







        for (auto const& pair : elements) {

            const string& name = pair.first;

            Element* elem_ptr = pair.second;





            double v1_new = grounded_node_names.count(elem_ptr->node1_name) ? 0.0 : (nodes.count(elem_ptr->node1_name) ? nodes.at(elem_ptr->node1_name)->voltage : 0.0);

            double v2_new = grounded_node_names.count(elem_ptr->node2_name) ? 0.0 : (nodes.count(elem_ptr->node2_name) ? nodes.at(elem_ptr->node2_name)->voltage : 0.0);



            double old_voltage = elem_ptr->voltage;

            elem_ptr->voltage = v1_new - v2_new;





            if (extra_vars_to_index.count(name)) {

                elem_ptr->current = x[num_nodes + extra_vars_to_index.at(name)];

            }



            else if (elem_ptr->type == "Capacitor"){

                if(request.tstep > 0) {

                    static_cast<Capacitor*>(elem_ptr)->current = static_cast<Capacitor*>(elem_ptr)->value * (elem_ptr->voltage - old_voltage) / request.tstep;

                }

            }

        }



        if (t >= request.tstart) {
            vector<double> current_step_results;
            current_step_results.push_back(t);

            for(const auto& expr : request.expressions) {
                auto get_value = [&](const OutputVariableRequest& var_req) -> double {
                    if(var_req.type == "V" && nodes.count(var_req.name)){
                        return grounded_node_names.count(var_req.name) ? 0.0 : nodes.at(var_req.name)->voltage;
                    }
                    if (var_req.type == "I" && elements.count(var_req.name)) {
                        return elements.at(var_req.name)->current;
                    }
                    return NAN;
                };

                double val1 = get_value(expr.var1);
                double final_val = val1;

                if (expr.op != ' ' && !isnan(val1)) {
                    double val2 = get_value(expr.var2);
                    if (!isnan(val2)) {
                        switch (expr.op) {
                            case '+': final_val = val1 + val2; break;
                            case '-': final_val = val1 - val2; break;
                            case '*': final_val = val1 * val2; break;
                            case '/': final_val = (abs(val2) > 1e-12) ? (val1 / val2) : NAN; break;
                            default:  final_val = NAN; break;
                        }
                    } else {
                        final_val = NAN;
                    }
                }
                current_step_results.push_back(final_val);
            }
            result.data.push_back(current_step_results);
        }
    }
    result.success = true;
    return result;
}
void Circuit::displayCircuit() const {
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











bool Circuit::processCommand(const string& command_line) {
    stringstream ss(command_line);
    string command_verb_orig; ss >> command_verb_orig;
    if (command_verb_orig.empty()) return true;

    string command_verb_lower = command_verb_orig;
    transform(command_verb_lower.begin(), command_verb_lower.end(), command_verb_lower.begin(), ::tolower);

    bool success = true;

    if (command_verb_lower == "add") {
        string full_name_or_type;
        if (!(ss >> full_name_or_type)) { printError("Syntax error: 'add' command requires element name or 'GND'."); return false; }
        if (full_name_or_type == "GND") { addGndCommand(ss); }
        else {
            char type_char = toupper(full_name_or_type[0]);
            if (type_char == 'R' || type_char == 'W') {
                success = addResistorCommand(full_name_or_type, ss);
            }
            else if (type_char == 'C') success = addCapacitorCommand(full_name_or_type, ss);
            else if (type_char == 'L') success = addInductorCommand(full_name_or_type, ss);
            else if (type_char == 'D') success = addDiodeCommand(full_name_or_type, ss);
            else if (type_char == 'V') success = addIndependentVoltageSourceCommand(full_name_or_type, ss);
            else if (type_char == 'I') success = addIndependentCurrentSourceCommand(full_name_or_type, ss);
            else if (type_char == 'E') success = addVCVS_E_Command(full_name_or_type, ss);
            else if (type_char == 'G') success = addVCCS_G_Command(full_name_or_type, ss);
            else if (type_char == 'H') success = addCCVS_H_Command(full_name_or_type, ss);
            else if (type_char == 'F') success = addCCCS_F_Command(full_name_or_type, ss);
            else if (type_char == 'U') success = addACVoltageSourceCommand(full_name_or_type, ss);
            else { printError("Unknown element type for 'add': " + full_name_or_type); success = false; }
        }
    } else if (command_verb_lower == "delete") {
        string element_name_to_delete; ss >> element_name_to_delete;
        if (element_name_to_delete.empty()) { printError("Syntax error: 'delete' command requires an element name."); return false; }
        if (element_name_to_delete == "GND") {
            string node_to_unground;
            if (ss >> node_to_unground) { stringstream temp_ss; temp_ss << node_to_unground; deleteGndCommand(temp_ss); }
            else { printError("Syntax error: 'delete GND' requires a node name."); return false; }
        } else {
            deleteElementAndCleanNodes(element_name_to_delete);
        }
    }
    else if (command_verb_orig == ".nodes") { displayNodesCommand(ss); }
    else if (command_verb_orig == ".list") { listElementsCommand(ss); }
    else if (command_verb_orig == ".rename") { renameNodeCommand(ss); }
    else if (command_verb_lower == "display"){ displayCircuit(); }
    else if (command_verb_lower == "exit" || command_verb_lower == "return" || command_verb_lower == "save" ){  }
    else {
        string line_with_add = "add " + command_line;
        success = processCommand(line_with_add);
    }
    return success;
}

Simulator::Simulator(CircuitRenderer* renderer) : renderer_ptr(renderer) {}

const vector<string>& Simulator::getSchematicOrder() const { return schematic_order; }
const map<string, string>& Simulator::getSchematicPaths() const { return schematic_paths; }

string Simulator::getSchematicName(const string& filepath) {
    size_t last_slash = filepath.find_last_of("/\\");
    size_t last_dot = filepath.find_last_of(".");
    string name = (last_slash == string::npos) ? filepath : filepath.substr(last_slash + 1);
    if (last_dot != string::npos && last_dot > last_slash) {
        name = name.substr(0, name.find_last_of("."));
    }
    return name;
}

void Simulator::saveCircuit(const string& schematic_name) {
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

void Simulator::loadCircuitFromFile(const string& filepath) {
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
    cout << "Finished loading from file: " << filepath << ". Displaying circuit." << endl;
    infile.close();
    if (renderer_ptr) {
        renderer_ptr->setActiveCircuit(&circuits[schematic_name]);
    }
}

void Simulator::createNewCircuit(const string& filepath) {
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


    if (renderer_ptr) {
        renderer_ptr->setActiveCircuit(&circuits[schematic_name]);
    }

}

void Simulator::showAndSelectSchematics() {

    if (renderer_ptr) {
        renderer_ptr->setState(CircuitRenderer::FILE_SELECT);
    }
}



CircuitRenderer::CircuitRenderer() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printError("SDL could not initialize! SDL_Error: " + string(SDL_GetError()));
        return;
    }
    if (TTF_Init() == -1) {
        printError("SDL_ttf could not initialize! TTF_Error: " + string(TTF_GetError()));
        SDL_Quit();
        return;
    }

    window = SDL_CreateWindow("Circuit Simulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_width, window_height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        printError("Window could not be created! SDL_Error: " + string(SDL_GetError()));
        SDL_Quit();
        TTF_Quit();
        return;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        printError("Renderer could not be created! SDL_Error: " + string(SDL_GetError()));
        SDL_DestroyWindow(window);
        SDL_Quit();
        TTF_Quit();
        return;
    }

    font = TTF_OpenFont("arial.ttf", 18);
    if (!font) {
        printError("Failed to load font! TTF_Error: " + string(TTF_GetError()) + ". Text rendering might be affected.");

    }
    setState(MAIN_MENU);
}

CircuitRenderer::~CircuitRenderer() {
    if (font) TTF_CloseFont(font);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}

void CircuitRenderer::setSimulator(Simulator* sim) {
    simulator_ptr = sim;
}

void CircuitRenderer::setActiveCircuit(Circuit* circuit) {
    active_circuit = circuit;
    if (active_circuit) {
        autoLayoutNodes();
        setState(CIRCUIT_VIEW_IDLE);
    }
}


bool CircuitRenderer::executeCommand(const string& command) {
    if (!active_circuit) {
        console_output_buffer.push_back("Error: No active circuit to execute command.");
        return false;
    }


    stringstream captured_output;
    streambuf* old_cout = cout.rdbuf(captured_output.rdbuf());
    streambuf* old_cerr = cerr.rdbuf(captured_output.rdbuf());

    bool command_success = active_circuit->processCommand(command);


    cout.rdbuf(old_cout);
    cerr.rdbuf(old_cerr);


    string output_line;
    while (getline(captured_output, output_line)) {
        console_output_buffer.push_back(output_line);
        if (console_output_buffer.size() > MAX_CONSOLE_LINES) {
            console_output_buffer.erase(console_output_buffer.begin());
        }
    }
    return command_success;
}


void CircuitRenderer::render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    switch (current_state) {
        case MAIN_MENU:
            drawMainMenu();
            break;
        case FILE_SELECT:
            if (simulator_ptr) {
                drawFileSelectionMenu(simulator_ptr->getSchematicOrder(), simulator_ptr->getSchematicPaths());
            }
            break;
        case CIRCUIT_VIEW_IDLE:
        case CIRCUIT_VIEW_PLACING_COMPONENT:
        case CIRCUIT_VIEW_DRAWING_WIRE:
            drawCircuitView();
            break;
        case INPUT_FILENAME_ADD:
        case INPUT_FILENAME_NEW:
            drawTextInputBox();
            break;
        case INPUT_COMPONENT_PARAMS:
            drawComponentParamsInputModal();
            break;
        case SIMULATION_SETUP:
            drawSimulationSetup();
            break;
        case PLOT_VIEW:
            drawPlotView();
            break;
    }
    SDL_RenderPresent(renderer);
}



void CircuitRenderer::resetPlotView() {
    plot_cursor_active = false;

    if (plot_data.data.empty() || plot_data.data[0].empty()) {
        return;
    }


    plot_view_xmin = plot_view_xmax = plot_data.data[0][0];
    plot_view_ymin = numeric_limits<double>::max();
    plot_view_ymax = -numeric_limits<double>::max();

    bool y_range_found = false;
    for(const auto& row : plot_data.data) {
        if (row[0] < plot_view_xmin) plot_view_xmin = row[0];
        if (row[0] > plot_view_xmax) plot_view_xmax = row[0];
        for(size_t i = 1; i < row.size(); ++i) {
            if (!isnan(row[i])) {
                if (row[i] < plot_view_ymin) plot_view_ymin = row[i];
                if (row[i] > plot_view_ymax) plot_view_ymax = row[i];
                y_range_found = true;
            }
        }
    }

    if (!y_range_found) {
        plot_view_ymin = -1.0;
        plot_view_ymax = 1.0;
    }

    double y_range = plot_view_ymax - plot_view_ymin;
    double y_padding;

    if (y_range < 1e-9) {
        y_padding = (abs(plot_view_ymin) > 1e-9) ? abs(plot_view_ymin) * 0.1 : 0.1;
    } else {
        y_padding = y_range * 0.05;
    }
    plot_view_ymin -= y_padding;
    plot_view_ymax += y_padding;

    double x_range = plot_view_xmax - plot_view_xmin;
    double x_padding = (x_range < 1e-9) ? 0.5 : x_range * 0.05;
    plot_view_xmin -= x_padding;
    plot_view_xmax += x_padding;
}





void CircuitRenderer::runAndPlotSimulation() {
    if (!active_circuit) {
        console_output_buffer.push_back("Error: No active circuit to simulate.");
        return;
    }
    string error_msg;
    if (!active_circuit->checkCircuitPrerequisites(error_msg)){
        console_output_buffer.push_back("Error: " + error_msg);
        return;
    }

    PrintRequest request;
    request.analysis_type = sim_analysis_type;
    request.expressions = sim_plottable_expressions;

    try {
        if (sim_analysis_type == "DC") {
            request.dc_source_name = sim_dc_source_name;
            request.dc_start_val = stod(sim_dc_start_val);
            request.dc_end_val = stod(sim_dc_end_val);
            request.dc_increment = stod(sim_dc_increment);
        }  else if (sim_analysis_type == "TRAN") {
            request.tstep = stod(sim_tran_tstep);
            request.tstop = stod(sim_tran_tstop);
            request.tstart = sim_tran_tstart.empty() ? 0.0 : stod(sim_tran_tstart);
        } else if (sim_analysis_type == "AC") {
            request.ac_source_name = sim_ac_source_name;
            request.num_points  = stoi(sim_ac_points);
            request.start_freq = stod(sim_ac_start_freq);
            request.end_freq = stod(sim_ac_end_freq);
        }
    } catch (...) {
        console_output_buffer.push_back("Error: Invalid simulation parameter format.");
        return;
    }

    SimulationResult result;
    if (request.analysis_type == "DC") result = active_circuit->handleDcAnalysis(request);
    else if (request.analysis_type == "TRAN") result = active_circuit->handleTranAnalysis(request);
    else if (request.analysis_type == "AC") result = active_circuit->handleAcAnalysis(request);

    if (result.success) {
        plot_data = result;
        resetPlotView();
        setState(PLOT_VIEW);
    } else {
        console_output_buffer.push_back("Simulation Failed: " + result.error_message);
        setState(CIRCUIT_VIEW_IDLE);
    }
}


void CircuitRenderer::drawPlotView() {
    drawButton("Back to Circuit", 50, window_height - 45, 150, 35, {100, 100, 100, 255}, {255, 255, 255, 255});

    if (plot_data.data.empty() || plot_data.data[0].empty()) {
        drawText("No data to plot.", 100, 100, {255, 100, 100, 255});
        return;
    }

    SDL_Rect plot_area = {100, 60, window_width - 200, window_height - 140};
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    fillRect(plot_area.x, plot_area.y, plot_area.w, plot_area.h);

    double x_range = (plot_view_xmax - plot_view_xmin);
    double y_range = (plot_view_ymax - plot_view_ymin);
    if (x_range == 0) x_range = 1.0;
    if (y_range == 0) y_range = 1.0;

    const int num_grid_lines = 10;


    for (int i = 0; i <= num_grid_lines; ++i) {
        double data_val_x = plot_view_xmin + (i / (double)num_grid_lines) * x_range;
        int screen_x = plot_area.x + static_cast<int>((i / (double)num_grid_lines) * plot_area.w);
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        drawLine(screen_x, plot_area.y, screen_x, plot_area.y + plot_area.h);



        string x_label = formatFrequency(data_val_x);
        drawText(x_label, screen_x - 15, plot_area.y + plot_area.h + 5, {200, 200, 200, 255});
    }
    for (int i = 0; i <= num_grid_lines; ++i) {
        double data_val_y = plot_view_ymin + (i / (double)num_grid_lines) * y_range;
        int screen_y = plot_area.y + plot_area.h - static_cast<int>((i / (double)num_grid_lines) * plot_area.h);
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        drawLine(plot_area.x, screen_y, plot_area.x + plot_area.w, screen_y);
        stringstream ssy; ssy << scientific << setprecision(2) << data_val_y;
        drawText(ssy.str(), plot_area.x - 80, screen_y - 8, {200, 200, 200, 255});
    }

    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    drawRect(plot_area.x, plot_area.y, plot_area.w, plot_area.h);
    drawText(plot_data.headers[0], plot_area.x + plot_area.w / 2 - 20, plot_area.y + plot_area.h + 25, {255,255,255,255});


    SDL_Color colors[] = {{100, 255, 100, 255}, {100, 100, 255, 255}, {255, 100, 100, 255}, {255, 255, 100, 255}};
    for(size_t series_idx = 1; series_idx < plot_data.headers.size(); ++series_idx) {
        SDL_Color color = colors[(series_idx - 1) % 4];
        drawText(plot_data.headers[series_idx], plot_area.x + plot_area.w + 10, plot_area.y + (series_idx-1)*20, color);
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        for(size_t i = 1; i < plot_data.data.size(); ++i) {
            if(isnan(plot_data.data[i-1][series_idx]) || isnan(plot_data.data[i][series_idx])) continue;
            int x1 = plot_area.x + static_cast<int>(((plot_data.data[i-1][0] - plot_view_xmin) / x_range) * plot_area.w);
            int y1 = plot_area.y + plot_area.h - static_cast<int>(((plot_data.data[i-1][series_idx] - plot_view_ymin) / y_range) * plot_area.h);
            int x2 = plot_area.x + static_cast<int>(((plot_data.data[i][0] - plot_view_xmin) / x_range) * plot_area.w);
            int y2 = plot_area.y + plot_area.h - static_cast<int>(((plot_data.data[i][series_idx] - plot_view_ymin) / y_range) * plot_area.h);
            SDL_RenderSetClipRect(renderer, &plot_area);
            drawLine(x1, y1, x2, y2);
            SDL_RenderSetClipRect(renderer, NULL);
        }
    }

    if (plot_cursor_active) {
        size_t idx = plot_cursor_data_index;
        const auto& row_data = plot_data.data[idx];
        double cursor_x_data = row_data[0];

        int cursor_x_screen = plot_area.x + static_cast<int>(((cursor_x_data - plot_view_xmin) / x_range) * plot_area.w);

        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 150);
        drawLine(cursor_x_screen, plot_area.y, cursor_x_screen, plot_area.y + plot_area.h);

        stringstream readout_ss;
        readout_ss << fixed << setprecision(4) << plot_data.headers[0] << ": " << formatFrequency(cursor_x_data);

        for (size_t series_idx = 1; series_idx < row_data.size(); ++series_idx) {
            double cursor_y_data = row_data[series_idx];
            if (!isnan(cursor_y_data)) {
                int cursor_y_screen = plot_area.y + plot_area.h - static_cast<int>(((cursor_y_data - plot_view_ymin) / y_range) * plot_area.h);
                SDL_Color color = colors[(series_idx - 1) % 4];
                SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
                drawCircle(cursor_x_screen, cursor_y_screen, 5);
                readout_ss << "  |  " << plot_data.headers[series_idx] << ": " << cursor_y_data;
            }
        }
        drawText(readout_ss.str(), plot_area.x, plot_area.y - 25, {255, 255, 0, 255});
    }


    int btn_x = window_width - 80;
    drawButton("V+", btn_x, 100, 30, 30, {80, 80, 80, 255}, {255, 255, 255, 255});
    drawButton("V-", btn_x, 140, 30, 30, {80, 80, 80, 255}, {255, 255, 255, 255});
    drawButton("H+", btn_x, 180, 30, 30, {80, 80, 80, 255}, {255, 255, 255, 255});
    drawButton("H-", btn_x, 220, 30, 30, {80, 80, 80, 255}, {255, 255, 255, 255});
    drawButton("Reset", btn_x - 10, 260, 50, 30, {120, 120, 80, 255}, {255, 255, 255, 255});
}
void CircuitRenderer::handleTextInput(SDL_Event& e) {

    if (e.type == SDL_TEXTINPUT) {
        if (current_state == INPUT_FILENAME_ADD || current_state == INPUT_FILENAME_NEW) {
            current_input_text += e.text.text;
        } else if (current_state == CIRCUIT_VIEW_IDLE && command_input_active) {
            command_input_text += e.text.text;
        } else if (current_state == INPUT_COMPONENT_PARAMS) {

            switch (current_input_focus) {
                case NAME_FIELD: current_param_input_name += e.text.text; break;
                case NODE1_FIELD: current_param_input_node1 += e.text.text; break;
                case NODE2_FIELD: current_param_input_node2 += e.text.text; break;
                case VALUE_FIELD: current_param_input_value += e.text.text; break;
                case VOFF_FIELD: current_param_input_voff += e.text.text; break;
                case VAMP_FIELD: current_param_input_vamp += e.text.text; break;
                case FREQ_FIELD: current_param_input_freq += e.text.text; break;
                case MODEL_FIELD: current_param_input_model += e.text.text; break;
                case CTRL_NODE1_FIELD: current_param_input_ctrl_node1 += e.text.text; break;
                case CTRL_NODE2_FIELD: current_param_input_ctrl_node2 += e.text.text; break;
                case CTRL_VSOURCE_FIELD: current_param_input_ctrl_vsource += e.text.text; break;
                case GAIN_FIELD: current_param_input_gain += e.text.text; break;
                default: break;
            }
        } else if (current_state == SIMULATION_SETUP) {
            switch (current_input_focus) {
                case DC_SOURCE_FIELD: sim_dc_source_name += e.text.text; break;
                case DC_START_FIELD: sim_dc_start_val += e.text.text; break;
                case DC_END_FIELD: sim_dc_end_val += e.text.text; break;
                case DC_INCREMENT_FIELD: sim_dc_increment += e.text.text; break;
                case TRAN_TSTEP_FIELD: sim_tran_tstep += e.text.text; break;
                case TRAN_TSTOP_FIELD: sim_tran_tstop += e.text.text; break;
                case TRAN_TSTART_FIELD: sim_tran_tstart += e.text.text; break;
                case TRAN_TMAXSTEP_FIELD: sim_tran_tmaxstep += e.text.text; break;
                case AC_SOURCE_FIELD: sim_ac_source_name += e.text.text; break;
                case AC_POINTS_FIELD: sim_ac_points += e.text.text; break;
                case AC_START_FREQ_FIELD: sim_ac_start_freq += e.text.text; break;
                case AC_END_FREQ_FIELD: sim_ac_end_freq += e.text.text; break;
                case VAR_INPUT_FIELD: sim_var_input_text += e.text.text; break;
                default: break;
            }
        }
    }

    else if (e.type == SDL_KEYDOWN) {
        if (e.key.keysym.sym == SDLK_BACKSPACE) {
            if (current_state == INPUT_FILENAME_ADD || current_state == INPUT_FILENAME_NEW) {
                if (!current_input_text.empty()) current_input_text.pop_back();
            } else if (current_state == CIRCUIT_VIEW_IDLE && command_input_active) {
                if (!command_input_text.empty()) command_input_text.pop_back();
            } else if (current_state == INPUT_COMPONENT_PARAMS) {

                switch (current_input_focus) {
                    case NAME_FIELD: if (!current_param_input_name.empty()) current_param_input_name.pop_back(); break;
                    case NODE1_FIELD: if (!current_param_input_node1.empty()) current_param_input_node1.pop_back(); break;
                    case NODE2_FIELD: if (!current_param_input_node2.empty()) current_param_input_node2.pop_back(); break;
                    case VALUE_FIELD: if (!current_param_input_value.empty()) current_param_input_value.pop_back(); break;
                    case VOFF_FIELD: if (!current_param_input_voff.empty()) current_param_input_voff.pop_back(); break;
                    case VAMP_FIELD: if (!current_param_input_vamp.empty()) current_param_input_vamp.pop_back(); break;
                    case FREQ_FIELD: if (!current_param_input_freq.empty()) current_param_input_freq.pop_back(); break;
                    case MODEL_FIELD: if (!current_param_input_model.empty()) current_param_input_model.pop_back(); break;
                    case CTRL_NODE1_FIELD: if (!current_param_input_ctrl_node1.empty()) current_param_input_ctrl_node1.pop_back(); break;
                    case CTRL_NODE2_FIELD: if (!current_param_input_ctrl_node2.empty()) current_param_input_ctrl_node2.pop_back(); break;
                    case CTRL_VSOURCE_FIELD: if (!current_param_input_ctrl_vsource.empty()) current_param_input_ctrl_vsource.pop_back(); break;
                    case GAIN_FIELD: if (!current_param_input_gain.empty()) current_param_input_gain.pop_back(); break;
                    default: break;
                }
            } else if (current_state == SIMULATION_SETUP) {
                switch (current_input_focus) {
                    case DC_SOURCE_FIELD: if (!sim_dc_source_name.empty()) sim_dc_source_name.pop_back(); break;
                    case DC_START_FIELD: if (!sim_dc_start_val.empty()) sim_dc_start_val.pop_back(); break;
                    case DC_END_FIELD: if (!sim_dc_end_val.empty()) sim_dc_end_val.pop_back(); break;
                    case DC_INCREMENT_FIELD: if (!sim_dc_increment.empty()) sim_dc_increment.pop_back(); break;
                    case TRAN_TSTEP_FIELD: if (!sim_tran_tstep.empty()) sim_tran_tstep.pop_back(); break;
                    case TRAN_TSTOP_FIELD: if (!sim_tran_tstop.empty()) sim_tran_tstop.pop_back(); break;
                    case TRAN_TSTART_FIELD: if (!sim_tran_tstart.empty()) sim_tran_tstart.pop_back(); break;
                    case TRAN_TMAXSTEP_FIELD: if (!sim_tran_tmaxstep.empty()) sim_tran_tmaxstep.pop_back(); break;
                    case AC_SOURCE_FIELD: if (!sim_ac_source_name.empty()) sim_ac_source_name.pop_back(); break;
                    case AC_POINTS_FIELD: if (!sim_ac_points.empty()) sim_ac_points.pop_back(); break;
                    case AC_START_FREQ_FIELD: if (!sim_ac_start_freq.empty()) sim_ac_start_freq.pop_back(); break;
                    case AC_END_FREQ_FIELD: if (!sim_ac_end_freq.empty()) sim_ac_end_freq.pop_back(); break;
                    case VAR_INPUT_FIELD: if (!sim_var_input_text.empty()) sim_var_input_text.pop_back(); break;
                    default: break;
                }
            }
        } else if (e.key.keysym.sym == SDLK_RETURN) {
            if (current_state == INPUT_FILENAME_ADD) {
                if (simulator_ptr) { simulator_ptr->loadCircuitFromFile(current_input_text); }
            } else if (current_state == INPUT_FILENAME_NEW) {
                if (simulator_ptr) { simulator_ptr->createNewCircuit(current_input_text); }
            } else if (current_state == CIRCUIT_VIEW_IDLE && command_input_active) {
                executeCommand(command_input_text);
                command_input_text = "";
            }

        }

        else if (current_state == CIRCUIT_VIEW_IDLE) {
            if (e.key.keysym.sym == SDLK_r) { component_type_to_place = RESISTOR; setState(INPUT_COMPONENT_PARAMS); }
            else if (e.key.keysym.sym == SDLK_c) { component_type_to_place = CAPACITOR; setState(INPUT_COMPONENT_PARAMS); }
            else if (e.key.keysym.sym == SDLK_l) { component_type_to_place = INDUCTOR; setState(INPUT_COMPONENT_PARAMS); }
            else if (e.key.keysym.sym == SDLK_v) { component_type_to_place = VSOURCE_DC; setState(INPUT_COMPONENT_PARAMS); }
            else if (e.key.keysym.sym == SDLK_s) { component_type_to_place = VSOURCE_SIN; setState(INPUT_COMPONENT_PARAMS); }
            else if (e.key.keysym.sym == SDLK_i) { component_type_to_place = ISOURCE; setState(INPUT_COMPONENT_PARAMS); }
            else if (e.key.keysym.sym == SDLK_g) { executeCommand("add GND 0"); active_circuit->node_positions["0"] = {window_width / 2, window_height / 2}; autoLayoutNodes(); }
            else if (e.key.keysym.sym == SDLK_w) {
                first_wire_node_name = "";
                selected_node_name = "";
                selected_element_name = "";
                setState(CIRCUIT_VIEW_DRAWING_WIRE);
                console_output_buffer.push_back("Wire drawing mode: Click first node.");
            }
            else if (e.key.keysym.sym == SDLK_ESCAPE) {
                setState(CIRCUIT_VIEW_IDLE);
                component_type_to_place = NONE;
                first_wire_node_name = "";
                console_output_buffer.push_back("Mode cancelled.");
            }
            else if (e.key.keysym.sym == SDLK_d && !selected_element_name.empty()) {
                executeCommand("delete " + selected_element_name);
                selected_element_name = "";
            }
            else if (e.key.keysym.sym == SDLK_e && !selected_element_name.empty()) {

                setState(INPUT_COMPONENT_PARAMS);
                console_output_buffer.push_back("Editing component: " + selected_element_name);
            }
        } else if (current_state == CIRCUIT_VIEW_PLACING_COMPONENT || current_state == CIRCUIT_VIEW_DRAWING_WIRE) {
             if (e.key.keysym.sym == SDLK_ESCAPE) {
                setState(CIRCUIT_VIEW_IDLE);
                component_type_to_place = NONE;
                first_wire_node_name = "";
                console_output_buffer.push_back("Mode cancelled.");
            }
        }
    }
}













void CircuitRenderer::handleEvent(SDL_Event& e) {
    if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_RESIZED) {
        window_width = e.window.data1; window_height = e.window.data2; autoLayoutNodes();
    }
    if (e.type == SDL_MOUSEMOTION) { mouse_pos.x = e.motion.x; mouse_pos.y = e.motion.y; }
    handleTextInput(e);

    if (e.type == SDL_MOUSEBUTTONDOWN) {
        switch (current_state) {
            case MAIN_MENU: {
                int btn_w = 250, btn_h = 50, x = (window_width - btn_w) / 2;
                int start_y = window_height / 2 - (btn_h * 5 + 20 * 4) / 2;
                int y_off = btn_h + 20;
                if (checkButtonClick(e, x, start_y, btn_w, btn_h)) { setState(INPUT_FILENAME_ADD); text_input_prompt = "Enter filepath to load:"; }
                else if (checkButtonClick(e, x, start_y + y_off, btn_w, btn_h)) { setState(INPUT_FILENAME_NEW); text_input_prompt = "Enter new filepath:"; }
                else if (checkButtonClick(e, x, start_y + 2 * y_off, btn_w, btn_h)) { setState(FILE_SELECT); }
                else if (checkButtonClick(e, x, start_y + 4 * y_off, btn_w, btn_h)) { SDL_Event q_ev; q_ev.type = SDL_QUIT; SDL_PushEvent(&q_ev); }
                break;
            }
            case FILE_SELECT: {
                if (simulator_ptr) {
                    const auto& order = simulator_ptr->getSchematicOrder();
                    const auto& paths = simulator_ptr->getSchematicPaths();
                    for (size_t i = 0; i < order.size(); ++i) {
                        if (checkButtonClick(e, 50, 100 + i * 40, 400, 30)) { simulator_ptr->loadCircuitFromFile(paths.at(order[i])); break; }
                    }
                }
                if (checkButtonClick(e, 50, window_height - 60, 200, 40)) { setState(MAIN_MENU); }
                break;
            }
            case CIRCUIT_VIEW_IDLE:
            case CIRCUIT_VIEW_PLACING_COMPONENT:
            case CIRCUIT_VIEW_DRAWING_WIRE: {
                int btn_w = 100, btn_h = 30, x = 10, start_y = 50, y_off = 40;
                int sidebar_w = x + btn_w + 10;
                int cmd_y = window_height - 60;


                if (checkButtonClick(e, 10, 10, 180, 30)) {
                    if(simulator_ptr && active_circuit){
                        string name = simulator_ptr->getSchematicName(active_circuit->filepath);
                        simulator_ptr->saveCircuit(name);
                        console_output_buffer.push_back("Auto-saved " + name);
                    }
                    setState(MAIN_MENU);
                }
                else if (checkButtonClick(e, x, start_y, btn_w, btn_h)) { component_type_to_place = RESISTOR; setState(INPUT_COMPONENT_PARAMS); }
                else if (checkButtonClick(e, x, start_y + y_off, btn_w, btn_h)) { component_type_to_place = CAPACITOR; setState(INPUT_COMPONENT_PARAMS); }
                else if (checkButtonClick(e, x, start_y + 2*y_off, btn_w, btn_h)) { component_type_to_place = INDUCTOR; setState(INPUT_COMPONENT_PARAMS); }
                else if (checkButtonClick(e, x, start_y + 3*y_off, btn_w, btn_h)) { component_type_to_place = VSOURCE_DC; setState(INPUT_COMPONENT_PARAMS); }
                else if (checkButtonClick(e, x, start_y + 4*y_off, btn_w, btn_h)) { component_type_to_place = VSOURCE_SIN; setState(INPUT_COMPONENT_PARAMS); }
                else if (checkButtonClick(e, x, start_y + 5*y_off, btn_w, btn_h)) { component_type_to_place = ISOURCE; setState(INPUT_COMPONENT_PARAMS); }
                else if (checkButtonClick(e, x, start_y + 6*y_off, btn_w, btn_h)) { component_type_to_place = VSOURCE_AC; setState(INPUT_COMPONENT_PARAMS); }
                else if (checkButtonClick(e, x, start_y + 7*y_off, btn_w, btn_h)) { component_type_to_place = GROUND; setState(CIRCUIT_VIEW_PLACING_COMPONENT); }
                else if (checkButtonClick(e, x, start_y + 8*y_off, btn_w, btn_h)) { setState(SIMULATION_SETUP); }

                else if (checkButtonClick(e, x, start_y + 9*y_off, btn_w, btn_h)) {
                     if(simulator_ptr && active_circuit){
                        string name = simulator_ptr->getSchematicName(active_circuit->filepath);
                        simulator_ptr->saveCircuit(name);
                        console_output_buffer.push_back("Circuit '" + name + "' saved.");
                    }
                }
                else if (checkButtonClick(e, x, start_y + 10*y_off, btn_w, btn_h)) {
                    setState(CIRCUIT_VIEW_DRAWING_WIRE);
                    console_output_buffer.push_back("Wire mode: Click to start/add corners. ESC to cancel.");
                }
                else if (checkButtonClick(e, x, start_y + 11*y_off, btn_w, btn_h)) { if (!selected_element_name.empty()) { setState(INPUT_COMPONENT_PARAMS); } }
                else if (checkButtonClick(e, x, start_y + 12*y_off, btn_w, btn_h)) {
                    if (active_circuit) {
                        if (!selected_element_name.empty()) {
                            executeCommand("delete " + selected_element_name);
                            selected_element_name = "";
                        } else if (!selected_node_name.empty()) {
                            executeCommand("delete node " + selected_node_name);
                            selected_node_name = "";
                        }
                    }
                }
                else if (e.button.x > sidebar_w && e.button.y < cmd_y) {
                    if (current_state == CIRCUIT_VIEW_DRAWING_WIRE && active_circuit) {
                        string clicked_node_name;
                        for (const auto& pair : active_circuit->nodes) {
                            Node* n = pair.second;
                            int r = 10;
                            if (e.button.x > n->x - r && e.button.x < n->x + r && e.button.y > n->y - r && e.button.y < n->y + r) {
                                clicked_node_name = n->name;
                                break;
                            }
                        }

                        if (first_wire_node_name.empty()) {
                            if (!clicked_node_name.empty()) {
                                first_wire_node_name = clicked_node_name;
                            } else {
                                string new_node = active_circuit->generateUniqueNodeName();
                                active_circuit->getOrCreateNode(new_node);
                                active_circuit->node_positions[new_node] = {e.button.x, e.button.y};
                                first_wire_node_name = new_node;
                            }
                            selected_node_name = first_wire_node_name;
                        } else {
                            string dest_node_name;
                            bool is_finishing_on_existing_node = !clicked_node_name.empty();

                            if (is_finishing_on_existing_node) {
                                dest_node_name = clicked_node_name;
                            } else {
                                string new_corner_node = active_circuit->generateUniqueNodeName();
                                active_circuit->getOrCreateNode(new_corner_node);
                                active_circuit->node_positions[new_corner_node] = {e.button.x, e.button.y};
                                dest_node_name = new_corner_node;
                            }

                            string wire_name = active_circuit->generateUniqueComponentName('W');
                            active_circuit->addResistorInternal(wire_name, first_wire_node_name, dest_node_name, "1e-6");

                            if(is_finishing_on_existing_node){
                                setState(CIRCUIT_VIEW_IDLE);
                            } else {
                                first_wire_node_name = dest_node_name;
                                selected_node_name = dest_node_name;
                            }
                        }
                    }
                    else if (current_state == CIRCUIT_VIEW_PLACING_COMPONENT && active_circuit) {
                        if (component_type_to_place == GROUND) {
                            string gnd_name = active_circuit->generateUniqueNodeName();
                            active_circuit->getOrCreateNode(gnd_name)->x = e.button.x;
                            active_circuit->getOrCreateNode(gnd_name)->y = e.button.y;
                            active_circuit->node_positions[gnd_name] = {e.button.x, e.button.y};
                            executeCommand("add GND " + gnd_name);
                        } else {
                            string n1 = current_param_input_node1, n2 = current_param_input_node2;
                            active_circuit->getOrCreateNode(n1)->x = e.button.x; active_circuit->getOrCreateNode(n1)->y = e.button.y;
                            active_circuit->node_positions[n1] = {e.button.x, e.button.y};
                            active_circuit->getOrCreateNode(n2)->x = e.button.x + 80; active_circuit->getOrCreateNode(n2)->y = e.button.y;
                            active_circuit->node_positions[n2] = {e.button.x + 80, e.button.y};
                            string cmd = "add " + current_param_input_name + " " + n1 + " " + n2 + " ";
                            if (component_type_to_place == VSOURCE_SIN) cmd += "SIN(" + current_param_input_voff + " " + current_param_input_vamp + " " + current_param_input_freq + ")";
                            else cmd += current_param_input_value;
                            executeCommand(cmd);
                        }
                        setState(CIRCUIT_VIEW_IDLE);
                    } else if (current_state == CIRCUIT_VIEW_IDLE && active_circuit) {
                        selected_element_name = ""; selected_node_name = "";
                        for (const auto& pair_e : active_circuit->elements) {
                            if(pair_e.second->node2_name.empty()) continue;
                            Element* elem = pair_e.second;
                            if (elem->name[0] == 'W') continue;
                            pair<int,int> p1=getNodePos(elem->node1_name), p2=getNodePos(elem->node2_name);
                            int midX=(p1.first+p2.first)/2, midY=(p1.second+p2.second)/2, tol=20;
                            if(e.button.x>midX-tol&&e.button.x<midX+tol&&e.button.y>midY-tol&&e.button.y<midY+tol){ selected_element_name=elem->name; break; }
                        }
                        if (selected_element_name.empty()) { for (const auto& pair : active_circuit->nodes) { Node* n=pair.second; int r=10; if(e.button.x>n->x-r&&e.button.x<n->x+r&&e.button.y>n->y-r&&e.button.y<n->y+r){ selected_node_name=n->name; break; } } }
                    }
                }
                break;
            }
            case INPUT_FILENAME_ADD:
            case INPUT_FILENAME_NEW: {
                if (checkButtonClick(e, window_width/2 - 100, 180, 80, 40)) { if (current_state == INPUT_FILENAME_ADD && simulator_ptr) { simulator_ptr->loadCircuitFromFile(current_input_text); } else if (current_state == INPUT_FILENAME_NEW && simulator_ptr) { simulator_ptr->createNewCircuit(current_input_text); } }
                else if (checkButtonClick(e, window_width/2 + 20, 180, 80, 40)) { setState(MAIN_MENU); }
                break;
            }
            case INPUT_COMPONENT_PARAMS: {
                int modal_w = 400;
                int mx = window_width/2 - modal_w/2, my = window_height/2 - 150;
                int iw = 200, ih = 30, y_off = 40;
                int y_f1 = my + 40, y_f2 = y_f1 + y_off, y_f3 = y_f2 + y_off, y_f4 = y_f3 + y_off;

                if (checkButtonClick(e, mx + 150, y_f1, iw, ih)) current_input_focus = NAME_FIELD;
                else if (checkButtonClick(e, mx + 150, y_f2, iw, ih)) current_input_focus = NODE1_FIELD;
                else if (checkButtonClick(e, mx + 150, y_f3, iw, ih)) current_input_focus = NODE2_FIELD;
                else if (component_type_to_place == VSOURCE_SIN) {
                    if (checkButtonClick(e, mx + 150, y_f4, iw, ih)) current_input_focus = VOFF_FIELD;
                    else if (checkButtonClick(e, mx + 150, y_f4 + y_off, iw, ih)) current_input_focus = VAMP_FIELD;
                    else if (checkButtonClick(e, mx + 150, y_f4 + 2*y_off, iw, ih)) current_input_focus = FREQ_FIELD;
                } else { if (checkButtonClick(e, mx + 150, y_f4, iw, ih)) current_input_focus = VALUE_FIELD; }

                int ok_y = my + 250;
                if (checkButtonClick(e, mx + modal_w/2 - 90, ok_y, 80, 40)) {
                    if (selected_element_name.empty()) {
                        setState(CIRCUIT_VIEW_PLACING_COMPONENT);
                    } else {
                        if(active_circuit) active_circuit->deleteElementAndCleanNodes(selected_element_name);
                        string cmd = "add " + current_param_input_name + " " + current_param_input_node1 + " " + current_param_input_node2 + " ";
                        if (component_type_to_place == VSOURCE_SIN) cmd += "SIN(" + current_param_input_voff + " " + current_param_input_vamp + " " + current_param_input_freq + ")";
                        else cmd += current_param_input_value;
                        executeCommand(cmd);
                        setState(CIRCUIT_VIEW_IDLE);
                    }
                }
                else if (checkButtonClick(e, mx + modal_w/2 + 10, ok_y, 80, 40)) { setState(CIRCUIT_VIEW_IDLE); }
                break;
            }
             case SIMULATION_SETUP: {
                int modal_w = 600, modal_h = 500;
                int mx = window_width/2 - modal_w/2, my = window_height/2 - modal_h/2;
                int ix = mx + 160, iw = modal_w - 180, y_off = 40, y_sec = 50;
                int y_an = my + 20 + y_sec, y_p1 = y_an + y_sec + y_off, y_v_in = y_p1 + 4*y_off + y_sec + 10, y_v_btns = y_v_in + y_off;
                int btn_w = 120, btn_h = 40, b_btn_y = my + modal_h - btn_h - 20;

                if (checkButtonClick(e, ix, y_an, 80, 30)) { sim_analysis_type = "DC"; }
                else if (checkButtonClick(e, ix + 90, y_an, 80, 30)) { sim_analysis_type = "TRAN"; }
                else if (checkButtonClick(e, ix + 180, y_an, 80, 30)) { sim_analysis_type = "AC"; }
                else if (checkButtonClick(e, ix + iw - 100, y_v_in, 100, 30)) {
                    PlottableExpression new_expr;
                    if (active_circuit && active_circuit->parsePlottableExpression(sim_var_input_text, new_expr)) {
                        sim_plottable_expressions.push_back(new_expr);
                    } else {

                        console_output_buffer.push_back("Error: Invalid expression format.");
                    }
                    sim_var_input_text = "";
                }
                else if (checkButtonClick(e, ix + iw - 100, y_v_btns, 100, 30)) { sim_plottable_expressions.clear(); }
                else if (checkButtonClick(e, mx + modal_w/2 - btn_w - 10, b_btn_y, btn_w, btn_h)) { runAndPlotSimulation(); }
                else if (checkButtonClick(e, mx + modal_w/2 + 10, b_btn_y, btn_w, btn_h)) { setState(CIRCUIT_VIEW_IDLE); }
                else if (sim_analysis_type == "DC") {
                    if (checkButtonClick(e, ix, y_p1, iw, 30)) current_input_focus = DC_SOURCE_FIELD;
                    else if (checkButtonClick(e, ix, y_p1+y_off, iw, 30)) current_input_focus = DC_START_FIELD;
                    else if (checkButtonClick(e, ix, y_p1+2*y_off, iw, 30)) current_input_focus = DC_END_FIELD;
                    else if (checkButtonClick(e, ix, y_p1+3*y_off, iw, 30)) current_input_focus = DC_INCREMENT_FIELD;
                    else if (checkButtonClick(e, ix, y_v_in, iw - 110, 30)) current_input_focus = VAR_INPUT_FIELD; else current_input_focus = NO_FOCUS;
                } else if (sim_analysis_type == "TRAN") {
                    if (checkButtonClick(e, ix, y_p1, iw, 30)) current_input_focus = TRAN_TSTEP_FIELD;
                    else if (checkButtonClick(e, ix, y_p1+y_off, iw, 30)) current_input_focus = TRAN_TSTOP_FIELD;
                    else if (checkButtonClick(e, ix, y_p1+2*y_off, iw, 30)) current_input_focus = TRAN_TSTART_FIELD;
                    else if (checkButtonClick(e, ix, y_p1+3*y_off, iw, 30)) current_input_focus = TRAN_TMAXSTEP_FIELD;
                    else if (checkButtonClick(e, ix, y_v_in, iw - 110, 30)) current_input_focus = VAR_INPUT_FIELD; else current_input_focus = NO_FOCUS;
                } else if (sim_analysis_type == "AC") {
                    if (checkButtonClick(e, ix, y_p1, iw, 30)) current_input_focus = AC_POINTS_FIELD;
                    else if (checkButtonClick(e, ix, y_p1+y_off, iw, 30)) current_input_focus = AC_START_FREQ_FIELD;
                    else if (checkButtonClick(e, ix, y_p1+2*y_off, iw, 30)) current_input_focus = AC_END_FREQ_FIELD;
                    else if (checkButtonClick(e, ix, y_v_in, iw - 110, 30)) current_input_focus = VAR_INPUT_FIELD; else current_input_focus = NO_FOCUS;
                }
                break;
            }
            case PLOT_VIEW: {
                SDL_Rect plot_area = {100, 60, window_width - 200, window_height - 140};
                if (e.button.x >= plot_area.x && e.button.x <= plot_area.x + plot_area.w &&
                    e.button.y >= plot_area.y && e.button.y <= plot_area.y + plot_area.h)
                {
                    if (!plot_data.data.empty()) {
                        double x_range = plot_view_xmax - plot_view_xmin;
                        double clicked_x_data = plot_view_xmin + ((e.button.x - plot_area.x) / (double)plot_area.w) * x_range;

                        size_t best_index = 0;
                        double min_dist = numeric_limits<double>::max();

                        for (size_t i = 0; i < plot_data.data.size(); ++i) {
                            double dist = abs(plot_data.data[i][0] - clicked_x_data);
                            if (dist < min_dist) {
                                min_dist = dist;
                                best_index = i;
                            }
                        }
                        plot_cursor_data_index = best_index;
                        plot_cursor_active = true;
                    }
                } else {
                    plot_cursor_active = false;
                    if (checkButtonClick(e, 50, window_height - 45, 150, 35)) { setState(CIRCUIT_VIEW_IDLE); }
                    int btn_x = window_width - 80;
                    if (checkButtonClick(e, btn_x, 100, 30, 30)) { double r = plot_view_ymax-plot_view_ymin, c = plot_view_ymin+r/2.0, nr = r/2.0; plot_view_ymin=c-nr/2.0; plot_view_ymax=c+nr/2.0; }
                    else if (checkButtonClick(e, btn_x, 140, 30, 30)) { double r = plot_view_ymax-plot_view_ymin, c = plot_view_ymin+r/2.0, nr = r*2.0; plot_view_ymin=c-nr/2.0; plot_view_ymax=c+nr/2.0; }
                    else if (checkButtonClick(e, btn_x, 180, 30, 30)) { double r = plot_view_xmax-plot_view_xmin, c = plot_view_xmin+r/2.0, nr = r/2.0; plot_view_xmin=c-nr/2.0; plot_view_xmax=c+nr/2.0; }
                    else if (checkButtonClick(e, btn_x, 220, 30, 30)) { double r = plot_view_xmax-plot_view_xmin, c = plot_view_xmin+r/2.0, nr = r*2.0; plot_view_xmin=c-nr/2.0; plot_view_xmax=c+nr/2.0; }
                    else if (checkButtonClick(e, btn_x - 10, 260, 50, 30)) { resetPlotView(); }
                }
                break;
            }
        }
    }
}

void CircuitRenderer::setState(CircuitRenderer::RendererState newState) {
    if (current_state == PLOT_VIEW && newState != PLOT_VIEW) {
        plot_cursor_active = false;
    }


    current_state = newState;
    SDL_StopTextInput();
    text_input_active = false;
    command_input_active = false;
    current_input_focus = NO_FOCUS;

    string old_selected_element_name = selected_element_name;
    selected_element_name = "";
    selected_node_name = "";
    first_wire_node_name = "";

    if (newState != CIRCUIT_VIEW_PLACING_COMPONENT) {
        current_param_input_name = "";
        current_param_input_node1 = "";
        current_param_input_node2 = "";
        current_param_input_value = "";
        current_param_input_voff = "";
        current_param_input_vamp = "";
        current_param_input_freq = "";
        current_param_input_model = "";
        current_param_input_ctrl_node1 = "";
        current_param_input_ctrl_node2 = "";
        current_param_input_ctrl_vsource = "";
        current_param_input_gain = "";
    }

    sim_var_input_text = "";

    if (newState == INPUT_FILENAME_ADD || newState == INPUT_FILENAME_NEW) {
        SDL_StartTextInput();
        text_input_active = true;
        current_input_text = "";
    } else if (newState == CIRCUIT_VIEW_IDLE) {
        SDL_StartTextInput();
        command_input_active = true;
        command_input_text = "";
    } else if (newState == INPUT_COMPONENT_PARAMS) {
        SDL_StartTextInput();
        if (!old_selected_element_name.empty() && active_circuit && active_circuit->elements.count(old_selected_element_name)) {
            Element* elem_to_edit = active_circuit->elements.at(old_selected_element_name);
            current_param_input_name = elem_to_edit->name;
            current_param_input_node1 = elem_to_edit->node1_name;
            current_param_input_node2 = elem_to_edit->node2_name;

            if (elem_to_edit->type == "Resistor") {
                component_type_to_place = RESISTOR;
                current_param_input_value = to_string(static_cast<Resistor*>(elem_to_edit)->value);
            } else if (elem_to_edit->type == "Capacitor") {
                component_type_to_place = CAPACITOR;
                current_param_input_value = to_string(static_cast<Capacitor*>(elem_to_edit)->value);
            } else if (elem_to_edit->type == "Inductor") {
                component_type_to_place = INDUCTOR;
                current_param_input_value = to_string(static_cast<Inductor*>(elem_to_edit)->value);
            } else if (elem_to_edit->type == "IndependentVoltageSource") {
                IndependentVoltageSource* vsrc = static_cast<IndependentVoltageSource*>(elem_to_edit);
                if (vsrc->source_form == "DC") {
                    component_type_to_place = VSOURCE_DC;
                    current_param_input_value = to_string(vsrc->dc_value);
                } else {
                    component_type_to_place = VSOURCE_SIN;
                    current_param_input_voff = to_string(vsrc->v_offset);
                    current_param_input_vamp = to_string(vsrc->v_amplitude);
                    current_param_input_freq = to_string(vsrc->frequency);
                }
            } else if (elem_to_edit->type == "IndependentCurrentSource") {
                component_type_to_place = ISOURCE;
                current_param_input_value = to_string(static_cast<IndependentCurrentSource*>(elem_to_edit)->dc_value);
            }
            selected_element_name = old_selected_element_name;
        } else if (active_circuit) {
            char prefix = ' ';
            switch (component_type_to_place) {
                case RESISTOR: prefix = 'R'; break;
                case CAPACITOR: prefix = 'C'; break;
                case INDUCTOR: prefix = 'L'; break;
                case VSOURCE_DC: case VSOURCE_SIN: prefix = 'V'; break;
                case ISOURCE: prefix = 'I'; break;
                case DIODE: prefix = 'D'; break;
                case VCVS: prefix = 'E'; break;
                case VSOURCE_AC: prefix = 'U'; break;
                case VCCS: prefix = 'G'; break;
                case CCVS: prefix = 'H'; break;
                case CCCS: prefix = 'F'; break;
                default: break;
            }
            if (prefix != ' ') {
                current_param_input_name = active_circuit->generateUniqueComponentName(prefix);
            }
            current_param_input_node1 = active_circuit->generateUniqueNodeName();
            current_param_input_node2 = active_circuit->generateUniqueNodeName();
        }
        current_input_focus = VALUE_FIELD;
    } else if (newState == SIMULATION_SETUP) {
        SDL_StartTextInput();
        current_input_focus = DC_SOURCE_FIELD;
    }
}

void CircuitRenderer::drawText(const string& text, int x, int y, SDL_Color color) {
    if (text.empty()) {
        return;
    }
    if (!font) {
        cerr << "Error: Font not loaded for drawing text. Text: '" << text << "'" << endl;
        return;
    }

    SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
    if (!surface) {
        cerr << "Error: TTF_RenderText_Solid failed for text '" << text << "': " << TTF_GetError() << endl;
        return;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        cerr << "Error: SDL_CreateTextureFromSurface failed: " << string(SDL_GetError()) << endl;
        SDL_FreeSurface(surface);
        return;
    }
    SDL_Rect dstRect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &dstRect);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

void CircuitRenderer::drawButton(const string& text, int x, int y, int w, int h, SDL_Color bgColor, SDL_Color textColor) {
    SDL_Rect buttonRect = {x, y, w, h};
    SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
    SDL_RenderFillRect(renderer, &buttonRect);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &buttonRect);

    int text_w = 0, text_h = 0;
    if (font && !text.empty()) {
        TTF_SizeText(font, text.c_str(), &text_w, &text_h);
    }
    drawText(text, x + (w - text_w) / 2, y + (h - text_h) / 2, textColor);
}

bool CircuitRenderer::checkButtonClick(const SDL_Event& e, int x, int y, int w, int h) {
    if (e.type == SDL_MOUSEBUTTONDOWN) {
        if (e.button.x >= x && e.button.x <= x + w &&
            e.button.y >= y && e.button.y <= y + h) {
            return true;
        }
    }
    return false;
}

void CircuitRenderer::drawInputField(const string& label, int x, int y, int w, int h, string& value_ref, InputFieldFocus field_focus_id) {
    SDL_Rect input_rect = {x, y, w, h};
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, &input_rect);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &input_rect);

    drawText(label, x - 150, y + 10, {255, 255, 255, 255});

    string display_text = value_ref;
    if (current_input_focus == field_focus_id) {
        display_text += "|";
    }
    drawText(display_text, input_rect.x + 5, input_rect.y + 10, {255, 255, 255, 255});
}

void CircuitRenderer::autoLayoutNodes() {
    if (!active_circuit || active_circuit->nodes.empty()) return;

    int node_count = active_circuit->nodes.size();
    int nodes_per_row = static_cast<int>(ceil(sqrt(node_count)));
    if (nodes_per_row == 0) nodes_per_row = 1;

    int sidebar_width = 120;
    int console_height = 200;
    int top_margin = 40;

    int drawable_width = window_width - sidebar_width - 20;
    int drawable_height = window_height - console_height - top_margin - 10;

    int node_spacing_x = drawable_width / (nodes_per_row + 1);
    int num_rows = static_cast<int>(ceil(node_count / (double)nodes_per_row));
    if (num_rows == 0) num_rows = 1;
    int node_spacing_y = drawable_height / (num_rows + 1);
    if (node_spacing_y == 0) node_spacing_y = drawable_height / 2;

    int i = 0;
    for (auto& pair_item : active_circuit->nodes) {
        string node_name = pair_item.first;
        Node* node_ptr = pair_item.second;
        int row = i / nodes_per_row;
        int col = i % nodes_per_row;
        int x = (col + 1) * node_spacing_x + sidebar_width + 10;
        int y = (row + 1) * node_spacing_y + top_margin + 10;
        active_circuit->node_positions[node_name] = {x, y};
        node_ptr->x = x;
        node_ptr->y = y;
        i++;
    }
}

pair<int, int> CircuitRenderer::getNodePos(const string& node_name) {
    if (active_circuit && active_circuit->node_positions.count(node_name)) {
        return active_circuit->node_positions.at(node_name);
    }
    return {0, 0};
}

void CircuitRenderer::drawLine(int x1, int y1, int x2, int y2) {
    SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
}

void CircuitRenderer::drawRect(int x, int y, int w, int h) {
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderDrawRect(renderer, &rect);
}

void CircuitRenderer::fillRect(int x, int y, int w, int h) {
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderFillRect(renderer, &rect);
}

void CircuitRenderer::drawCircle(int centerX, int centerY, int radius) {
    for (int i = 0; i < 360; ++i) {
        double angle = i * M_PI / 180.0;
        int x = centerX + static_cast<int>(radius * cos(angle));
        int y = centerY + static_cast<int>(radius * sin(angle));
        SDL_RenderDrawPoint(renderer, x, y);
    }
}

void CircuitRenderer::drawResistor(int x1, int y1, int x2, int y2) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
    drawRect((x1 + x2)/2 - 10, (y1 + y2)/2 - 5, 20, 10);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    drawLine(x1, y1, (x1 + x2)/2 - 10, (y1 + y2)/2);
    drawLine((x1 + x2)/2 + 10, (y1 + y2)/2, x2, y2);
}

void CircuitRenderer::drawCapacitor(int x1, int y1, int x2, int y2) {
    SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
    drawLine(x1, y1, (x1 + x2) / 2 - 5, (y1 + y2) / 2);
    drawLine((x1 + x2) / 2 + 5, (y1 + y2) / 2, x2, y2);
    drawLine((x1 + x2) / 2 - 5, (y1 + y2) / 2 - 10, (x1 + x2) / 2 - 5, (y1 + y2) / 2 + 10);
    drawLine((x1 + x2) / 2 + 5, (y1 + y2) / 2 - 10, (x1 + x2) / 2 + 5, (y1 + y2) / 2 + 10);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
}

void CircuitRenderer::drawInductor(int x1, int y1, int x2, int y2) {
    SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
    int midX = (x1 + x2) / 2;
    int midY = (y1 + y2) / 2;
    drawLine(x1, y1, midX - 15, midY);
    drawLine(midX - 15, midY, midX - 5, midY - 10);
    drawLine(midX - 5, midY - 10, midX + 5, midY + 10);
    drawLine(midX + 5, midY + 10, midX + 15, midY - 10);
    drawLine(midX + 15, midY - 10, x2, y2);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
}

void CircuitRenderer::drawVoltageSource(int x1, int y1, int x2, int y2, bool is_sin) {
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    int centerX = (x1 + x2) / 2;
    int centerY = (y1 + y2) / 2;
    drawCircle(centerX, centerY, 15);
    if (is_sin) {
        for (int i = 0; i < 360; ++i) {
            double angle = i * M_PI / 180.0;
            double prev_angle = (i - 1) * M_PI / 180.0;
            if (i > 0) {
                drawLine(centerX + static_cast<int>(10 * sin(prev_angle * 2)), centerY + static_cast<int>(10 * cos(prev_angle * 2)),
                         centerX + static_cast<int>(10 * sin(angle * 2)), centerY + static_cast<int>(10 * cos(angle * 2)));
            }
        }
    } else {
        drawLine(centerX - 5, centerY, centerX + 5, centerY);
        drawLine(centerX, centerY - 5, centerX, centerY + 5);
    }
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    drawLine(x1, y1, centerX - 15, centerY);
    drawLine(centerX + 15, centerY, x2, y2);
}

void CircuitRenderer::drawCurrentSource(int x1, int y1, int x2, int y2) {
    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255);
    int centerX = (x1 + x2) / 2;
    int centerY = (y1 + y2) / 2;
    drawCircle(centerX, centerY, 15);
    drawLine(centerX, centerY - 10, centerX, centerY + 10);
    drawLine(centerX, centerY + 10, centerX - 5, centerY + 5);
    drawLine(centerX, centerY + 10, centerX + 5, centerY + 5);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    drawLine(x1, y1, centerX, centerY - 15);
    drawLine(centerX, centerY + 15, x2, y2);
}

void CircuitRenderer::drawGround(int x, int y) {
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    drawLine(x, y, x, y + 15);
    drawLine(x - 10, y + 15, x + 10, y + 15);
    drawLine(x - 7, y + 20, x + 7, y + 20);
    drawLine(x - 4, y + 25, x + 4, y + 25);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
}

void CircuitRenderer::drawMainMenu() {
    int button_width = 250;
    int button_height = 50;
    int start_y = window_height / 2 - (button_height * 5 + 20 * 4) / 2;
    int x = (window_width - button_width) / 2;
    int y_offset = button_height + 20;

    drawText("===== Circuit Simulator =====", x - 50, start_y - 60, {255, 255, 255, 255});

    drawButton("1. Add/Load Schematic", x, start_y, button_width, button_height, {50, 50, 150, 255}, {255, 255, 255, 255});
    drawButton("2. Create New Schematic", x, start_y + y_offset, button_width, button_height, {50, 50, 150, 255}, {255, 255, 255, 255});
    drawButton("3. Show Existing Schematics", x, start_y + 2 * y_offset, button_width, button_height, {50, 50, 150, 255}, {255, 255, 255, 255});
    drawButton("4. Help", x, start_y + 3 * y_offset, button_width, button_height, {50, 50, 150, 255}, {255, 255, 255, 255});
    drawButton("5. Exit", x, start_y + 4 * y_offset, button_width, button_height, {150, 50, 50, 255}, {255, 255, 255, 255});
}

void CircuitRenderer::drawFileSelectionMenu(const vector<string>& schematic_order, const map<string, string>& schematic_paths) {
    drawText("--- Choose Existing Schematic ---", 50, 50, {255, 255, 255, 255});

    int start_y = 100;
    int button_width = 400;
    int button_height = 30;
    int x = 50;

    if (schematic_order.empty()) {
        drawText("No schematics loaded. Use 'Add/Load Schematic' from main menu.", x, start_y, {255, 255, 255, 255});
    } else {
        for (size_t i = 0; i < schematic_order.size(); ++i) {
            string button_text = to_string(i + 1) + ". " + schematic_order[i] + " (" + schematic_paths.at(schematic_order[i]) + ")";
            drawButton(button_text, x, start_y + i * (button_height + 10), button_width, button_height, {80, 80, 80, 255}, {255, 255, 255, 255});
        }
    }

    drawButton("Return to Main Menu", 50, window_height - 60, 200, 40, {100, 100, 100, 255}, {255, 255, 255, 255});
}




void CircuitRenderer::drawCircuitView() {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    if(active_circuit) {

        for (const auto& element_pair : active_circuit->elements) {
            Element* elem = element_pair.second;
            if (elem->node2_name.empty()) {
                continue;
            }

            pair<int, int> pos1 = getNodePos(elem->node1_name);
            pair<int, int> pos2 = getNodePos(elem->node2_name);

            if (elem->name[0] == 'W') {
                SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
                drawLine(pos1.first, pos1.second, pos2.first, pos2.second);
                continue;
            }

            if (elem->type == "Resistor") drawResistor(pos1.first, pos1.second, pos2.first, pos2.second);
            else if (elem->type == "Capacitor") drawCapacitor(pos1.first, pos1.second, pos2.first, pos2.second);
            else if (elem->type == "Inductor") drawInductor(pos1.first, pos1.second, pos2.first, pos2.second);
            else if (elem->type == "IndependentVoltageSource") {
                auto* vsrc = static_cast<IndependentVoltageSource*>(elem);
                drawVoltageSource(pos1.first, pos1.second, pos2.first, pos2.second, vsrc->source_form == "SIN");
            }
            else if (elem->type == "ACVoltageSource") {
                drawVoltageSource(pos1.first, pos1.second, pos2.first, pos2.second, true);
            }
            else if (elem->type == "IndependentCurrentSource") drawCurrentSource(pos1.first, pos1.second, pos2.first, pos2.second);

            SDL_Color elem_name_color = {255, 255, 0, 255};
            if (elem->name == selected_element_name) elem_name_color = {255, 0, 255, 255};
            int midX = (pos1.first + pos2.first) / 2;
            int midY = (pos1.second + pos2.second) / 2;
            drawText(elem->name, midX + 5, midY + 5, elem_name_color);
        }


        for (const auto& node_pair : active_circuit->nodes) {
            Node* node = node_pair.second;
            pair<int, int> pos = getNodePos(node->name);

            SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
            if (node->name == selected_node_name || node->name == first_wire_node_name) { SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255); }
            drawCircle(pos.first, pos.second, 5);

            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            drawText(node->name, pos.first + 10, pos.second - 10, {255, 255, 255, 255});

            if (active_circuit->grounded_node_names.count(node->name)) { drawGround(pos.first, pos.second); }
        }
    }

    if (current_state == CIRCUIT_VIEW_DRAWING_WIRE && !first_wire_node_name.empty()) {
        pair<int, int> start_pos = getNodePos(first_wire_node_name);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
        drawLine(start_pos.first, start_pos.second, mouse_pos.x, mouse_pos.y);
    }

    if (current_state == CIRCUIT_VIEW_PLACING_COMPONENT && component_type_to_place != NONE) {
        SDL_SetRenderDrawColor(renderer, 100, 100, 255, 150);
        int gx1 = mouse_pos.x, gy1 = mouse_pos.y;
        int gx2 = mouse_pos.x + 80, gy2 = mouse_pos.y;

        switch(component_type_to_place) {
            case GROUND: drawGround(gx1, gy1); break;
            case RESISTOR: drawResistor(gx1, gy1, gx2, gy2); break;
            case CAPACITOR: drawCapacitor(gx1, gy1, gx2, gy2); break;
            case INDUCTOR: drawInductor(gx1, gy1, gx2, gy2); break;
            case VSOURCE_DC: drawVoltageSource(gx1, gy1, gx2, gy2, false); break;
            case VSOURCE_SIN: drawVoltageSource(gx1, gy1, gx2, gy2, true); break;
            case VSOURCE_AC: drawVoltageSource(gx1, gy1, gx2, gy2, true); break;
            case ISOURCE: drawCurrentSource(gx1, gy1, gx2, gy2); break;
            default: break;
        }
    }

    int comp_button_w = 130; int comp_button_h = 30;
    int comp_start_x = 10; int comp_start_y = 50;
    int sidebar_width = comp_start_x + comp_button_w + 10;

    int y_offset = 40;
    drawButton("Resistor (R)", comp_start_x, comp_start_y, comp_button_w, comp_button_h, {80, 80, 80, 255}, {255, 255, 255, 255});
    drawButton("Capacitor (C)", comp_start_x, comp_start_y + 40, comp_button_w, comp_button_h, {80, 80, 80, 255}, {255, 255, 255, 255});
    drawButton("Inductor (L)", comp_start_x, comp_start_y + 80, comp_button_w, comp_button_h, {80, 80, 80, 255}, {255, 255, 255, 255});
    drawButton("Vsrc DC (V)", comp_start_x, comp_start_y + 120, comp_button_w, comp_button_h, {80, 80, 80, 255}, {255, 255, 255, 255});
    drawButton("Vsrc SIN (S)", comp_start_x, comp_start_y + 160, comp_button_w, comp_button_h, {80, 80, 80, 255}, {255, 255, 255, 255});
    drawButton("I Source (I)", comp_start_x, comp_start_y + 200, comp_button_w, comp_button_h, {80, 80, 80, 255}, {255, 255, 255, 255});
    drawButton("AC Source (U)", comp_start_x, comp_start_y +  6*y_offset, comp_button_w, comp_button_h, {80, 80, 80, 255}, {255, 255, 255, 255});
    drawButton("Add GND 0 (G)", comp_start_x, comp_start_y +  7*y_offset, comp_button_w, comp_button_h, {80, 80, 80, 255}, {255, 255, 255, 255});
    drawButton("Simulate", comp_start_x, comp_start_y +  8*y_offset, comp_button_w, comp_button_h, {50, 150, 50, 255}, {255, 255, 255, 255});


    drawButton("Save Circuit", comp_start_x, comp_start_y +  9*y_offset, comp_button_w, comp_button_h, {50, 100, 150, 255}, {255, 255, 255, 255});

    drawButton("Wire (W)", comp_start_x, comp_start_y +  10*y_offset, comp_button_w, comp_button_h, {80, 80, 80, 255}, {255, 255, 255, 255});
    drawButton("Edit Selected", comp_start_x, comp_start_y +  11*y_offset, comp_button_w, comp_button_h, {150, 150, 50, 255}, {255, 255, 255, 255});
    drawButton("Delete Selected", comp_start_x, comp_start_y +  12*y_offset, comp_button_w, comp_button_h, {150, 50, 50, 255}, {255, 255, 255, 255});

    int console_start_y = window_height - 200;
    int console_height = 120;
    SDL_Rect console_rect = {sidebar_width, console_start_y, window_width - sidebar_width - 10, console_height};
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderFillRect(renderer, &console_rect);
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderDrawRect(renderer, &console_rect);
    int text_y = console_start_y + 5;
    for (const string& line : console_output_buffer) {
        drawText(line, console_rect.x + 5, text_y, {255, 255, 255, 255});
        text_y += 20;
        if (text_y > console_start_y + console_height - 20) break;
    }

    int input_box_y = window_height - 60;
    int execute_button_w = 90;
    SDL_Rect command_input_rect = {sidebar_width, input_box_y, window_width - sidebar_width - execute_button_w - 20, 40};
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, &command_input_rect);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &command_input_rect);
    drawText(command_input_text + (command_input_active ? "|" : ""), command_input_rect.x + 5, command_input_rect.y + 10, {255, 255, 255, 255});
    if (active_circuit) {
        drawButton("Execute", command_input_rect.x + command_input_rect.w + 10, input_box_y, execute_button_w, 40, {0, 100, 0, 255}, {255, 255, 255, 255});
    }
}
void CircuitRenderer::drawTextInputBox() {
    drawText(text_input_prompt, 50, 50, {255, 255, 255, 255});

    SDL_Rect input_rect = {50, 100, window_width - 100, 40};
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, &input_rect);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &input_rect);

    drawText(current_input_text + (text_input_active ? "|" : ""), input_rect.x + 5, input_rect.y + 10, {255, 255, 255, 255});

    drawButton("OK", window_width / 2 - 100, 180, 80, 40, {0, 150, 0, 255}, {255, 255, 255, 255});
    drawButton("Cancel", window_width / 2 + 20, 180, 80, 40, {150, 0, 0, 255}, {255, 255, 255, 255});
}





void CircuitRenderer::drawComponentParamsInputModal() {

    int num_fields = 3;
    if (component_type_to_place == VSOURCE_SIN) {
        num_fields += 3;
    } else if (component_type_to_place == DIODE) {
        num_fields += 1;
    } else if (component_type_to_place != NONE && component_type_to_place != GROUND) {
        num_fields += 1;
    }


    const int top_margin = 40;
    const int bottom_margin = 60;
    const int line_height = 40;
    const int modal_width = 400;
    int modal_height = top_margin + (num_fields * line_height) + bottom_margin;

    int modal_x = window_width / 2 - modal_width / 2;
    int modal_y = window_height / 2 - modal_height / 2;


    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    fillRect(0, 0, window_width, window_height);


    SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
    fillRect(modal_x, modal_y, modal_width, modal_height);
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    drawRect(modal_x, modal_y, modal_width, modal_height);

    string title_text = "Component Parameters";
    switch (component_type_to_place) {
        case RESISTOR: title_text = "Add Resistor"; break;
        case CAPACITOR: title_text = "Add Capacitor"; break;
        case INDUCTOR: title_text = "Add Inductor"; break;
        case VSOURCE_DC: title_text = "Add DC Voltage Source"; break;
        case VSOURCE_SIN: title_text = "Add SIN Voltage Source"; break;
        case ISOURCE: title_text = "Add Current Source"; break;
        case DIODE: title_text = "Add Diode"; break;
    }
    if (!selected_element_name.empty()) {
        title_text = "Edit " + selected_element_name;
    }
    drawText(title_text, modal_x + 20, modal_y + 10, {255, 255, 255, 255});

    int input_x = modal_x + 150;
    int input_w = 200;
    int input_h = 30;
    int current_y = modal_y + top_margin;

    drawInputField("Name:", input_x, current_y, input_w, input_h, current_param_input_name, NAME_FIELD);
    current_y += line_height;
    drawInputField("Node 1:", input_x, current_y, input_w, input_h, current_param_input_node1, NODE1_FIELD);
    current_y += line_height;
    drawInputField("Node 2:", input_x, current_y, input_w, input_h, current_param_input_node2, NODE2_FIELD);
    current_y += line_height;

    if (component_type_to_place == RESISTOR || component_type_to_place == CAPACITOR || component_type_to_place == INDUCTOR || component_type_to_place == VSOURCE_DC || component_type_to_place == ISOURCE) {
        drawInputField("Value:", input_x, current_y, input_w, input_h, current_param_input_value, VALUE_FIELD);
    }else if (component_type_to_place == VSOURCE_AC) {
        drawInputField("Magnitude:", input_x, current_y, input_w, input_h, current_param_input_value, VALUE_FIELD);
    }else if (component_type_to_place == VSOURCE_SIN) {
        drawInputField("Voffset:", input_x, current_y, input_w, input_h, current_param_input_voff, VOFF_FIELD);
        current_y += line_height;
        drawInputField("Vamp:", input_x, current_y, input_w, input_h, current_param_input_vamp, VAMP_FIELD);
        current_y += line_height;
        drawInputField("Freq:", input_x, current_y, input_w, input_h, current_param_input_freq, FREQ_FIELD);
    } else if (component_type_to_place == DIODE) {
        drawInputField("Model:", input_x, current_y, input_w, input_h, current_param_input_model, MODEL_FIELD);
    }

    drawButton("OK", modal_x + modal_width / 2 - 90, modal_y + modal_height - 50, 80, 40, {0, 150, 0, 255}, {255, 255, 255, 255});
    drawButton("Cancel", modal_x + modal_width / 2 + 10, modal_y + modal_height - 50, 80, 40, {150, 0, 0, 255}, {255, 255, 255, 255});
}


void CircuitRenderer::drawSimulationSetup() {

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_RenderFillRect(renderer, NULL);


    int modal_width = 600;
    int modal_height = 500;
    int modal_x = window_width / 2 - modal_width / 2;
    int modal_y = window_height / 2 - modal_height / 2;


    SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
    fillRect(modal_x, modal_y, modal_width, modal_height);
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    drawRect(modal_x, modal_y, modal_width, modal_height);


    int current_y = modal_y + 20;
    const int y_offset_section = 50;
    const int y_offset_line = 40;
    const int label_x = modal_x + 20;
    const int input_x = modal_x + 160;
    const int input_w_main = modal_width - 180;


    drawText("Simulation Setup", label_x, current_y, {255, 255, 255, 255});
    current_y += y_offset_section;


    drawText("Analysis Type:", label_x, current_y + 5, {255, 255, 255, 255});
    SDL_Color dc_color = (sim_analysis_type == "DC") ? SDL_Color{0, 200, 0, 255} : SDL_Color{100, 100, 100, 255};
    SDL_Color tran_color = (sim_analysis_type == "TRAN") ? SDL_Color{0, 200, 0, 255} : SDL_Color{100, 100, 100, 255};
    SDL_Color ac_color = (sim_analysis_type == "AC") ? SDL_Color{0, 200, 0, 255} : SDL_Color{100, 100, 100, 255};

    drawButton("DC", input_x, current_y, 80, 30, dc_color, {255, 255, 255, 255});
    drawButton("TRAN", input_x + 90, current_y, 80, 30, tran_color, {255, 255, 255, 255});
    drawButton("AC", input_x + 180, current_y, 80, 30, ac_color, {255, 255, 255, 255});
    current_y += y_offset_section;


    if (sim_analysis_type == "DC") {
        drawText("DC Parameters:", label_x, current_y, {255, 255, 255, 255});
        current_y += y_offset_line;
        drawInputField("Source Name:", input_x, current_y, input_w_main, 30, sim_dc_source_name, DC_SOURCE_FIELD);
        current_y += y_offset_line;
        drawInputField("Start Val:", input_x, current_y, input_w_main, 30, sim_dc_start_val, DC_START_FIELD);
        current_y += y_offset_line;
        drawInputField("End Val:", input_x, current_y, input_w_main, 30, sim_dc_end_val, DC_END_FIELD);
        current_y += y_offset_line;
        drawInputField("Increment:", input_x, current_y, input_w_main, 30, sim_dc_increment, DC_INCREMENT_FIELD);
    } else if (sim_analysis_type == "TRAN") {
        drawText("TRAN Parameters:", label_x, current_y, {255, 255, 255, 255});
        current_y += y_offset_line;
        drawInputField("Tstep:", input_x, current_y, input_w_main, 30, sim_tran_tstep, TRAN_TSTEP_FIELD);
        current_y += y_offset_line;
        drawInputField("Tstop:", input_x, current_y, input_w_main, 30, sim_tran_tstop, TRAN_TSTOP_FIELD);
        current_y += y_offset_line;
        drawInputField("Tstart (opt.):", input_x, current_y, input_w_main, 30, sim_tran_tstart, TRAN_TSTART_FIELD);
        current_y += y_offset_line;
        drawInputField("Tmax (opt.):", input_x, current_y, input_w_main, 30, sim_tran_tmaxstep, TRAN_TMAXSTEP_FIELD);
    }
    else if (sim_analysis_type == "AC") {
        drawText("AC Sweep Parameters:", label_x, current_y, {255, 255, 255, 255});
        current_y += y_offset_line;
        drawInputField("Points/Decade:", input_x, current_y, input_w_main, 30, sim_ac_points, AC_POINTS_FIELD);
        current_y += y_offset_line;
        drawInputField("Start Freq (Hz):", input_x, current_y, input_w_main, 30, sim_ac_start_freq, AC_START_FREQ_FIELD);
        current_y += y_offset_line;
        drawInputField("End Freq (Hz):", input_x, current_y, input_w_main, 30, sim_ac_end_freq, AC_END_FREQ_FIELD);
        current_y += y_offset_line;
    }
    current_y += y_offset_section + 10;


    drawText("Output Variables:", label_x, current_y, {255, 255, 255, 255});
    current_y += y_offset_line;
    drawInputField("Add Var (V(N)/I(E)):", input_x, current_y, input_w_main - 110, 30, sim_var_input_text, VAR_INPUT_FIELD);
    drawButton("Add", input_x + input_w_main - 100, current_y, 100, 30, {0, 100, 0, 255}, {255, 255, 255, 255});
    current_y += y_offset_line;

    drawText("Selected:", label_x, current_y + 5, {255, 255, 255, 255});
    string selected_vars_str;
    for (const auto& expr : sim_plottable_expressions) {
        selected_vars_str += expr.original_expression + " ";
    }

    if (selected_vars_str.length() > 50) {
        selected_vars_str = selected_vars_str.substr(0, 47) + "...";
    }
    drawText(selected_vars_str,  modal_x + 100, current_y + 5, {200, 220, 255, 255});
    drawButton("Clear All", input_x + input_w_main - 100, current_y, 100, 30, {150, 50, 50, 255}, {255, 255, 255, 255});


    int sim_button_w = 120;
    int sim_button_h = 40;
    int bottom_button_y = modal_y + modal_height - sim_button_h - 20;
    drawButton("Run Simulation", modal_x + modal_width / 2 - sim_button_w - 10, bottom_button_y, sim_button_w, sim_button_h, {50, 50, 150, 255}, {255, 255, 255, 255});
    drawButton("Back to Circuit", modal_x + modal_width / 2 + 10, bottom_button_y, sim_button_w, sim_button_h, {100, 100, 100, 255}, {255, 255, 255, 255});
}


CircuitRenderer* g_renderer = nullptr;
Simulator* g_simulator = nullptr;

void main_loop() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
        }
        if (g_renderer) {
            g_renderer->handleEvent(event);
        }
    }
    if (g_renderer) {
        g_renderer->render();
    }
}

int main(int argc, char* argv[]) {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    g_renderer = new CircuitRenderer();
    g_simulator = new Simulator(g_renderer);
    g_renderer->setSimulator(g_simulator);

    cout << "Welcome to the Circuit Simulator! (Console output for debugging only)" << endl;

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop, 0, 1);
#else
    SDL_Event event;
    bool quit = false;
    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            }
            if (g_renderer) {
                g_renderer->handleEvent(event);
            }
        }
        if (g_renderer) {
            g_renderer->render();
        }
    }
#endif

    delete g_simulator;
    delete g_renderer;

    return 0;
}