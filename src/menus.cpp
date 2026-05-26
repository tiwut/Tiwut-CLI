#include "menus.hpp"
#include "globals.hpp"
#include "utils.hpp"
#include <vector>
#include <algorithm>

void draw_top_menus(WINDOW* parent) {
    if (active_menu == 0) return;
    
    std::vector<std::string> items;
    int start_x = 0;
    
    if (active_menu == 1) { 
        items = {
            " 1. Preview Selected File ",
            " 2. Shell Environment Info",
            " 3. Reset Scratchpad      ",
            " 4. Shutdown Desktop (Q)  "
        };
        start_x = 12;
    } else if (active_menu == 2) { 
        items = {
            " 1. Cycle UI Theme (C)    ",
            " 2. Toggle Full Screen (M)",
            " 3. Reset UI Layout       "
        };
        start_x = 24;
    } else if (active_menu == 3) { 
        items = {
            " 1. Open Website          ",
            " 2. Open GitHub           ",
            " 3. Core Credits          "
        };
        start_x = 37;
    } else if (active_menu == 4) { 
        items = {
            " 1. Open Interactive Shell ",
            " 2. Fetch GitHub Repos     ",
            " 3. Open App Store         "
        };
        start_x = 51;
    }
    
    int menu_h = items.size() + 2;
    int menu_w = 28;
    
    WINDOW* menu_win = derwin(parent, menu_h, menu_w, 2, start_x);
    if (menu_win) {
        wattron(menu_win, COLOR_PAIR(1));
        box(menu_win, 0, 0);
        wattroff(menu_win, COLOR_PAIR(1));
        
        for (size_t idx = 0; idx < items.size(); ++idx) {
            if ((int)idx == menu_idx) {
                wattron(menu_win, COLOR_PAIR(1) | A_REVERSE | A_BOLD);
                mvwaddstr(menu_win, 1 + idx, 1, items[idx].c_str());
                wattroff(menu_win, COLOR_PAIR(1) | A_REVERSE | A_BOLD);
            } else {
                mvwaddstr(menu_win, 1 + idx, 1, items[idx].c_str());
            }
        }
        delwin(menu_win);
    }
}

void draw_dock(WINDOW* win, int w, int h) {
    int dock_w = 51;
    int dock_x = std::max(0, (w - dock_w) / 2);
    int dock_y = h - 3;
    
    
    wattron(win, COLOR_PAIR(2));
    mvwaddch(win, dock_y, dock_x, ACS_ULCORNER);
    mvwhline(win, dock_y, dock_x + 1, ACS_HLINE, dock_w - 2);
    mvwaddch(win, dock_y, dock_x + dock_w - 1, ACS_URCORNER);
    
    mvwaddch(win, dock_y + 1, dock_x, ACS_VLINE);
    mvwaddch(win, dock_y + 1, dock_x + dock_w - 1, ACS_VLINE);
    
    mvwaddch(win, dock_y + 2, dock_x, ACS_LLCORNER);
    mvwhline(win, dock_y + 2, dock_x + 1, ACS_HLINE, dock_w - 2);
    mvwaddch(win, dock_y + 2, dock_x + dock_w - 1, ACS_LRCORNER);
    wattroff(win, COLOR_PAIR(2));
    
    
    std::string item0 = active_pane == 0 ? " * STATS " : "   STATS ";
    std::string item1 = active_pane == 1 ? " * SCRATCH " : "   SCRATCH ";
    std::string item2 = active_pane == 2 ? " * LINKS " : "   LINKS ";
    std::string item3 = active_pane == 3 ? " * FILES " : "   FILES ";
    
    
    wattron(win, active_pane == 0 ? (COLOR_PAIR(1) | A_REVERSE | A_BOLD) : COLOR_PAIR(2));
    mvwaddstr(win, dock_y + 1, dock_x + 2, item0.c_str());
    wattroff(win, active_pane == 0 ? (COLOR_PAIR(1) | A_REVERSE | A_BOLD) : COLOR_PAIR(2));
    
    wattron(win, COLOR_PAIR(2));
    mvwaddch(win, dock_y + 1, dock_x + 12, ACS_VLINE);
    wattroff(win, COLOR_PAIR(2));
    
    
    wattron(win, active_pane == 1 ? (COLOR_PAIR(1) | A_REVERSE | A_BOLD) : COLOR_PAIR(2));
    mvwaddstr(win, dock_y + 1, dock_x + 14, item1.c_str());
    wattroff(win, active_pane == 1 ? (COLOR_PAIR(1) | A_REVERSE | A_BOLD) : COLOR_PAIR(2));
    
    wattron(win, COLOR_PAIR(2));
    mvwaddch(win, dock_y + 1, dock_x + 26, ACS_VLINE);
    wattroff(win, COLOR_PAIR(2));
    
    
    wattron(win, active_pane == 2 ? (COLOR_PAIR(1) | A_REVERSE | A_BOLD) : COLOR_PAIR(2));
    mvwaddstr(win, dock_y + 1, dock_x + 28, item2.c_str());
    wattroff(win, active_pane == 2 ? (COLOR_PAIR(1) | A_REVERSE | A_BOLD) : COLOR_PAIR(2));
    
    wattron(win, COLOR_PAIR(2));
    mvwaddch(win, dock_y + 1, dock_x + 38, ACS_VLINE);
    wattroff(win, COLOR_PAIR(2));
    
    
    wattron(win, active_pane == 3 ? (COLOR_PAIR(1) | A_REVERSE | A_BOLD) : COLOR_PAIR(2));
    mvwaddstr(win, dock_y + 1, dock_x + 40, item3.c_str());
    wattroff(win, active_pane == 3 ? (COLOR_PAIR(1) | A_REVERSE | A_BOLD) : COLOR_PAIR(2));
}
