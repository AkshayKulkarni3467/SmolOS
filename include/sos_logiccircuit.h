#ifndef INCLUDE_SMOLOS_LOGICCIRCUIT_H
#define INCLUDE_SMOLOS_LOGICCIRCUIT_H

#include "sos_stdint.h"

#define GRID_WIDTH 70
#define GRID_HEIGHT 20
#define GRID_OFFSET_X 5
#define GRID_OFFSET_Y 3

#define MAX_GATES 50
#define MAX_WIRES 100
#define MAX_INPUTS 8
#define MAX_OUTPUTS 8
#define MAX_LEVELS 20

typedef enum {
    GATE_NONE = 0,
    GATE_AND,
    GATE_OR,
    GATE_XOR,
    GATE_NOT,
    GATE_NAND,
    GATE_NOR,
    GATE_XNOR,
    GATE_BUFFER,
    GATE_INPUT,
    GATE_OUTPUT,
    GATE_SWITCH,
    GATE_LED
} GateType;

typedef enum {
    PIN_NONE = 0,
    PIN_INPUT1,
    PIN_INPUT2,
    PIN_OUTPUT
} PinType;

typedef struct {
    int gate_id;
    PinType pin;
    int pin_index;  
} ConnectionPoint;

typedef struct {
    ConnectionPoint from;
    ConnectionPoint to;
    int active;
    int signal;
    uint8_t color;
} Wire;

typedef struct {
    GateType type;
    int x, y;
    int active;
    int inputs[2]; 
    int output;
    int id;
    uint8_t color;
    char label[8];
    int selected;
} Gate;

typedef struct {
    uint8_t inputs[MAX_INPUTS];
    uint8_t outputs[MAX_OUTPUTS];
    int input_count;
    int output_count;
} LevelObjective;

typedef struct {
    char name[32];
    char description[128];
    int difficulty;
    int max_gates;
    int allowed_gate_types;
    LevelObjective objective;
    int tutorial;
    char hint[128];
} LCLevel;

typedef enum {
    CIRCUIT_MENU = 0,
    CIRCUIT_LEVEL_SELECT,
    CIRCUIT_PLAYING,
    CIRCUIT_SANDBOX,
    CIRCUIT_LEVEL_COMPLETE,
    CIRCUIT_HELP,
    CIRCUIT_EXIT
} CircuitState;

typedef enum {
    TOOL_SELECT = 0,
    TOOL_WIRE,
    TOOL_DELETE,
    TOOL_PLACE_GATE,
    TOOL_MOVE
} Tool;

typedef enum {
    CONNECTION_NONE,
    CONNECTION_SELECTING_SOURCE,
    CONNECTION_SELECTING_TARGET,
    CONNECTION_SELECTING_PIN
} ConnectionState;

typedef struct {
    CircuitState state;

    Gate gates[MAX_GATES];
    Wire wires[MAX_WIRES];
    int gate_count;
    int wire_count;
    int cycles;
    
    LCLevel current_level;
    int current_level_index;
    int levels_completed[MAX_LEVELS];

    Tool current_tool;
    GateType selected_gate_type;
    int cursor_x, cursor_y;
    int menu_selection;

    ConnectionState connection_state;
    ConnectionPoint connection_source;
    ConnectionPoint connection_target;

    uint32_t last_input_time;
    int input_repeat_delay;
    int fast_repeat_mode;

    int total_gates_placed;
    int total_wires_placed;
    int levels_solved;

    int selected_gate_id;
    int selected_pin;
    int turbo_mode;
    
} LCGameSession;

void logiccircuit_game_init(void);
void logiccircuit_game_run(void);
void logiccircuit_game_cleanup(void);

void logiccircuit_init_levels(void);
void logiccircuit_load_level(LCGameSession* session, int level_index);
int logiccircuit_check_level_complete(LCGameSession* session);
void logiccircuit_unlock_next_level(LCGameSession* session);

void circuit_init(LCGameSession* session);
void circuit_simulate_step(LCGameSession* session);
void circuit_simulate_full(LCGameSession* session);
void circuit_clear(LCGameSession* session);
int circuit_add_gate(LCGameSession* session, GateType type, int x, int y);
int circuit_add_wire(LCGameSession* session, ConnectionPoint from, ConnectionPoint to);
void circuit_remove_gate(LCGameSession* session, int gate_id);
Gate* circuit_get_gate_at(LCGameSession* session, int x, int y);
Gate* circuit_get_gate(LCGameSession* session, int gate_id);
int gate_get_input_count(GateType type);
int gate_get_pin_count(GateType type);

int gate_evaluate(GateType type, int input1, int input2);
const char* gate_get_name(GateType type);
char gate_get_symbol(GateType type);
uint8_t gate_get_color(GateType type);

void logiccircuit_handle_input(LCGameSession* session);
void logiccircuit_handle_menu_input(LCGameSession* session, char c);
void logiccircuit_handle_game_input(LCGameSession* session, char c);
void logiccircuit_handle_connection_mode(LCGameSession* session, char c);
void logiccircuit_update_input_timing(LCGameSession* session);

void logiccircuit_draw_menu(LCGameSession* session);
void logiccircuit_draw_level_select(LCGameSession* session);
void logiccircuit_draw_game(LCGameSession* session);
void logiccircuit_draw_grid(void);
void logiccircuit_draw_gates(LCGameSession* session);
void logiccircuit_draw_wires(LCGameSession* session);
void logiccircuit_draw_toolbox(LCGameSession* session);
void logiccircuit_draw_status(LCGameSession* session);
void logiccircuit_draw_help(void);
void logiccircuit_draw_level_complete(LCGameSession* session);
void logiccircuit_draw_gate(LCGameSession* session, Gate* gate);
void logiccircuit_draw_cursor(LCGameSession* session);
void logiccircuit_draw_connection_info(LCGameSession* session);
void logiccircuit_draw_pin_indicators(Gate* gate, int show_pins);

void logiccircuit_grid_to_screen(int grid_x, int grid_y, int* screen_x, int* screen_y);
void logiccircuit_move_cursor(LCGameSession* session, int dx, int dy);
int logiccircuit_is_valid_placement(LCGameSession* session, int x, int y);
void logiccircuit_select_gate_at_cursor(LCGameSession* session);
void logiccircuit_deselect_all_gates(LCGameSession* session);
int logiccircuit_get_gate_pin_at_cursor(LCGameSession* session, Gate* gate, PinType* pin_type);

void logiccircuit_save_progress(LCGameSession* session);
void logiccircuit_load_progress(LCGameSession* session);
void run_logiccircuit_game(void);

#endif // INCLUDE_SMOLOS_LOGICCIRCUIT_H