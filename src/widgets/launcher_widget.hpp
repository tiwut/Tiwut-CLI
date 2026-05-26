#pragma once
#include <curses.h>
#include <string>
#include <vector>


struct AppIcon {
    std::string name;
    std::string icon;
    std::string exec;
};

extern std::vector<AppIcon> loaded_apps;
extern int launcher_idx;


void load_desktop_apps();
void draw_launcher_widget(WINDOW* win, bool focused);
