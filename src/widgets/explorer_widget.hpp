#pragma once
#include <curses.h>
#include <string>
#include <vector>


extern std::string explorer_dir;
extern int explorer_idx;
extern std::vector<std::string> explorer_files;


void load_explorer_files();
void draw_explorer_widget(WINDOW* win, bool focused);
void view_file_modal(WINDOW* parent, const std::string& file_path);
void show_system_info_modal(WINDOW* parent);
