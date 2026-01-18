#include "sos_logiccircuit.h"
#include "sos_vga.h"
#include "sos_vgraphics.h"
#include "sos_keyboard.h"
#include "sos_pit.h"
#include "sos_memory.h"
#include "sos_string.h"
#include "sos_fat16.h"

#define GAME_LOGIC_SAVE_FILE "LOGICSAVE.DAT"

static LCGameSession session;
static LCLevel levels[MAX_LEVELS];
static int level_count = 0;

static void print_centered(const char* text, int row) {
    int len = 0;
    while (text[len]) len++;
    int start = (80 - len) / 2;
    if (start < 0) start = 0;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(start + i, row, text[i]);
    }
}

static void fill_rect(int x, int y, int width, int height, char c, uint8_t fg, uint8_t bg) {
    uint8_t old_color = vga_t_color;
    vga_set_color(fg, bg);
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            vga_putchr_at(x + col, y + row, c);
        }
    }
    vga_t_color = old_color;
}

static void draw_box(int x, int y, int width, int height, uint8_t fg, uint8_t bg) {
    if (x < 0 || y < 0 || x + width > 80 || y + height > 25) return;
    
    uint8_t old_color = vga_t_color;
    vga_set_color(fg, bg);
    
    for (int i = 1; i < width - 1; i++) {
        vga_putchr_at(x + i, y, 0xC4);
        vga_putchr_at(x + i, y + height - 1, 0xC4);
    }
    
    for (int i = 1; i < height - 1; i++) {
        vga_putchr_at(x, y + i, 0xB3);
        vga_putchr_at(x + width - 1, y + i, 0xB3);
    }
    
    vga_putchr_at(x, y, 0xDA);
    vga_putchr_at(x + width - 1, y, 0xBF);
    vga_putchr_at(x, y + height - 1, 0xC0);
    vga_putchr_at(x + width - 1, y + height - 1, 0xD9);
    
    vga_t_color = old_color;
}


int gate_evaluate(GateType type, int input1, int input2) {
    switch (type) {
        case GATE_AND:    return input1 && input2;
        case GATE_OR:     return input1 || input2;
        case GATE_XOR:    return input1 ^ input2;
        case GATE_NOT:    return !input1;
        case GATE_NAND:   return !(input1 && input2);
        case GATE_NOR:    return !(input1 || input2);
        case GATE_XNOR:   return !(input1 ^ input2);
        case GATE_BUFFER: return input1;
        default:          return 0;
    }
}

const char* gate_get_name(GateType type) {
    switch (type) {
        case GATE_AND:    return "AND";
        case GATE_OR:     return "OR";
        case GATE_XOR:    return "XOR";
        case GATE_NOT:    return "NOT";
        case GATE_NAND:   return "NAND";
        case GATE_NOR:    return "NOR";
        case GATE_XNOR:   return "XNOR";
        case GATE_BUFFER: return "BUF";
        case GATE_INPUT:  return "IN";
        case GATE_OUTPUT: return "OUT";
        case GATE_SWITCH: return "SW";
        case GATE_LED:    return "LED";
        default:          return "???";
    }
}

char gate_get_symbol(GateType type) {
    switch (type) {
        case GATE_AND:    return '&';
        case GATE_OR:     return '|';
        case GATE_XOR:    return '^';
        case GATE_NOT:    return '!';
        case GATE_NAND:   return 'D';
        case GATE_NOR:    return 'R';
        case GATE_XNOR:   return 'X';
        case GATE_BUFFER: return '>';
        case GATE_INPUT:  return 'I';
        case GATE_OUTPUT: return 'O';
        case GATE_SWITCH: return 'S';
        case GATE_LED:    return 'L';
        default:          return '?';
    }
}

uint8_t gate_get_color(GateType type) {
    switch (type) {
        case GATE_AND:    return VGA_LGREEN;
        case GATE_OR:     return VGA_YELLOW;
        case GATE_XOR:    return VGA_LMAGENTA;
        case GATE_NOT:    return VGA_LRED;
        case GATE_NAND:   return VGA_GREEN;
        case GATE_NOR:    return VGA_BRWN;
        case GATE_XNOR:   return VGA_MAGENTA;
        case GATE_BUFFER: return VGA_LGREY;
        case GATE_INPUT:  return VGA_LCYAN;
        case GATE_OUTPUT: return VGA_LBLUE;
        case GATE_SWITCH: return VGA_WHITE;
        case GATE_LED:    return VGA_LRED;
        default:          return VGA_WHITE;
    }
}

int gate_get_input_count(GateType type) {
    switch (type) {
        case GATE_NOT:
        case GATE_BUFFER:
        case GATE_INPUT:
        case GATE_SWITCH:
            return 1;
        case GATE_AND:
        case GATE_OR:
        case GATE_XOR:
        case GATE_NAND:
        case GATE_NOR:
        case GATE_XNOR:
            return 2;
        case GATE_OUTPUT:
        case GATE_LED:
            return 1;
        default:
            return 0;
    }
}

int gate_get_pin_count(GateType type) {
    return gate_get_input_count(type) + 1;
}


void circuit_init(LCGameSession* session) {
    session->gate_count = 0;
    session->wire_count = 0;
    session->cycles = 0;
    session->selected_gate_id = -1;
    
    for (int i = 0; i < MAX_GATES; i++) {
        session->gates[i].active = 0;
        session->gates[i].selected = 0;
        session->gates[i].inputs[0] = 0;
        session->gates[i].inputs[1] = 0;
        session->gates[i].output = 0;
    }
    
    for (int i = 0; i < MAX_WIRES; i++) {
        session->wires[i].active = 0;
    }
}

int circuit_add_gate(LCGameSession* session, GateType type, int x, int y) {
    if (session->gate_count >= MAX_GATES) return -1;

    int id = -1;
    for (int i = 0; i < MAX_GATES; i++) {
        if (!session->gates[i].active) {
            id = i;
            break;
        }
    }
    
    if (id == -1) return -1;
    
    Gate* gate = &session->gates[id];
    gate->type = type;
    gate->x = x;
    gate->y = y;
    gate->active = 1;
    gate->inputs[0] = 0;
    gate->inputs[1] = 0;
    gate->output = 0;
    gate->id = id;
    gate->color = gate_get_color(type);
    gate->selected = 0;
    
    const char* name = gate_get_name(type);
    int i;
    for (i = 0; name[i] && i < 7; i++) {
        gate->label[i] = name[i];
    }
    gate->label[i] = '\0';
    
    session->gate_count++;
    session->total_gates_placed++;
    
    return id;
}

int circuit_add_wire(LCGameSession* session, ConnectionPoint from, ConnectionPoint to) {
    if (session->wire_count >= MAX_WIRES) return -1;

    if (from.gate_id < 0 || from.gate_id >= MAX_GATES || 
        !session->gates[from.gate_id].active) return -1;
    if (to.gate_id < 0 || to.gate_id >= MAX_GATES || 
        !session->gates[to.gate_id].active) return -1;

    int id = -1;
    for (int i = 0; i < MAX_WIRES; i++) {
        if (!session->wires[i].active) {
            id = i;
            break;
        }
    }
    
    if (id == -1) return -1;
    
    Wire* wire = &session->wires[id];
    wire->from = from;
    wire->to = to;
    wire->active = 1;
    wire->signal = 0;
    wire->color = VGA_DGREY;
    
    session->wire_count++;
    session->total_wires_placed++;
    
    return id;
}

void circuit_remove_gate(LCGameSession* session, int gate_id) {
    if (gate_id < 0 || gate_id >= MAX_GATES) return;

    for (int i = 0; i < session->wire_count; i++) {
        if (session->wires[i].active) {
            if (session->wires[i].from.gate_id == gate_id ||
                session->wires[i].to.gate_id == gate_id) {
                session->wires[i].active = 0;
            }
        }
    }
    
    session->gates[gate_id].active = 0;
    session->gates[gate_id].selected = 0;
    
    if (session->selected_gate_id == gate_id) {
        session->selected_gate_id = -1;
    }
}

Gate* circuit_get_gate_at(LCGameSession* session, int x, int y) {
    for (int i = 0; i < MAX_GATES; i++) {
        if (session->gates[i].active) {
            if (session->gates[i].x == x && session->gates[i].y == y) {
                return &session->gates[i];
            }
        }
    }
    return 0;
}

Gate* circuit_get_gate(LCGameSession* session, int gate_id) {
    if (gate_id >= 0 && gate_id < MAX_GATES && session->gates[gate_id].active) {
        return &session->gates[gate_id];
    }
    return 0;
}

void circuit_simulate_step(LCGameSession* session) {
    for (int i = 0; i < session->wire_count; i++) {
        if (!session->wires[i].active) continue;
        
        Wire* wire = &session->wires[i];
        Gate* from_gate = &session->gates[wire->from.gate_id];
        Gate* to_gate = &session->gates[wire->to.gate_id];

        int signal = 0;
        if (wire->from.pin == PIN_OUTPUT) {
            signal = from_gate->output;
        } else if (wire->from.pin == PIN_INPUT1) {
            signal = from_gate->inputs[0];
        } else if (wire->from.pin == PIN_INPUT2) {
            signal = from_gate->inputs[1];
        }
        
        wire->signal = signal;
        wire->color = signal ? VGA_LGREEN : VGA_DGREY;

        if (wire->to.pin == PIN_INPUT1) {
            to_gate->inputs[0] = signal;
        } else if (wire->to.pin == PIN_INPUT2) {
            to_gate->inputs[1] = signal;
        } else if (wire->to.pin == PIN_OUTPUT) {
            to_gate->output = signal;
        }
    }

    for (int i = 0; i < MAX_GATES; i++) {
        if (!session->gates[i].active) continue;
        
        Gate* gate = &session->gates[i];
        
        if (gate->type == GATE_INPUT || gate->type == GATE_SWITCH) {
            continue;
        } else if (gate->type == GATE_OUTPUT || gate->type == GATE_LED) {
            gate->output = gate->inputs[0];
        } else {
            gate->output = gate_evaluate(gate->type, gate->inputs[0], gate->inputs[1]);
        }
    }
    
    session->cycles++;
}

void circuit_simulate_full(LCGameSession* session) {
    for (int i = 0; i < 10; i++) {
        circuit_simulate_step(session);
    }
}

void circuit_clear(LCGameSession* session) {
    circuit_init(session);
    session->connection_state = CONNECTION_NONE;
}


void logiccircuit_init_levels(void) {
    level_count = 0;
    
    if (level_count > 0) {
        return; 
    }

    if (level_count < MAX_LEVELS) {
        LCLevel* lv = &levels[level_count];
        strcpy(lv->name, "Tutorial: Basic Gates");
        strcpy(lv->description, "Learn to place and connect gates");
        strcpy(lv->hint, "Use arrow keys to move, 1-9 for gates, SPACE to place");
        lv->difficulty = 1;
        lv->max_gates = 15;
        lv->allowed_gate_types = 0xFFFF;
        lv->tutorial = 1;
        lv->objective.input_count = 2;
        lv->objective.output_count = 1;
        lv->objective.inputs[0] = 1;
        lv->objective.inputs[1] = 0;
        lv->objective.outputs[0] = 1;
        level_count++;
    }

    if (level_count < MAX_LEVELS) {
        LCLevel* lv = &levels[level_count];
        strcpy(lv->name, "AND Gate Challenge");
        strcpy(lv->description, "Build a circuit that outputs 1 when both inputs are 1");
        strcpy(lv->hint, "Use two INPUTs, one AND gate (1), and one OUTPUT");
        lv->difficulty = 1;
        lv->max_gates = 10;
        lv->allowed_gate_types = (1 << GATE_INPUT) | (1 << GATE_OUTPUT) | (1 << GATE_AND);
        lv->tutorial = 0;
        lv->objective.input_count = 2;
        lv->objective.output_count = 1;
        lv->objective.inputs[0] = 1;
        lv->objective.inputs[1] = 1;
        lv->objective.outputs[0] = 1;
        level_count++;
    }

    if (level_count < MAX_LEVELS) {
        LCLevel* lv = &levels[level_count];
        strcpy(lv->name, "OR Gate Challenge");
        strcpy(lv->description, "Output 1 when at least one input is 1");
        strcpy(lv->hint, "Use OR gate (2) instead of AND");
        lv->difficulty = 1;
        lv->max_gates = 10;
        lv->allowed_gate_types = (1 << GATE_INPUT) | (1 << GATE_OUTPUT) | (1 << GATE_OR);
        lv->tutorial = 0;
        lv->objective.input_count = 2;
        lv->objective.output_count = 1;
        lv->objective.inputs[0] = 1;
        lv->objective.inputs[1] = 0;
        lv->objective.outputs[0] = 1;
        level_count++;
    }

    if (level_count < MAX_LEVELS) {
        LCLevel* lv = &levels[level_count];
        strcpy(lv->name, "XOR Gate Challenge");
        strcpy(lv->description, "Output 1 when inputs are different");
        strcpy(lv->hint, "XOR gate (3) outputs 1 only when inputs differ");
        lv->difficulty = 2;
        lv->max_gates = 10;
        lv->allowed_gate_types = (1 << GATE_INPUT) | (1 << GATE_OUTPUT) | (1 << GATE_XOR);
        lv->tutorial = 0;
        lv->objective.input_count = 2;
        lv->objective.output_count = 1;
        lv->objective.inputs[0] = 1;
        lv->objective.inputs[1] = 0;
        lv->objective.outputs[0] = 1;
        level_count++;
    }

    if (level_count < MAX_LEVELS) {
        LCLevel* lv = &levels[level_count];
        strcpy(lv->name, "NOT Gate Challenge");
        strcpy(lv->description, "Invert the input signal");
        strcpy(lv->hint, "Connect INPUT to NOT gate (4) to OUTPUT");
        lv->difficulty = 1;
        lv->max_gates = 10;
        lv->allowed_gate_types = (1 << GATE_INPUT) | (1 << GATE_OUTPUT) | (1 << GATE_NOT);
        lv->tutorial = 0;
        lv->objective.input_count = 1;
        lv->objective.output_count = 1;
        lv->objective.inputs[0] = 1;
        lv->objective.outputs[0] = 0;
        level_count++;
    }
}

void logiccircuit_load_level(LCGameSession* session, int level_index) {
    if (level_index < 0 || level_index >= level_count) return;
    
    session->current_level = levels[level_index];
    session->current_level_index = level_index;
    circuit_clear(session);

    session->cursor_x = GRID_WIDTH / 2;
    session->cursor_y = GRID_HEIGHT / 2;

    for (int i = 0; i < session->current_level.objective.input_count; i++) {
        circuit_add_gate(session, GATE_INPUT, 10, 5 + i * 3);
    }

    for (int i = 0; i < session->current_level.objective.output_count; i++) {
        circuit_add_gate(session, GATE_OUTPUT, GRID_WIDTH - 10, 5 + i * 3);
    }
}

int logiccircuit_check_level_complete(LCGameSession* session) {
    circuit_simulate_full(session);
    
    int output_index = 0;
    for (int i = 0; i < MAX_GATES; i++) {
        if (!session->gates[i].active) continue;
        
        if (session->gates[i].type == GATE_OUTPUT) {
            if (output_index >= session->current_level.objective.output_count) return 0;
            
            int expected = session->current_level.objective.outputs[output_index];
            int actual = session->gates[i].output;
            
            if (expected != actual) return 0;
            
            output_index++;
        }
    }
    
    return (output_index == session->current_level.objective.output_count);
}

void logiccircuit_unlock_next_level(LCGameSession* session) {
    if (session->current_level_index < MAX_LEVELS - 1) {
        session->levels_completed[session->current_level_index] = 1;
        if (session->current_level_index >= session->levels_solved) {
            session->levels_solved = session->current_level_index + 1;
        }
    }
}


void logiccircuit_update_input_timing(LCGameSession* session) {
    uint32_t current_time = pit_get_total_milliseconds();

    if (current_time - session->last_input_time < session->input_repeat_delay) {
        return; 
    }
    
    session->last_input_time = current_time;

    keyboard_poll();
    
    if (!has_key()) return;
    
    char c = get_char();

    if (session->state == CIRCUIT_MENU) {
        logiccircuit_handle_menu_input(session, c);
    } else if (session->state == CIRCUIT_LEVEL_SELECT) {
        logiccircuit_handle_menu_input(session, c);
    } else if (session->state == CIRCUIT_PLAYING || session->state == CIRCUIT_SANDBOX) {
        logiccircuit_handle_game_input(session, c);
    } else if (session->state == CIRCUIT_HELP) {
        if (c == 27 || c == ' ' || c == '\n') {
            session->state = CIRCUIT_MENU;
        }
    } else if (session->state == CIRCUIT_LEVEL_COMPLETE) {
        if (c == ' ' || c == '\n') {
            if (session->current_level_index < level_count - 1) {
                session->current_level_index++;
                logiccircuit_load_level(session, session->current_level_index);
                session->state = CIRCUIT_PLAYING;
            } else {
                session->state = CIRCUIT_MENU;
            }
        } else if (c == 27) {
            session->state = CIRCUIT_MENU;
        }
    }
}

void logiccircuit_handle_menu_input(LCGameSession* session, char c) {
    if (c == 0x11) { 
        session->menu_selection--;
        if (session->menu_selection < 0) session->menu_selection = 3;
        session->input_repeat_delay = 200;
    } else if (c == 0x12) {  
        session->menu_selection++;
        if (session->menu_selection > 3) session->menu_selection = 0;
        session->input_repeat_delay = 200;
    } else if (c == '\n') {  
        if (session->state == CIRCUIT_MENU) {
            switch (session->menu_selection) {
                case 0: session->state = CIRCUIT_LEVEL_SELECT; break;
                case 1: 
                    session->state = CIRCUIT_SANDBOX; 
                    circuit_clear(session);
                    break;
                case 2: session->state = CIRCUIT_HELP; break;
                case 3: session->state = CIRCUIT_EXIT; break;
            }
        } else if (session->state == CIRCUIT_LEVEL_SELECT) {
            logiccircuit_load_level(session, session->current_level_index);
            session->state = CIRCUIT_PLAYING;
        }
        session->input_repeat_delay = 300;
    } else if (c == 27) {  
        if (session->state == CIRCUIT_LEVEL_SELECT) {
            session->state = CIRCUIT_MENU;
        }
        session->input_repeat_delay = 300;
    }
}

void logiccircuit_handle_game_input(LCGameSession* session, char c) {
    if (session->connection_state != CONNECTION_NONE) {
        logiccircuit_handle_connection_mode(session, c);
        return;
    }

    if (c == 0x11) {  
        logiccircuit_move_cursor(session, 0, -1);
        session->input_repeat_delay = session->fast_repeat_mode ? 30 : 100;
    } else if (c == 0x12) {  
        logiccircuit_move_cursor(session, 0, 1);
        session->input_repeat_delay = session->fast_repeat_mode ? 30 : 100;
    } else if (c == 0x13) {  
        logiccircuit_move_cursor(session, -1, 0);
        session->input_repeat_delay = session->fast_repeat_mode ? 30 : 100;
    } else if (c == 0x14) {  
        logiccircuit_move_cursor(session, 1, 0);
        session->input_repeat_delay = session->fast_repeat_mode ? 30 : 100;
    } else if (c == 27) { 
        session->state = CIRCUIT_MENU;
        session->input_repeat_delay = 300;
    }

    else if (c >= '1' && c <= '9') {
        int index = c - '1';
        GateType types[] = {
            GATE_AND, GATE_OR, GATE_XOR, GATE_NOT,
            GATE_NAND, GATE_NOR, GATE_XNOR, GATE_BUFFER,
            GATE_INPUT
        };
        if (index < 9) {
            session->selected_gate_type = types[index];
            session->current_tool = TOOL_PLACE_GATE;
        }
        session->input_repeat_delay = 200;
    } else if (c == '0') {
        session->selected_gate_type = GATE_OUTPUT;
        session->current_tool = TOOL_PLACE_GATE;
        session->input_repeat_delay = 200;
    } else if (c == 's' || c == 'S') {
        session->selected_gate_type = GATE_SWITCH;
        session->current_tool = TOOL_PLACE_GATE;
        session->input_repeat_delay = 200;
    } else if (c == 'l' || c == 'L') {
        session->selected_gate_type = GATE_LED;
        session->current_tool = TOOL_PLACE_GATE;
        session->input_repeat_delay = 200;
    }

    else if (c == 'q' || c == 'Q') {
        session->current_tool = TOOL_SELECT;
        session->input_repeat_delay = 200;
    } else if (c == 'w' || c == 'W') {
        session->current_tool = TOOL_WIRE;
        session->input_repeat_delay = 200;
    } else if (c == 'e' || c == 'E') {
        session->current_tool = TOOL_DELETE;
        session->input_repeat_delay = 200;
    } else if (c == 'p' || c == 'P') {
        session->current_tool = TOOL_PLACE_GATE;
        session->input_repeat_delay = 200;
    } else if (c == 'm' || c == 'M') {
        session->current_tool = TOOL_MOVE;
        session->input_repeat_delay = 200;
    }

    else if (c == ' ') {  
        logiccircuit_handle_primary_action(session);
        session->input_repeat_delay = 200;
    } else if (c == 't' || c == 'T') {  
        Gate* gate = circuit_get_gate_at(session, session->cursor_x, session->cursor_y);
        if (gate && (gate->type == GATE_INPUT || gate->type == GATE_SWITCH)) {
            gate->output = !gate->output;
            gate->inputs[0] = gate->output;
            circuit_simulate_full(session);
        }
        session->input_repeat_delay = 200;
    } else if (c == 'r' || c == 'R') {  
        circuit_simulate_full(session);
        session->input_repeat_delay = 200;
    } else if (c == 'c' || c == 'C') {  
        circuit_clear(session);
        if (session->state == CIRCUIT_PLAYING) {
            logiccircuit_load_level(session, session->current_level_index);
        }
        session->input_repeat_delay = 200;
    } else if (c == 'x' || c == 'X') {  
        if (session->state == CIRCUIT_PLAYING) {
            if (logiccircuit_check_level_complete(session)) {
                session->state = CIRCUIT_LEVEL_COMPLETE;
                logiccircuit_unlock_next_level(session);
            }
        }
        session->input_repeat_delay = 200;
    } else if (c == 'h' || c == 'H') {  
        session->state = CIRCUIT_HELP;
        session->input_repeat_delay = 200;
    } else if (c == 'f' || c == 'F') {  
        session->fast_repeat_mode = !session->fast_repeat_mode;
        session->input_repeat_delay = 200;
    }
}

void logiccircuit_handle_primary_action(LCGameSession* session) {
    switch (session->current_tool) {
        case TOOL_PLACE_GATE:
            if (logiccircuit_is_valid_placement(session, session->cursor_x, session->cursor_y)) {
                circuit_add_gate(session, session->selected_gate_type, 
                               session->cursor_x, session->cursor_y);
            }
            break;
            
        case TOOL_SELECT:
            logiccircuit_select_gate_at_cursor(session);
            break;
            
        case TOOL_WIRE:
            if (session->connection_state == CONNECTION_NONE) {
                Gate* gate = circuit_get_gate_at(session, session->cursor_x, session->cursor_y);
                if (gate) {
                    session->connection_state = CONNECTION_SELECTING_SOURCE;
                    session->selected_gate_id = gate->id;
                    session->selected_pin = -1;
                }
            }
            break;
            
        case TOOL_DELETE:
            Gate* gate = circuit_get_gate_at(session, session->cursor_x, session->cursor_y);
            if (gate) {
                circuit_remove_gate(session, gate->id);
            }
            break;
            
        case TOOL_MOVE:
            if (session->selected_gate_id != -1) {
                Gate* gate = circuit_get_gate(session, session->selected_gate_id);
                if (gate && logiccircuit_is_valid_placement(session, session->cursor_x, session->cursor_y)) {
                    gate->x = session->cursor_x;
                    gate->y = session->cursor_y;
                }
            }
            break;
    }
}

void logiccircuit_handle_connection_mode(LCGameSession* session, char c) {
    Gate* gate = circuit_get_gate(session, session->selected_gate_id);
    if (!gate) {
        session->connection_state = CONNECTION_NONE;
        return;
    }
    
    if (session->connection_state == CONNECTION_SELECTING_SOURCE) {
        if (c == 'o' || c == 'O') {  
            session->connection_source.gate_id = gate->id;
            session->connection_source.pin = PIN_OUTPUT;
            session->connection_state = CONNECTION_SELECTING_TARGET;
            session->turbo_mode = 1;  
        } else if (c == '1' && gate_get_input_count(gate->type) >= 1) {  
            session->connection_source.gate_id = gate->id;
            session->connection_source.pin = PIN_INPUT1;
            session->connection_state = CONNECTION_SELECTING_TARGET;
            session->turbo_mode = 1;  
        } else if (c == '2' && gate_get_input_count(gate->type) >= 2) {  
            session->connection_source.gate_id = gate->id;
            session->connection_source.pin = PIN_INPUT2;
            session->connection_state = CONNECTION_SELECTING_TARGET;
            session->turbo_mode = 1;  
        } else if (c == 27) {  
            session->connection_state = CONNECTION_NONE;
            session->turbo_mode = 0;  
        }
    } else if (session->connection_state == CONNECTION_SELECTING_TARGET) {

        int move_amount = session->turbo_mode ? 3 : 1;  
        
        if (c == 27) {  
            session->connection_state = CONNECTION_NONE;
            session->selected_gate_id = -1;
            session->turbo_mode = 0;  
        } else if (c == 0x11) { 
            session->cursor_y -= move_amount;
            if (session->cursor_y < 0) session->cursor_y = 0;
        } else if (c == 0x12) {  
            session->cursor_y += move_amount;
            if (session->cursor_y >= GRID_HEIGHT) session->cursor_y = GRID_HEIGHT - 1;
        } else if (c == 0x13) {  
            session->cursor_x -= move_amount;
            if (session->cursor_x < 0) session->cursor_x = 0;
        } else if (c == 0x14) {  
            session->cursor_x += move_amount;
            if (session->cursor_x >= GRID_WIDTH) session->cursor_x = GRID_WIDTH - 1;
        } else if (c == ' ') {  
            Gate* target_gate = circuit_get_gate_at(session, session->cursor_x, session->cursor_y);
            if (target_gate && target_gate->id != session->connection_source.gate_id) {
                session->connection_target.gate_id = target_gate->id;
                session->selected_gate_id = target_gate->id;
                session->connection_state = CONNECTION_SELECTING_PIN;
            }
        } else if (c == 't' || c == 'T') {  
            session->turbo_mode = !session->turbo_mode;
        }
    } else if (session->connection_state == CONNECTION_SELECTING_PIN) {
        if (c == '1' && gate_get_input_count(gate->type) >= 1) { 
            session->connection_target.pin = PIN_INPUT1;
            if (circuit_add_wire(session, session->connection_source, session->connection_target) >= 0) {
                session->connection_state = CONNECTION_NONE;
                session->turbo_mode = 0;  
            }
        } else if (c == '2' && gate_get_input_count(gate->type) >= 2) { 
            session->connection_target.pin = PIN_INPUT2;
            if (circuit_add_wire(session, session->connection_source, session->connection_target) >= 0) {
                session->connection_state = CONNECTION_NONE;
                session->turbo_mode = 0; 
            }
        } else if (c == 'o' || c == 'O') { 
            session->connection_target.pin = PIN_OUTPUT;
            if (circuit_add_wire(session, session->connection_source, session->connection_target) >= 0) {
                session->connection_state = CONNECTION_NONE;
                session->turbo_mode = 0;  
            }
        } else if (c == 27) {  
            session->connection_state = CONNECTION_NONE;
            session->turbo_mode = 0;  
        }
    }
}


void logiccircuit_grid_to_screen(int grid_x, int grid_y, int* screen_x, int* screen_y) {
    *screen_x = GRID_OFFSET_X + grid_x;
    *screen_y = GRID_OFFSET_Y + grid_y;
}

void logiccircuit_move_cursor(LCGameSession* session, int dx, int dy) {
    session->cursor_x += dx;
    session->cursor_y += dy;
    
    if (session->cursor_x < 0) session->cursor_x = 0;
    if (session->cursor_x >= GRID_WIDTH) session->cursor_x = GRID_WIDTH - 1;
    if (session->cursor_y < 0) session->cursor_y = 0;
    if (session->cursor_y >= GRID_HEIGHT) session->cursor_y = GRID_HEIGHT - 1;
}

int logiccircuit_is_valid_placement(LCGameSession* session, int x, int y) {
    if (x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT) return 0;

    if (circuit_get_gate_at(session, x, y)) return 0;
    
    return 1;
}

void logiccircuit_select_gate_at_cursor(LCGameSession* session) {
    Gate* gate = circuit_get_gate_at(session, session->cursor_x, session->cursor_y);
    if (gate) {

        logiccircuit_deselect_all_gates(session);

        gate->selected = 1;
        session->selected_gate_id = gate->id;
    } else {

        logiccircuit_deselect_all_gates(session);
        session->selected_gate_id = -1;
    }
}

void logiccircuit_deselect_all_gates(LCGameSession* session) {
    for (int i = 0; i < MAX_GATES; i++) {
        session->gates[i].selected = 0;
    }
}

int logiccircuit_get_gate_pin_at_cursor(LCGameSession* session, Gate* gate, PinType* pin_type) {

    int dx = session->cursor_x - gate->x;
    int dy = session->cursor_y - gate->y;
    
    if (dx == -1 && dy == 0) { 
        *pin_type = PIN_INPUT1;
        return 1;
    } else if (dx == -1 && dy == 1 && gate_get_input_count(gate->type) >= 2) { 
        *pin_type = PIN_INPUT2;
        return 1;
    } else if (dx == 1 && dy == 0) { 
        *pin_type = PIN_OUTPUT;
        return 1;
    }
    
    return 0;
}


void logiccircuit_draw_menu(LCGameSession* session) {
    vga_clear();

    vga_set_color(VGA_LCYAN, VGA_BLCK);
    print_centered("LOGIC CIRCUIT SIMULATOR", 3);
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    print_centered("Digital Logic Sandbox", 4);

    const char* options[] = {
        "Play Levels",
        "Sandbox Mode",
        "Help & Tutorial",
        "Exit Game"
    };
    
    for (int i = 0; i < 4; i++) {
        int y = 10 + i * 2;
        
        if (i == session->menu_selection) {
            vga_set_color(VGA_YELLOW, VGA_BLUE);
            fill_rect(28, y, 24, 1, ' ', VGA_YELLOW, VGA_BLUE);
            vga_print_at("> ", 30, y);
        } else {
            vga_set_color(VGA_WHITE, VGA_BLCK);
            vga_print_at("  ", 30, y);
        }
        
        vga_print_at(options[i], 32, y);
    }

    vga_set_color(VGA_LGREY, VGA_BLCK);
    char stats[40];
    int pos = 0;
    const char* label = "Progress: ";
    for (int i = 0; label[i]; i++) stats[pos++] = label[i];
    
    int levels_solved = session->levels_solved;
    if (levels_solved >= 10) stats[pos++] = '0' + (levels_solved / 10);
    stats[pos++] = '0' + (levels_solved % 10);
    stats[pos++] = '/';
    if (level_count >= 10) stats[pos++] = '0' + (level_count / 10);
    stats[pos++] = '0' + (level_count % 10);
    stats[pos] = '\0';
    
    print_centered(stats, 19);

    vga_set_color(VGA_DGREY, VGA_BLCK);
    print_centered("UP/DOWN: Navigate  ENTER: Select  ESC: Exit", 23);
}

void logiccircuit_draw_level_select(LCGameSession* session) {
    vga_clear();
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    print_centered("SELECT LEVEL", 1);

    if (level_count == 0) {
        vga_set_color(VGA_RED, VGA_BLCK);
        print_centered("No levels available!", 10);
        print_centered("Please restart the game", 11);
    } else {
        draw_box(5, 3, 70, 18, VGA_LGREY, VGA_BLCK);
        
        int start = 0;
        int end = level_count;
        if (end > 10) end = 10; 
        
        for (int i = start; i < end; i++) {
            int y = 5 + (i - start);
            int selected = (i == session->current_level_index);
            int completed = session->levels_completed[i];
            
            if (selected) {
                vga_set_color(VGA_YELLOW, VGA_BLUE);
                fill_rect(10, y, 60, 1, ' ', VGA_YELLOW, VGA_BLUE);
            } else if (completed) {
                vga_set_color(VGA_LGREEN, VGA_BLCK);
            } else {
                vga_set_color(VGA_WHITE, VGA_BLCK);
            }

            char level_str[70];
            int pos = 0;
            level_str[pos++] = (completed ? '*' : ' ');
            level_str[pos++] = ' ';

            int level_num = i + 1;
            if (level_num >= 10) level_str[pos++] = '0' + (level_num / 10);
            level_str[pos++] = '0' + (level_num % 10);
            level_str[pos++] = '.';
            level_str[pos++] = ' ';

            const char* name = levels[i].name;
            for (int j = 0; name[j] && j < 30; j++) {
                level_str[pos++] = name[j];
            }

            level_str[pos++] = ' ';
            level_str[pos++] = '[';
            for (int j = 0; j < levels[i].difficulty; j++) {
                level_str[pos++] = '*';
            }
            level_str[pos++] = ']';
            level_str[pos] = '\0';
            
            vga_print_at(level_str, 12, y);
        }

        if (session->current_level_index < level_count) {
            vga_set_color(VGA_LCYAN, VGA_BLCK);
            print_centered(levels[session->current_level_index].description, 17);
            
            vga_set_color(VGA_LGREY, VGA_BLCK);
            print_centered(levels[session->current_level_index].hint, 18);
        }
    }

    vga_set_color(VGA_DGREY, VGA_BLCK);
    const char* inst = "UP/DOWN: Select | ENTER: Play | ESC: Back";
    int len = 0;
    while (inst[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((80 - len) / 2 + i, 22, inst[i]);
    }
}

void logiccircuit_draw_game(LCGameSession* session) {
    vga_clear();

    vga_set_color(VGA_WHITE, VGA_BLUE);
    fill_rect(0, 0, 80, 1, ' ', VGA_WHITE, VGA_BLUE);
    
    char title[60];
    int pos = 0;
    const char* mode = (session->state == CIRCUIT_PLAYING) ? "LEVEL MODE" : "SANDBOX MODE";
    
    const char* prefix = "Logic Circuit - ";
    for (int i = 0; prefix[i] && pos < 59; i++) title[pos++] = prefix[i];
    for (int i = 0; mode[i] && pos < 59; i++) title[pos++] = mode[i];
    
    if (session->state == CIRCUIT_PLAYING) {
        title[pos++] = ' ';
        title[pos++] = '(';
        title[pos++] = '0' + (session->current_level_index + 1);
        title[pos++] = ')';
    }
    title[pos] = '\0';
    
    int title_len = 0;
    while (title[title_len]) title_len++;
    int title_start = (80 - title_len) / 2;
    for (int i = 0; i < title_len; i++) {
        vga_putchr_at(title_start + i, 0, title[i]);
    }

    logiccircuit_draw_grid();

    logiccircuit_draw_wires(session);

    logiccircuit_draw_gates(session);

    logiccircuit_draw_cursor(session);

    if (session->connection_state != CONNECTION_NONE) {
        logiccircuit_draw_connection_info(session);
    }

    logiccircuit_draw_toolbox(session);
    logiccircuit_draw_status(session);

    vga_set_color(VGA_DGREY, VGA_BLCK);
    const char* controls1 = "ARROWS: Move  SPACE: Action  F: Fast Move  ESC: Menu";
    const char* controls2 = "1-9: Gates  0:OUT  S:SW  L:LED  Q:Sel  W:Wire  E:Del  P:Place  R:Run  X:Test";
    
    print_centered(controls1, 23);
    print_centered(controls2, 24);
}

void logiccircuit_draw_grid(void) {
    vga_set_color(VGA_DGREY, VGA_BLCK);
    
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            int screen_x = GRID_OFFSET_X + x;
            int screen_y = GRID_OFFSET_Y + y;
            
            if (x % 5 == 0 && y % 5 == 0) {
                vga_putchr_at(screen_x, screen_y, '+');
            } else if (x % 5 == 0) {
                vga_putchr_at(screen_x, screen_y, '|');
            } else if (y % 5 == 0) {
                vga_putchr_at(screen_x, screen_y, '-');
            } else {
                vga_putchr_at(screen_x, screen_y, '.');
            }
        }
    }

    draw_box(GRID_OFFSET_X - 2, GRID_OFFSET_Y - 1, 
             GRID_WIDTH + 4, GRID_HEIGHT + 2, VGA_LGREY, VGA_BLCK);
}

void logiccircuit_draw_gates(LCGameSession* session) {
    for (int i = 0; i < MAX_GATES; i++) {
        if (session->gates[i].active) {
            logiccircuit_draw_gate(session, &session->gates[i]);
        }
    }
}

void logiccircuit_draw_gate(LCGameSession* session, Gate* gate) {
    int screen_x, screen_y;
    logiccircuit_grid_to_screen(gate->x, gate->y, &screen_x, &screen_y);
    
    uint8_t bg_color = gate->color;
    uint8_t fg_color = VGA_BLCK;

    if (gate->selected || 
        (session->connection_state != CONNECTION_NONE && 
         session->selected_gate_id == gate->id)) {
        bg_color = VGA_YELLOW;
        fg_color = VGA_BLCK;
    }

    if (gate->type == GATE_LED || gate->type == GATE_OUTPUT) {
        fg_color = gate->output ? VGA_LGREEN : VGA_LRED;
    } else if (gate->type == GATE_INPUT || gate->type == GATE_SWITCH) {
        fg_color = gate->output ? VGA_LGREEN : VGA_DGREY;
    } else {
        fg_color = VGA_WHITE;
    }

    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            uint8_t old_color = vga_t_color;
            vga_set_color(fg_color, bg_color);
            vga_putchr_at(screen_x + dx, screen_y + dy, ' ');
            vga_t_color = old_color;
        }
    }

    uint8_t old_color = vga_t_color;
    vga_set_color(fg_color, bg_color);
    vga_putchr_at(screen_x, screen_y, gate_get_symbol(gate->type));

    vga_set_color(VGA_WHITE, bg_color);
    for (int i = 0; gate->label[i] && i < 3; i++) {
        vga_putchr_at(screen_x - 1 + i, screen_y - 2, gate->label[i]);
    }

    int input_count = gate_get_input_count(gate->type);

    if (input_count >= 1) {
        vga_set_color(gate->inputs[0] ? VGA_LGREEN : VGA_LRED, bg_color);
        vga_putchr_at(screen_x - 2, screen_y, gate->inputs[0] ? '1' : '0');
    }
    if (input_count >= 2) {
        vga_set_color(gate->inputs[1] ? VGA_LGREEN : VGA_LRED, bg_color);
        vga_putchr_at(screen_x - 2, screen_y + 1, gate->inputs[1] ? '1' : '0');
    }

    vga_set_color(gate->output ? VGA_LGREEN : VGA_LRED, bg_color);
    vga_putchr_at(screen_x + 2, screen_y, gate->output ? '1' : '0');
    
    vga_t_color = old_color;

    if (session->connection_state != CONNECTION_NONE && 
        session->selected_gate_id == gate->id) {
        logiccircuit_draw_pin_indicators(gate, 1);
    }
}

void logiccircuit_draw_pin_indicators(Gate* gate, int show_pins) {
    int screen_x, screen_y;
    logiccircuit_grid_to_screen(gate->x, gate->y, &screen_x, &screen_y);
    
    if (!show_pins) return;
    
    uint8_t old_color = vga_t_color;

    int input_count = gate_get_input_count(gate->type);
    
    if (input_count >= 1) {
        vga_set_color(VGA_CYAN, VGA_BLCK);
        vga_putchr_at(screen_x - 2, screen_y - 1, '1');
    }
    
    if (input_count >= 2) {
        vga_set_color(VGA_CYAN, VGA_BLCK);
        vga_putchr_at(screen_x - 2, screen_y + 2, '2');
    }

    vga_set_color(VGA_CYAN, VGA_BLCK);
    vga_putchr_at(screen_x + 2, screen_y - 1, 'O');
    
    vga_t_color = old_color;
}

void logiccircuit_draw_wires(LCGameSession* session) {
    for (int i = 0; i < session->wire_count; i++) {
        if (!session->wires[i].active) continue;
        
        Wire* wire = &session->wires[i];
        Gate* from_gate = &session->gates[wire->from.gate_id];
        Gate* to_gate = &session->gates[wire->to.gate_id];

        int from_x, from_y, to_x, to_y;
        logiccircuit_grid_to_screen(from_gate->x, from_gate->y, &from_x, &from_y);
        logiccircuit_grid_to_screen(to_gate->x, to_gate->y, &to_x, &to_y);

        switch (wire->from.pin) {
            case PIN_INPUT1: from_x -= 2; break;
            case PIN_INPUT2: from_x -= 2; from_y += 1; break;
            case PIN_OUTPUT: from_x += 2; break;
            default: break;
        }
        
        switch (wire->to.pin) {
            case PIN_INPUT1: to_x -= 2; break;
            case PIN_INPUT2: to_x -= 2; to_y += 1; break;
            case PIN_OUTPUT: to_x += 2; break;
            default: break;
        }

        uint8_t old_color = vga_t_color;
        vga_set_color(wire->signal ? VGA_LGREEN : VGA_DGREY, VGA_BLCK);

        if (from_x != to_x) {
            int step = (from_x < to_x) ? 1 : -1;
            for (int x = from_x; x != to_x; x += step) {
                vga_putchr_at(x, from_y, '-');
            }
        }
        
        if (from_y != to_y) {
            int step = (from_y < to_y) ? 1 : -1;
            for (int y = from_y; y != to_y; y += step) {
                vga_putchr_at(to_x, y, '|');
            }
        }

        vga_putchr_at(from_x, from_y, 'o');
        vga_putchr_at(to_x, to_y, 'o');
        
        vga_t_color = old_color;
    }
}

void logiccircuit_draw_cursor(LCGameSession* session) {
    int screen_x, screen_y;
    logiccircuit_grid_to_screen(session->cursor_x, session->cursor_y, &screen_x, &screen_y);
    
    uint8_t old_color = vga_t_color;
    
    if (session->connection_state != CONNECTION_NONE) {

        vga_set_color(VGA_LMAGENTA, VGA_BLCK);
        vga_putchr_at(screen_x, screen_y, 'X');
        

        Gate* gate_at_cursor = circuit_get_gate_at(session, session->cursor_x, session->cursor_y);
        if (gate_at_cursor) {
            vga_set_color(VGA_CYAN, gate_at_cursor->color);
            vga_putchr_at(screen_x, screen_y, '?');  
        }
    } else {

        switch (session->current_tool) {
            case TOOL_PLACE_GATE:
                vga_set_color(VGA_CYAN, VGA_BLCK);
                vga_putchr_at(screen_x, screen_y, '+');
                break;
            case TOOL_SELECT:
                vga_set_color(VGA_YELLOW, VGA_BLCK);
                vga_putchr_at(screen_x, screen_y, 'X');
                break;
            case TOOL_WIRE:
                vga_set_color(VGA_LMAGENTA, VGA_BLCK);
                vga_putchr_at(screen_x, screen_y, 'W');
                break;
            case TOOL_DELETE:
                vga_set_color(VGA_LRED, VGA_BLCK);
                vga_putchr_at(screen_x, screen_y, 'X');
                break;
            case TOOL_MOVE:
                vga_set_color(VGA_WHITE, VGA_BLCK);
                vga_putchr_at(screen_x, screen_y, 'M');
                break;
        }
    }
    
    vga_t_color = old_color;
}

void logiccircuit_draw_connection_info(LCGameSession* session) {
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    
    if (session->connection_state == CONNECTION_SELECTING_SOURCE) {
        vga_print_at("Select source pin: 1=Input1, 2=Input2, O=Output, ESC=Cancel", 10, 22);
    } else if (session->connection_state == CONNECTION_SELECTING_TARGET) {
        char msg[80];
        int pos = 0;
        const char* text = "Move to target gate (";
        for (int i = 0; text[i]; i++) msg[pos++] = text[i];
        
        if (session->turbo_mode) {
            msg[pos++] = 'T';
            msg[pos++] = 'U';
            msg[pos++] = 'R';
            msg[pos++] = 'B';
            msg[pos++] = 'O';
            msg[pos++] = ' ';
        } else {
            msg[pos++] = 'N';
            msg[pos++] = 'o';
            msg[pos++] = 'r';
            msg[pos++] = 'm';
            msg[pos++] = 'a';
            msg[pos++] = 'l';
            msg[pos++] = ' ';
        }
        
        const char* text2 = "speed), SPACE=Select, T=Toggle Turbo, ESC=Cancel";
        for (int i = 0; text2[i]; i++) msg[pos++] = text2[i];
        msg[pos] = '\0';
        
        vga_print_at(msg, 10, 22);
    } else if (session->connection_state == CONNECTION_SELECTING_PIN) {
        vga_print_at("Select target pin: 1=Input1, 2=Input2, O=Output, ESC=Cancel", 10, 22);
    }
}

void logiccircuit_draw_toolbox(LCGameSession* session) {
    int toolbox_x = GRID_OFFSET_X + GRID_WIDTH + 5;
    int toolbox_y = GRID_OFFSET_Y;
    
    draw_box(toolbox_x - 1, toolbox_y, 16, 20, VGA_LGREY, VGA_BLCK);
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_print_at("TOOLBOX", toolbox_x + 3, toolbox_y);

    GateType gate_types[] = {
        GATE_AND, GATE_OR, GATE_XOR, GATE_NOT,
        GATE_NAND, GATE_NOR, GATE_XNOR, GATE_BUFFER,
        GATE_INPUT, GATE_OUTPUT, GATE_SWITCH, GATE_LED
    };
    
    const char* gate_keys[] = {
        "1:AND", "2:OR", "3:XOR", "4:NOT",
        "5:NAND", "6:NOR", "7:XNOR", "8:BUF",
        "9:IN", "0:OUT", "S:SW", "L:LED"
    };
    
    for (int i = 0; i < 12; i++) {
        int y = toolbox_y + 2 + i;
        
        if (session->selected_gate_type == gate_types[i] && 
            session->current_tool == TOOL_PLACE_GATE) {
            vga_set_color(VGA_BLCK, VGA_CYAN);
            fill_rect(toolbox_x, y, 14, 1, ' ', VGA_BLCK, VGA_CYAN);
        } else {
            vga_set_color(VGA_WHITE, VGA_BLCK);
        }

        uint8_t old_color = vga_t_color;
        vga_set_color(gate_get_color(gate_types[i]), VGA_BLCK);
        vga_putchr_at(toolbox_x, y, gate_get_symbol(gate_types[i]));
        vga_t_color = old_color;
        
        vga_print_at(gate_keys[i], toolbox_x + 2, y);
    }

    vga_set_color(VGA_DGREY, VGA_BLCK);
    for (int i = 0; i < 14; i++) {
        vga_putchr_at(toolbox_x + i, toolbox_y + 14, 0xC4);
    }

    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_print_at("TOOLS", toolbox_x + 4, toolbox_y + 15);
    
    const char* tool_keys[] = {"Q:Select", "W:Wire", "E:Delete", "P:Place", "M:Move"};
    Tool tools[] = {TOOL_SELECT, TOOL_WIRE, TOOL_DELETE, TOOL_PLACE_GATE, TOOL_MOVE};
    
    for (int i = 0; i < 5; i++) {
        int y = toolbox_y + 16 + i;
        
        if (session->current_tool == tools[i]) {
            vga_set_color(VGA_BLCK, VGA_CYAN);
            fill_rect(toolbox_x, y, 14, 1, ' ', VGA_BLCK, VGA_CYAN);
        } else {
            vga_set_color(VGA_WHITE, VGA_BLCK);
        }
        
        vga_print_at(tool_keys[i], toolbox_x, y);
    }
}

void logiccircuit_draw_status(LCGameSession* session) {
    int status_x = 1;
    int status_y = 2;
    
    draw_box(status_x - 1, status_y - 1, 18, 12, VGA_LGREY, VGA_BLCK);
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_print_at("STATUS", status_x + 5, status_y);

    int active_gates = 0;
    for (int i = 0; i < MAX_GATES; i++) {
        if (session->gates[i].active) active_gates++;
    }
    
    vga_print_at("Gates:", status_x, status_y + 2);
    char buf[10];
    int pos = 0;
    if (active_gates >= 100) buf[pos++] = '0' + (active_gates / 100);
    if (active_gates >= 10) buf[pos++] = '0' + ((active_gates / 10) % 10);
    buf[pos++] = '0' + (active_gates % 10);
    buf[pos++] = '/';
    
    int max_gates = (session->state == CIRCUIT_PLAYING) ? 
                   session->current_level.max_gates : MAX_GATES;
    if (max_gates >= 100) buf[pos++] = '0' + (max_gates / 100);
    if (max_gates >= 10) buf[pos++] = '0' + ((max_gates / 10) % 10);
    buf[pos++] = '0' + (max_gates % 10);
    buf[pos] = '\0';
    vga_print_at(buf, status_x + 7, status_y + 2);

    vga_print_at("Wires:", status_x, status_y + 3);
    pos = 0;
    if (session->wire_count >= 100) buf[pos++] = '0' + (session->wire_count / 100);
    if (session->wire_count >= 10) buf[pos++] = '0' + ((session->wire_count / 10) % 10);
    buf[pos++] = '0' + (session->wire_count % 10);
    buf[pos++] = '/';
    
    if (MAX_WIRES >= 100) buf[pos++] = '0' + (MAX_WIRES / 100);
    if (MAX_WIRES >= 10) buf[pos++] = '0' + ((MAX_WIRES / 10) % 10);
    buf[pos++] = '0' + (MAX_WIRES % 10);
    buf[pos] = '\0';
    vga_print_at(buf, status_x + 7, status_y + 3);

    vga_print_at("Cycles:", status_x, status_y + 4);
    pos = 0;
    int cycles = session->cycles;
    if (cycles == 0) {
        buf[pos++] = '0';
    } else {
        int temp = cycles;
        int num_digits = 0;
        while (temp > 0) {
            temp /= 10;
            num_digits++;
        }
        temp = cycles;
        for (int i = num_digits - 1; i >= 0; i--) {
            buf[i] = '0' + (temp % 10);
            temp /= 10;
        }
        pos = num_digits;
    }
    buf[pos] = '\0';
    vga_print_at(buf, status_x + 8, status_y + 4);

    vga_print_at("Tool:", status_x, status_y + 6);
    const char* tool_name = "";
    switch (session->current_tool) {
        case TOOL_SELECT: tool_name = "Select"; break;
        case TOOL_WIRE: tool_name = "Wire"; break;
        case TOOL_DELETE: tool_name = "Delete"; break;
        case TOOL_PLACE_GATE: tool_name = "Place"; break;
        case TOOL_MOVE: tool_name = "Move"; break;
    }
    vga_set_color(VGA_CYAN, VGA_BLCK);
    vga_print_at(tool_name, status_x + 6, status_y + 6);

    if (session->current_tool == TOOL_PLACE_GATE) {
        vga_set_color(VGA_WHITE, VGA_BLCK);
        vga_print_at("Gate:", status_x, status_y + 7);
        vga_set_color(VGA_LGREEN, VGA_BLCK);
        vga_print_at(gate_get_name(session->selected_gate_type), status_x + 6, status_y + 7);
    }
    
    vga_set_color(VGA_WHITE, VGA_BLCK);

    if (session->state == CIRCUIT_PLAYING) {
        vga_print_at("LCLevel:", status_x, status_y + 9);
        pos = 0;
        int level = session->current_level_index + 1;
        buf[pos++] = '0' + level;
        buf[pos++] = '/';
        buf[pos++] = '0' + level_count;
        buf[pos] = '\0';
        vga_print_at(buf, status_x + 7, status_y + 9);
        
        vga_print_at("Diff:", status_x, status_y + 10);
        vga_set_color(VGA_YELLOW, VGA_BLCK);
        for (int i = 0; i < session->current_level.difficulty; i++) {
            vga_putchr_at(status_x + 6 + i, status_y + 10, '*');
        }
    }
}

void logiccircuit_draw_help(void) {
    vga_clear();
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    print_centered("LOGIC CIRCUIT - HELP", 1);
    
    draw_box(5, 3, 70, 18, VGA_LGREY, VGA_BLCK);
    
    int y = 5;
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_print_at("CONTROLS:", 8, y++);
    y++;
    
    const char* controls[] = {
        "ARROW KEYS: Move cursor",
        "SPACE: Primary action (place, select, etc.)",
        "F: Toggle fast movement mode",
        "ESC: Return to menu",
        "R: Run simulation",
        "C: Clear circuit",
        "X: Test level (in Play mode)",
        "T: Toggle input/switch gate",
        "1-9: Select gate types",
        "0: Output gate",
        "S: Switch gate",
        "L: LED gate",
        "Q: Select tool",
        "W: Wire tool (connect gates)",
        "E: Delete tool",
        "P: Place tool",
        "M: Move tool (move selected gate)"
    };
    
    for (int i = 0; i < 17; i++) {
        if (i < 8) {
            vga_set_color(VGA_LGREY, VGA_BLCK);
            vga_print_at(controls[i], 10, y + i);
        } else {
            vga_set_color(VGA_LGREY, VGA_BLCK);
            vga_print_at(controls[i], 45, y + i - 8);
        }
    }
    
    vga_set_color(VGA_DGREY, VGA_BLCK);
    print_centered("Press ESC or SPACE to return", 22);
}

void logiccircuit_draw_level_complete(LCGameSession* session) {
    vga_clear();
    
    draw_box(10, 5, 60, 15, VGA_YELLOW, VGA_BLCK);
    
    vga_set_color(VGA_LGREEN, VGA_BLCK);
    print_centered("LEVEL COMPLETE!", 8);
    
    vga_set_color(VGA_WHITE, VGA_BLCK);

    char msg[60];
    int pos = 0;
    const char* prefix = "LCLevel ";
    for (int i = 0; prefix[i]; i++) msg[pos++] = prefix[i];
    msg[pos++] = '0' + (session->current_level_index + 1);
    msg[pos++] = ':';
    msg[pos++] = ' ';
    
    const char* name = levels[session->current_level_index].name;
    for (int i = 0; name[i] && pos < 59; i++) {
        msg[pos++] = name[i];
    }
    msg[pos] = '\0';
    
    print_centered(msg, 10);

    vga_set_color(VGA_LCYAN, VGA_BLCK);
    print_centered("Statistics:", 12);
    
    char stats[40];
    vga_set_color(VGA_LGREY, VGA_BLCK);

    pos = 0;
    const char* label = "Gates used: ";
    for (int i = 0; label[i]; i++) stats[pos++] = label[i];
    
    int gates = 0;
    for (int i = 0; i < MAX_GATES; i++) {
        if (session->gates[i].active) gates++;
    }
    
    if (gates >= 10) stats[pos++] = '0' + (gates / 10);
    stats[pos++] = '0' + (gates % 10);
    stats[pos] = '\0';
    print_centered(stats, 14);

    pos = 0;
    label = "Wires used: ";
    for (int i = 0; label[i]; i++) stats[pos++] = label[i];
    
    if (session->wire_count >= 10) stats[pos++] = '0' + (session->wire_count / 10);
    stats[pos++] = '0' + (session->wire_count % 10);
    stats[pos] = '\0';
    print_centered(stats, 15);

    if (session->current_level_index < level_count - 1) {
        vga_set_color(VGA_YELLOW, VGA_BLCK);
        print_centered("Press ENTER for next level", 18);
    } else {
        vga_set_color(VGA_LGREEN, VGA_BLCK);
        print_centered("All levels completed!", 18);
    }
    
    vga_set_color(VGA_DGREY, VGA_BLCK);
    print_centered("Press ESC to return to menu", 20);
}


void logiccircuit_save_progress(LCGameSession* session) {
    typedef struct {
        int magic_number;     
        LCGameSession ssession;
        int version;           
        int checksum;
    } GameLogicSaveData;
    
    GameLogicSaveData save_data;
    save_data.magic_number = 0x15151515;  
    save_data.ssession = *session;
    save_data.version = 1;
    
    save_data.checksum = save_data.magic_number + 
                         save_data.ssession.cycles + 
                         save_data.version;
    
    fat16_write_file(GAME_LOGIC_SAVE_FILE, 
                     (const char*)&save_data, 
                     sizeof(GameLogicSaveData));
}

void logiccircuit_load_progress(LCGameSession* session) {
    if (!fat16_file_exists(GAME_LOGIC_SAVE_FILE)) {
        for (int i = 0; i < MAX_LEVELS; i++) {
            session->levels_completed[i] = 0;
        }
        session->levels_solved = 0;
        session->total_gates_placed = 0;
        session->total_wires_placed = 0;
        return;
    }
    
    uint32_t file_size;
    char* file_content = fat16_read_file(GAME_LOGIC_SAVE_FILE, &file_size);
    
    if (!file_content || file_size == 0) {
        for (int i = 0; i < MAX_LEVELS; i++) {
            session->levels_completed[i] = 0;
        }
        session->levels_solved = 0;
        session->total_gates_placed = 0;
        session->total_wires_placed = 0;
        return;
    }
    
    typedef struct {
        int magic_number;     
        LCGameSession ssession;
        int version;           
        int checksum;
    } GameLogicSaveData;
    
    if (file_size < sizeof(GameLogicSaveData)) {
        for (int i = 0; i < MAX_LEVELS; i++) {
            session->levels_completed[i] = 0;
        }
        session->levels_solved = 0;
        session->total_gates_placed = 0;
        session->total_wires_placed = 0;
        return;
    }
    
    GameLogicSaveData* save_data = (GameLogicSaveData*)file_content;
    
    if (save_data->magic_number != 0x15151515) {
        for (int i = 0; i < MAX_LEVELS; i++) {
            session->levels_completed[i] = 0;
        }
        session->levels_solved = 0;
        session->total_gates_placed = 0;
        session->total_wires_placed = 0;
        return;
    }
    
    int calculated_checksum = save_data->magic_number + 
                             save_data->ssession.cycles + 
                             save_data->version;
    
    if (calculated_checksum != save_data->checksum) {
        for (int i = 0; i < MAX_LEVELS; i++) {
            session->levels_completed[i] = 0;
        }
        session->levels_solved = 0;
        session->total_gates_placed = 0;
        session->total_wires_placed = 0;
        return;
    }
    
    *session = save_data->ssession;

}


void logiccircuit_game_init(void) {
    level_count = 0;
    
    memset(&session, 0, sizeof(LCGameSession));
    
    vga_init();
    keyboard_init();
    pit_init(1000);

    session.state = CIRCUIT_MENU;
    session.current_tool = TOOL_PLACE_GATE;
    session.selected_gate_type = GATE_AND;
    session.cursor_x = GRID_WIDTH / 2;
    session.cursor_y = GRID_HEIGHT / 2;
    session.menu_selection = 0;
    session.current_level_index = 0;
    session.levels_solved = 0;
    session.connection_state = CONNECTION_NONE;
    session.selected_gate_id = -1;
    session.last_input_time = 0;
    session.input_repeat_delay = 100;
    session.fast_repeat_mode = 0;
    session.turbo_mode = 0;

    circuit_init(&session);

    logiccircuit_init_levels();

    logiccircuit_load_progress(&session);

    vga_enable_double_buffer(1);
    vga_set_auto_swap(1);

    if (level_count == 0) {
        level_count = 0;
        logiccircuit_init_levels();
    }
}

void run_logiccircuit_game(void) {
    logiccircuit_game_init();
    logiccircuit_game_run();
    logiccircuit_game_cleanup();
}

void logiccircuit_game_run(void) {
    int running = 1;
    
    while (running) {
        logiccircuit_update_input_timing(&session);
        
        if (session.state == CIRCUIT_EXIT) {
            running = 0;
            continue;
        }
        
        vga_begin_batch();
        
        switch (session.state) {
            case CIRCUIT_MENU:
                logiccircuit_draw_menu(&session);
                break;
            case CIRCUIT_LEVEL_SELECT:
                logiccircuit_draw_level_select(&session);
                break;
            case CIRCUIT_PLAYING:
            case CIRCUIT_SANDBOX:
                logiccircuit_draw_game(&session);
                break;
            case CIRCUIT_HELP:
                logiccircuit_draw_help();
                break;
            case CIRCUIT_LEVEL_COMPLETE:
                logiccircuit_draw_level_complete(&session);
                break;
        }
        
        vga_end_batch();

        pit_delay_ms(33); 
    }
}

void logiccircuit_game_cleanup(void) {
    logiccircuit_save_progress(&session);
    vga_clear();
    vga_reset_color();
}