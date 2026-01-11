#ifndef CALCULATOR_H
#define CALCULATOR_H

#include "sos_stdint.h"


double power(double base, double exp);
double sqrt(double x);
double factorial(int n);
double sine(double x);
double cosine(double x);
double tangent(double x);
double logarithm(double x);

double deg_to_rad(double deg);
double rad_to_deg(double rad);

double string_to_double(const char* str);
void double_to_string(double num, char* buffer);

void add_to_history(const char* expression);

void print_at(int x, int y, const char* str, uint8_t fg, uint8_t bg);
void show_calculation_animation(void);
void draw_calculator_ui(void);
void show_help(void);
double evaluate_expression(const char* expr);
void calc_command(void);

#endif
