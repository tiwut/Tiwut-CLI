#pragma once
#include <string>
#include <vector>
#include <curses.h>


extern const std::vector<std::string> embedded_logo;


std::string get_basename(const std::string& path);
void safe_addstr(WINDOW* win, int y, int x, const std::string& text, int attr = 0);
void draw_box(WINDOW* win, const std::string& title, bool focused);
std::vector<std::string> read_logo();
void log_debug(const std::string& msg);


void show_modal(WINDOW* parent, const std::string& title, const std::vector<std::string>& content_lines);
