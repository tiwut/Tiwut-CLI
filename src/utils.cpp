#include "utils.hpp"
#include "globals.hpp"
#include <fstream>
#include <algorithm>
#include <unistd.h>


const std::vector<std::string> embedded_logo = {
    " ____________________________________________________________ ",
    "|                          TIWUT               {#} {-} {x}   |",
    "|------------------------------------------------------------|",
    "|                                                            |",
    "|         #######  #  #         #  #     #  #######          |",
    "|            #     #   #   #   #   #     #     #             |",
    "|            #     #    # # # #    #     #     #             |",
    "|            #     #     #   #      #####      #             |",
    "|                                                            |",
    "|                 Pure Code. Total Freedom.                  |",
    "|           ______________________________________           |",
    "|           |                                    |           |",
    "|           | Website : https://tiwut.org/       |           |",
    "|           | GitHub  : https://github.com/tiwut |           |",
    "|           |____________________________________|           |",
    "|                                                            |",
    "| ''Every line of code is a step towards digital autonomy.'' |",
    "|                                                            |",
    "|____________________________________________________________|"
};

std::string get_basename(const std::string& path) {
    size_t found = path.find_last_of("/\\");
    if (found != std::string::npos) {
        return path.substr(found + 1);
    }
    return path;
}

void safe_addstr(WINDOW* win, int y, int x, const std::string& text, int attr) {
    if (!win) return;
    int h, w;
    getmaxyx(win, h, w);
    if (y >= 0 && y < h && x >= 0 && x < w) {
        int max_len = w - x - 1;
        if (max_len > 0) {
            std::string truncated = text.substr(0, max_len);
            wmove(win, y, x);
            wattron(win, attr);
            waddstr(win, truncated.c_str());
            wattroff(win, attr);
        }
    }
}

void draw_box(WINDOW* win, const std::string& title, bool focused) {
    if (!win) return;
    wclear(win);
    int h, w;
    getmaxyx(win, h, w);
    int color = COLOR_PAIR(focused ? 1 : 2);
    
    wattron(win, color);
    box(win, 0, 0);
    
    if (!title.empty()) {
        std::string padded = " " + title + " ";
        int start_x = std::max(1, (w - (int)padded.length()) / 2);
        wmove(win, 0, start_x);
        wattron(win, A_BOLD | color);
        waddstr(win, padded.c_str());
        wattroff(win, A_BOLD | color);
    }
    
    
    if (w > 12) {
        safe_addstr(win, 0, w - 10, "[", color);
        safe_addstr(win, 0, w - 9, "x", COLOR_PAIR(4) | A_BOLD); 
        safe_addstr(win, 0, w - 8, "][", color);
        safe_addstr(win, 0, w - 6, "■", COLOR_PAIR(5) | A_BOLD); 
        safe_addstr(win, 0, w - 5, "][", color);
        safe_addstr(win, 0, w - 3, "-", COLOR_PAIR(3) | A_BOLD); 
        safe_addstr(win, 0, w - 2, "]", color);
    }
    wattroff(win, color);
}

std::vector<std::string> read_logo() {
    char* home = getenv("HOME");
    std::string home_dir = home ? std::string(home) : "";
    std::vector<std::string> paths = {
        "./logo.txt",
        home_dir + "/.local/share/tiwut-cli/logo.txt",
        "/home/tiwut/Documents/Dev/Tiwut-CLI/logo.txt",
        "logo.txt"
    };
    
    for (const auto& path : paths) {
        std::ifstream f(path);
        if (f.is_open()) {
            std::vector<std::string> lines;
            std::string line;
            while (std::getline(f, line)) {
                lines.push_back(line);
            }
            return lines;
        }
    }
    return embedded_logo;
}

void show_modal(WINDOW* parent, const std::string& title, const std::vector<std::string>& content_lines) {
    int ph, pw;
    getmaxyx(stdscr, ph, pw);
    
    int modal_h = std::min(20, ph - 4);
    int modal_w = std::min(60, pw - 8);
    
    int y = (ph - modal_h) / 2;
    int x = (pw - modal_w) / 2;
    
    WINDOW* modal = newwin(modal_h, modal_w, y, x);
    nodelay(modal, FALSE);
    keypad(modal, TRUE);
    
    while (true) {
        wclear(modal);
        wattron(modal, COLOR_PAIR(1));
        box(modal, 0, 0);
        std::string title_padded = " " + title + " ";
        safe_addstr(modal, 0, (modal_w - (int)title_padded.length()) / 2, title_padded, A_BOLD | COLOR_PAIR(1));
        wattroff(modal, COLOR_PAIR(1));
        
        for (int idx = 0; idx < modal_h - 4; ++idx) {
            if (idx < (int)content_lines.size()) {
                safe_addstr(modal, 2 + idx, 2, content_lines[idx].substr(0, modal_w - 4), COLOR_PAIR(2));
            }
        }
        
        std::string footer = " Press ESC or 'q' to close ";
        safe_addstr(modal, modal_h - 1, (modal_w - (int)footer.length()) / 2, footer, COLOR_PAIR(5) | A_REVERSE);
        wrefresh(modal);
        
        int key = wgetch(modal);
        if (key == 27 || key == 'q' || key == 'Q') {
            break;
        }
    }
    
    delwin(modal);
    nodelay(stdscr, TRUE);
}

void log_debug(const std::string& msg) {
    std::ofstream f("debug.log", std::ios::app);
    if (f.is_open()) {
        f << msg << "\n";
    }
}

