#pragma once
#include <curses.h>
#include <string>
#include <vector>


extern std::vector<std::string> notepad_lines;
extern int notepad_y;
extern int notepad_x;
extern std::string notes_file_path;


void load_notes();
void save_notes();
void draw_notepad_widget(WINDOW* win, bool focused);
