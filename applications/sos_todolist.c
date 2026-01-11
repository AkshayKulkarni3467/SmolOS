#include "sos_todolist.h"
#include "sos_vga.h"
#include "sos_vgraphics.h"
#include "sos_keyboard.h"
#include "sos_fat16.h"
#include "sos_string.h"
#include "sos_memory.h"
#include "sos_pit.h"

#define TODO_FILENAME "TODOS.DAT"

static TodoList todo_list;

static const uint8_t priority_colors[] = {
    VGA_DGREY,   
    VGA_YELLOW, 
    VGA_LRED,    
    VGA_RED      
};

static const char* priority_names[] = {
    "Low",
    "Medium",
    "High",
    "URGENT"
};

static void todo_save(void) {
    fat16_init();
    
    int data_size = sizeof(int) * 6 + (todo_list.count * sizeof(TodoItem));
    
    char buffer[8192];
    int pos = 0;
    
    memcpy(buffer + pos, &todo_list.count, sizeof(int));
    pos += sizeof(int);
    
    for (int i = 0; i < todo_list.count; i++) {
        memcpy(buffer + pos, &todo_list.items[i], sizeof(TodoItem));
        pos += sizeof(TodoItem);
    }
    
    fat16_write_file(TODO_FILENAME, buffer, pos);
    todo_list.modified = 0;
}

static void todo_load(void) {
    fat16_init();
    
    memset(&todo_list, 0, sizeof(TodoList));
    todo_list.filter_priority = -1;
    todo_list.filter_show_completed = 1;
    
    if (fat16_file_exists(TODO_FILENAME)) {
        uint32_t size;
        char* data = fat16_read_file(TODO_FILENAME, &size);
        
        if (data && size >= sizeof(int)) {
            int pos = 0;
            
            memcpy(&todo_list.count, data + pos, sizeof(int));
            pos += sizeof(int);
            
            if (todo_list.count > MAX_TODOS) {
                todo_list.count = MAX_TODOS;
            }
            
            for (int i = 0; i < todo_list.count && pos + sizeof(TodoItem) <= size; i++) {
                memcpy(&todo_list.items[i], data + pos, sizeof(TodoItem));
                pos += sizeof(TodoItem);
            }
            
            todo_list.modified = 0;
        }
    }
}

static void todo_add_item(const char* text, const char* category, TodoPriority priority) {
    if (todo_list.count >= MAX_TODOS) return;
    
    TodoItem* item = &todo_list.items[todo_list.count];
    
    int i;
    for (i = 0; i < TODO_TEXT_LENGTH - 1 && text[i]; i++) {
        item->text[i] = text[i];
    }
    item->text[i] = '\0';
    
    for (i = 0; i < TODO_CATEGORY_LENGTH - 1 && category[i]; i++) {
        item->category[i] = category[i];
    }
    item->category[i] = '\0';
    
    item->priority = priority;
    item->completed = 0;
    item->created_timestamp = pit_get_seconds();
    item->completed_timestamp = 0;
    
    todo_list.count++;
    todo_list.modified = 1;
}

static void todo_toggle_complete(int index) {
    if (index < 0 || index >= todo_list.count) return;
    
    todo_list.items[index].completed = !todo_list.items[index].completed;
    
    if (todo_list.items[index].completed) {
        todo_list.items[index].completed_timestamp = pit_get_seconds();
    } else {
        todo_list.items[index].completed_timestamp = 0;
    }
    
    todo_list.modified = 1;
}

static void todo_delete_item(int index) {
    if (index < 0 || index >= todo_list.count) return;
    
    for (int i = index; i < todo_list.count - 1; i++) {
        memcpy(&todo_list.items[i], &todo_list.items[i + 1], sizeof(TodoItem));
    }
    
    todo_list.count--;
    
    if (todo_list.selected >= todo_list.count && todo_list.count > 0) {
        todo_list.selected = todo_list.count - 1;
    }
    
    todo_list.modified = 1;
}

static int todo_matches_filter(const TodoItem* item) {
    if (!todo_list.filter_show_completed && item->completed) {
        return 0;
    }
    
    if (todo_list.filter_priority >= 0 && item->priority != todo_list.filter_priority) {
        return 0;
    }
    
    if (todo_list.filter_category[0] != '\0') {
        if (strcmp(item->category, todo_list.filter_category) != 0) {
            return 0;
        }
    }
    
    return 1;
}

static void todo_get_stats(int* total, int* completed, int* urgent) {
    *total = 0;
    *completed = 0;
    *urgent = 0;
    
    for (int i = 0; i < todo_list.count; i++) {
        (*total)++;
        if (todo_list.items[i].completed) (*completed)++;
        if (todo_list.items[i].priority == PRIORITY_URGENT && !todo_list.items[i].completed) {
            (*urgent)++;
        }
    }
}

static void todo_draw_screen(void) {
    vga_begin_batch();
    vga_clear();
    
    vga_fill_rect(0, 0, 80, 2, ' ', VGA_BLCK, VGA_CYAN);
    vga_set_color(VGA_YELLOW, VGA_CYAN);
    vga_print_centered("SmolOS TODO List Manager", 0);
    
    int total, completed, urgent;
    todo_get_stats(&total, &completed, &urgent);
    
    vga_set_color(VGA_WHITE, VGA_CYAN);
    char stats[80];
    int pos = 0;
    
    const char* total_label = "Total: ";
    for (int i = 0; total_label[i]; i++) stats[pos++] = total_label[i];
    if (total >= 10) stats[pos++] = '0' + (total / 10);
    stats[pos++] = '0' + (total % 10);
    stats[pos++] = ' ';
    stats[pos++] = ' ';
    
    const char* done_label = "Done: ";
    for (int i = 0; done_label[i]; i++) stats[pos++] = done_label[i];
    if (completed >= 10) stats[pos++] = '0' + (completed / 10);
    stats[pos++] = '0' + (completed % 10);
    stats[pos++] = ' ';
    stats[pos++] = ' ';
    
    const char* urgent_label = "Urgent: ";
    for (int i = 0; urgent_label[i]; i++) stats[pos++] = urgent_label[i];
    if (urgent >= 10) stats[pos++] = '0' + (urgent / 10);
    stats[pos++] = '0' + (urgent % 10);
    
    stats[pos] = '\0';
    vga_print_centered(stats, 1);
    
    vga_set_color(VGA_DGREY, VGA_BLCK);
    for (int x = 0; x < 80; x++) {
        vga_putchr_at(x, 2, 0xC4);
    }
    
    if (todo_list.filter_priority >= 0 || todo_list.filter_category[0] != '\0' || 
        !todo_list.filter_show_completed) {
        vga_set_color(VGA_YELLOW, VGA_BLCK);
        vga_print_centered("[FILTERS ACTIVE - Press F to clear]", 3);
    }
    
    int display_row = 4;
    int visible_count = 0;
    
    for (int i = 0; i < todo_list.count && display_row < 22; i++) {
        TodoItem* item = &todo_list.items[i];
        
        if (!todo_matches_filter(item)) continue;
        
        if (visible_count < todo_list.scroll_offset) {
            visible_count++;
            continue;
        }
        
        int is_selected = (i == todo_list.selected);
        
        if (is_selected) {
            vga_fill_rect(0, display_row, 80, 1, ' ', VGA_WHITE, VGA_BLUE);
        }
        
        vga_set_color(is_selected ? VGA_YELLOW : VGA_WHITE, 
                      is_selected ? VGA_BLUE : VGA_BLCK);
        vga_putchr_at(2, display_row, '[');
        
        if (item->completed) {
            vga_set_color(VGA_LGREEN, is_selected ? VGA_BLUE : VGA_BLCK);
            vga_putchr_at(3, display_row, 0xFB);
        } else {
            vga_putchr_at(3, display_row, ' ');
        }
        
        vga_set_color(is_selected ? VGA_YELLOW : VGA_WHITE, 
                      is_selected ? VGA_BLUE : VGA_BLCK);
        vga_putchr_at(4, display_row, ']');
        
        vga_set_color(priority_colors[item->priority], 
                      is_selected ? VGA_BLUE : VGA_BLCK);
        vga_putchr_at(6, display_row, 0x07);
        
        uint8_t text_color = item->completed ? VGA_DGREY : 
                            (is_selected ? VGA_WHITE : VGA_LGREY);
        vga_set_color(text_color, is_selected ? VGA_BLUE : VGA_BLCK);
        
        int x = 8;
        for (int j = 0; j < TODO_TEXT_LENGTH && item->text[j] && x < 62; j++) {
            vga_putchr_at(x++, display_row, item->text[j]);
        }
        
        if (item->category[0] != '\0') {
            vga_set_color(VGA_CYAN, is_selected ? VGA_BLUE : VGA_BLCK);
            vga_putchr_at(64, display_row, '[');
            x = 65;
            for (int j = 0; item->category[j] && x < 77; j++) {
                vga_putchr_at(x++, display_row, item->category[j]);
            }
            vga_putchr_at(x, display_row, ']');
        }
        
        display_row++;
        visible_count++;
    }
    
    if (todo_list.count == 0) {
        vga_set_color(VGA_DGREY, VGA_BLCK);
        vga_print_centered("No tasks yet! Press 'A' to add your first task.", 12);
    } else if (visible_count == 0) {
        vga_set_color(VGA_DGREY, VGA_BLCK);
        vga_print_centered("No tasks match current filters.", 12);
    }
    
    vga_set_color(VGA_DGREY, VGA_BLCK);
    for (int x = 0; x < 80; x++) {
        vga_putchr_at(x, 22, 0xC4);
    }
    
    vga_fill_rect(0, 23, 80, 2, ' ', VGA_WHITE, VGA_DGREY);
    vga_set_color(VGA_YELLOW, VGA_DGREY);
    
    vga_putchr_at(1, 23, 0);
    vga_print("A");
    vga_set_color(VGA_BLCK, VGA_DGREY);
    vga_print(":Add ");
    
    vga_set_color(VGA_YELLOW, VGA_DGREY);
    vga_print("SPACE");
    vga_set_color(VGA_BLCK, VGA_DGREY);
    vga_print(":Toggle ");
    
    vga_set_color(VGA_YELLOW, VGA_DGREY);
    vga_print("E");
    vga_set_color(VGA_BLCK, VGA_DGREY);
    vga_print(":Edit ");
    
    vga_set_color(VGA_YELLOW, VGA_DGREY);
    vga_print("D");
    vga_set_color(VGA_BLCK, VGA_DGREY);
    vga_print(":Del ");
    
    vga_set_color(VGA_YELLOW, VGA_DGREY);
    vga_print("F");
    vga_set_color(VGA_BLCK, VGA_DGREY);
    vga_print(":Filter");
    
    vga_putchr_at(1, 24, 0);
    vga_set_color(VGA_YELLOW, VGA_DGREY);
    vga_print("S");
    vga_set_color(VGA_BLCK, VGA_DGREY);
    vga_print(":Save ");
    
    vga_set_color(VGA_YELLOW, VGA_DGREY);
    vga_print("C");
    vga_set_color(VGA_BLCK, VGA_DGREY);
    vga_print(":Clear Done ");
    
    vga_set_color(VGA_YELLOW, VGA_DGREY);
    vga_print("ESC");
    vga_set_color(VGA_BLCK, VGA_DGREY);
    vga_print(":Exit");
    
    if (todo_list.modified) {
        vga_set_color(VGA_LRED, VGA_DGREY);
        vga_print_centered("*UNSAVED*", 24);
    }
    
    vga_end_batch();
}

static void todo_edit_dialog(int edit_index) {
    char task_text[TODO_TEXT_LENGTH] = {0};
    char category[TODO_CATEGORY_LENGTH] = {0};
    int priority = PRIORITY_MEDIUM;
    
    if (edit_index >= 0 && edit_index < todo_list.count) {
        strcpy(task_text, todo_list.items[edit_index].text);
        strcpy(category, todo_list.items[edit_index].category);
        priority = todo_list.items[edit_index].priority;
    }
    
    vga_begin_batch();
    
    vga_fill_rect(10, 8, 60, 10, ' ', VGA_WHITE, VGA_BLUE);
    vga_draw_box_double(10, 8, 60, 10, VGA_CYAN, VGA_BLUE);
    
    vga_set_color(VGA_YELLOW, VGA_BLUE);
    if (edit_index >= 0) {
        vga_print_centered("Edit Task", 9);
    } else {
        vga_print_centered("Add New Task", 9);
    }
    
    vga_set_color(VGA_WHITE, VGA_BLUE);
    print_at(12, 11, "Task:", VGA_LGREY, VGA_BLUE);
    print_at(12, 12, "Category:", VGA_LGREY, VGA_BLUE);
    print_at(12, 13, "Priority:", VGA_LGREY, VGA_BLUE);
    
    if (edit_index >= 0) {
        print_at(23, 11, task_text, VGA_WHITE, VGA_BLUE);
        print_at(23, 12, category, VGA_WHITE, VGA_BLUE);
    }
    
    vga_end_batch();
    
    int task_pos = strlen(task_text);
    
    while (1) {
        vga_begin_batch();
        vga_fill_rect(23, 11, 40, 1, ' ', VGA_WHITE, VGA_BLUE);
        print_at(23, 11, task_text, VGA_WHITE, VGA_BLUE);
        vga_end_batch();
        
        vga_t_row = 11;
        vga_t_column = 23 + task_pos;
        vga_setcursor(23 + task_pos, 11);
        
        keyboard_poll();
        if (has_key()) {
            char c = get_char();
            
            if (c == '\n') break;
            else if (c == 27) return;
            else if (c == '\b' && task_pos > 0) {
                task_pos--;
                task_text[task_pos] = '\0';
            }
            else if (is_printable(c) && task_pos < TODO_TEXT_LENGTH - 1) {
                task_text[task_pos++] = c;
                task_text[task_pos] = '\0';
            }
        }
        pit_delay_ms(50);
    }
    
    if (task_pos == 0) return;
    
    int cat_pos = strlen(category);
    
    while (1) {
        vga_begin_batch();
        vga_fill_rect(23, 12, 40, 1, ' ', VGA_WHITE, VGA_BLUE);
        print_at(23, 12, category, VGA_WHITE, VGA_BLUE);
        vga_end_batch();
        
        vga_t_row = 12;
        vga_t_column = 23 + cat_pos;
        vga_setcursor(23 + cat_pos, 12);
        
        keyboard_poll();
        if (has_key()) {
            char c = get_char();
            
            if (c == '\n') break;
            else if (c == 27) return;
            else if (c == '\b' && cat_pos > 0) {
                cat_pos--;
                category[cat_pos] = '\0';
            }
            else if (is_printable(c) && cat_pos < TODO_CATEGORY_LENGTH - 1) {
                category[cat_pos++] = c;
                category[cat_pos] = '\0';
            }
        }
        pit_delay_ms(50);
    }
    
    int selecting = 1;
    
    while (selecting) {
        vga_begin_batch();
        vga_fill_rect(23, 13, 50, 1, ' ', VGA_WHITE, VGA_BLUE);
        
        for (int p = 0; p < 4; p++) {
            int x_offset = 23 + (p * 11);
            
            if (p == priority) {
                vga_set_color(VGA_BLCK, VGA_CYAN);
                print_at(x_offset, 13, "[", VGA_BLCK, VGA_CYAN);
                print_at(x_offset + 1, 13, priority_names[p], VGA_BLCK, VGA_CYAN);
                vga_putchr_at(x_offset + 1 + strlen(priority_names[p]), 13, ']');
            } else {
                print_at(x_offset, 13, priority_names[p], priority_colors[p], VGA_BLUE);
            }
        }
        
        vga_end_batch();
        
        keyboard_poll();
        if (has_key()) {
            char c = get_char();
            
            if (c == '\n' || c == ' ') {
                selecting = 0;
            } else if (c == 27) {
                return;
            } else if (c == CHAR_LEFT && priority > 0) {
                priority--;
            } else if (c == CHAR_RIGHT && priority < 3) {
                priority++;
            }
        }
        
        pit_delay_ms(50);
    }
    
    if (edit_index >= 0) {
        strcpy(todo_list.items[edit_index].text, task_text);
        strcpy(todo_list.items[edit_index].category, category);
        todo_list.items[edit_index].priority = priority;
        todo_list.modified = 1;
    } else {
        todo_add_item(task_text, category, priority);
    }
}

static void todo_filter_dialog(void) {
    int selected_option = 0;
    int running = 1;
    
    while (running) {
        vga_begin_batch();
        
        vga_fill_rect(15, 9, 50, 11, ' ', VGA_WHITE, VGA_BLUE);
        vga_draw_box_double(15, 9, 50, 11, VGA_CYAN, VGA_BLUE);
        
        vga_set_color(VGA_YELLOW, VGA_BLUE);
        vga_print_centered("Filter Options", 10);
        
        const char* options[] = {
            "Clear all filters",
            "Show completed tasks",
            "Hide completed tasks",
            "Filter by priority",
            "Back"
        };
        
        for (int i = 0; i < 5; i++) {
            int y = 12 + i;
            
            if (i == selected_option) {
                vga_fill_rect(17, y, 46, 1, ' ', VGA_BLCK, VGA_CYAN);
                print_at(19, y, ">", VGA_YELLOW, VGA_CYAN);
                print_at(21, y, options[i], VGA_YELLOW, VGA_CYAN);
            } else {
                print_at(21, y, options[i], VGA_WHITE, VGA_BLUE);
            }
        }
        
        vga_end_batch();
        
        keyboard_poll();
        if (has_key()) {
            char c = get_char();
            
            if (c == CHAR_UP && selected_option > 0) {
                selected_option--;
            } else if (c == CHAR_DOWN && selected_option < 4) {
                selected_option++;
            } else if (c == '\n' || c == ' ') {
                if (selected_option == 0) {
                    todo_list.filter_priority = -1;
                    todo_list.filter_show_completed = 1;
                    todo_list.filter_category[0] = '\0';
                    running = 0;
                } else if (selected_option == 1) {
                    todo_list.filter_show_completed = 1;
                    running = 0;
                } else if (selected_option == 2) {
                    todo_list.filter_show_completed = 0;
                    running = 0;
                } else if (selected_option == 3) {
                    int p = 0;
                    int selecting = 1;
                    
                    while (selecting) {
                        vga_begin_batch();
                        vga_fill_rect(20, 15, 40, 1, ' ', VGA_WHITE, VGA_BLUE);
                        
                        for (int i = 0; i < 4; i++) {
                            int x = 22 + (i * 10);
                            if (i == p) {
                                print_at(x, 15, priority_names[i], VGA_BLCK, VGA_CYAN);
                            } else {
                                print_at(x, 15, priority_names[i], priority_colors[i], VGA_BLUE);
                            }
                        }
                        vga_end_batch();
                        
                        keyboard_poll();
                        if (has_key()) {
                            char c2 = get_char();
                            if (c2 == CHAR_LEFT && p > 0) p--;
                            else if (c2 == CHAR_RIGHT && p < 3) p++;
                            else if (c2 == '\n') {
                                todo_list.filter_priority = p;
                                selecting = 0;
                                running = 0;
                            } else if (c2 == 27) {
                                selecting = 0;
                            }
                        }
                        pit_delay_ms(50);
                    }
                } else {
                    running = 0;
                }
            } else if (c == 27) {
                running = 0;
            }
        }
        
        pit_delay_ms(50);
    }
}

void cmd_todo(void) {
    todo_load();
    
    int running = 1;
    
    while (running) {
        todo_draw_screen();
        
        keyboard_poll();
        if (has_key()) {
            char c = get_char();
            
            if (c == 27) {
                if (todo_list.modified) {
                    vga_begin_batch();
                    vga_fill_rect(20, 10, 40, 5, ' ', VGA_WHITE, VGA_RED);
                    vga_draw_box_single(20, 10, 40, 5, VGA_YELLOW, VGA_RED);
                    vga_set_color(VGA_WHITE, VGA_RED);
                    vga_print_centered("You have unsaved changes!", 11);
                    vga_print_centered("Save before exit? (Y/N)", 12);
                    vga_end_batch();
                    
                    char confirm = wait_for_char();
                    if (confirm == 'y' || confirm == 'Y') {
                        todo_save();
                    }
                }
                running = 0;
            }
            else if (c == 'a' || c == 'A') {
                todo_edit_dialog(-1);  
            }
            else if (c == 'e' || c == 'E') {
                if (todo_list.count > 0) {
                    todo_edit_dialog(todo_list.selected);  
                }
            }
            else if (c == ' ') {
                if (todo_list.count > 0) {
                    todo_toggle_complete(todo_list.selected);
                }
            }
            else if (c == 'd' || c == 'D') {
                if (todo_list.count > 0) {
                    todo_delete_item(todo_list.selected);
                }
            }
            else if (c == 's' || c == 'S') {
                todo_save();
            }
            else if (c == 'f' || c == 'F') {
                todo_filter_dialog();
            }
            else if (c == 'c' || c == 'C') {
                for (int i = todo_list.count - 1; i >= 0; i--) {
                    if (todo_list.items[i].completed) {
                        todo_delete_item(i);
                    }
                }
            }
            else if (c == CHAR_UP && todo_list.selected > 0) {
                todo_list.selected--;
                if (todo_list.selected < todo_list.scroll_offset) {
                    todo_list.scroll_offset--;
                }
            }
            else if (c == CHAR_DOWN && todo_list.selected < todo_list.count - 1) {
                todo_list.selected++;
                if (todo_list.selected >= todo_list.scroll_offset + 18) {
                    todo_list.scroll_offset++;
                }
            }
        }
        
        pit_delay_ms(50);
    }
    
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
}