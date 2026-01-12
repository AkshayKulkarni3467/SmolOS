#ifndef INCLUDE_SMOLOS_TODOLIST_H
#define INCLUDE_SMOLOS_TODOLIST_H

#include "sos_stdint.h"

#define MAX_TODOS 50
#define TODO_TEXT_LENGTH 60
#define TODO_CATEGORY_LENGTH 16

typedef enum {
    PRIORITY_LOW = 0,
    PRIORITY_MEDIUM = 1,
    PRIORITY_HIGH = 2,
    PRIORITY_URGENT = 3
} TodoPriority;

typedef struct {
    char text[TODO_TEXT_LENGTH];
    char category[TODO_CATEGORY_LENGTH];
    TodoPriority priority;
    uint8_t completed;
    uint32_t created_timestamp;
    uint32_t completed_timestamp;
} TodoItem;

typedef struct {
    TodoItem items[MAX_TODOS];
    int count;
    int selected;
    int scroll_offset;
    int modified;
    int filter_priority;
    int filter_show_completed;
    char filter_category[TODO_CATEGORY_LENGTH];
} TodoList;

void cmd_todo(void);

#endif // INCLUDE_SMOLOS_TODOLIST_H