#pragma once
#include <string>
#include <vector>
#include <curses.h>


struct Theme {
    std::string name;
    short border_active;
    short border_inactive;
    short header_fg;
    short accent;
    short success;
};


extern const std::vector<Theme> themes;
extern int current_theme;
extern int active_pane;
extern bool maximized;

extern int active_menu;
extern int menu_idx;


void init_theme_colors();
