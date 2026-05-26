#include "notepad_widget.hpp"
#include "../globals.hpp"
#include "../utils.hpp"
#include <fstream>
#include <algorithm>


std::vector<std::string> notepad_lines = {""};
int notepad_y = 0;
int notepad_x = 0;
std::string notes_file_path;

void load_notes() {
    char* home = getenv("HOME");
    std::string app_dir = home ? std::string(home) + "/.local/share/tiwut-cli" : ".";
    notes_file_path = app_dir + "/notes.txt";
    
    std::ifstream f(notes_file_path);
    if (f.is_open()) {
        notepad_lines.clear();
        std::string line;
        while (std::getline(f, line)) {
            notepad_lines.push_back(line);
        }
        if (notepad_lines.empty()) {
            notepad_lines.push_back("");
        }
    } else {
        notepad_lines = {
            "Welcome to Tiwut Scratch Pad!",
            "Type notes here... they save auto-magically!",
            "Press TAB to cycle focused widgets.",
            "Press F1-F3 to explore real GUI-like drop down menus!"
        };
    }
    notepad_y = notepad_lines.size() - 1;
    notepad_x = notepad_lines[notepad_y].length();
}

void save_notes() {
    std::ofstream f(notes_file_path);
    if (f.is_open()) {
        for (size_t i = 0; i < notepad_lines.size(); ++i) {
            f << notepad_lines[i];
            if (i + 1 < notepad_lines.size()) f << "\n";
        }
    }
}

void draw_notepad_widget(WINDOW* win, bool focused) {
    draw_box(win, "TIWUT SCRATCHPAD", focused);
    int h, w;
    getmaxyx(win, h, w);
    
    int vis_h = h - 2;
    int vis_w = w - 4;
    
    int start_y = std::max(0, notepad_y - vis_h + 1);
    
    for (int i = 0; i < vis_h; ++i) {
        int idx = start_y + i;
        if (idx < (int)notepad_lines.size()) {
            safe_addstr(win, i + 1, 2, notepad_lines[idx], COLOR_PAIR(2));
        }
    }
    
    
    if ((int)notepad_lines.size() > vis_h) {
        int scroll_h = h - 4;
        int scroll_y = 2;
        int scroll_x = w - 2;
        
        for (int i = 0; i < scroll_h; ++i) {
            safe_addstr(win, scroll_y + i, scroll_x, "|", COLOR_PAIR(2));
        }
        
        int slider_h = std::max(1, (vis_h * scroll_h) / (int)notepad_lines.size());
        int slider_pos = (start_y * (scroll_h - slider_h)) / ((int)notepad_lines.size() - vis_h);
        
        for (int i = 0; i < slider_h; ++i) {
            safe_addstr(win, scroll_y + slider_pos + i, scroll_x, "#", COLOR_PAIR(1) | A_BOLD);
        }
    }
    
    if (focused) {
        int cur_row = 1 + (notepad_y - start_y);
        int cur_col = 2 + notepad_x;
        if (cur_row >= 1 && cur_row < h - 1 && cur_col >= 2 && cur_col < w - 2) {
            wmove(win, cur_row, cur_col);
            curs_set(1);
        } else {
            curs_set(0);
        }
    }
}
