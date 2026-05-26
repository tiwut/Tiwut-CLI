#include "explorer_widget.hpp"
#include "../globals.hpp"
#include "../utils.hpp"
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <algorithm>


std::string explorer_dir = ".";
int explorer_idx = 0;
std::vector<std::string> explorer_files;

void load_explorer_files() {
    explorer_files.clear();
    DIR* dir = opendir(explorer_dir.c_str());
    if (dir == nullptr) {
        explorer_files.push_back("../");
        return;
    }
    
    std::vector<std::string> dirs;
    std::vector<std::string> files;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name == ".") continue;
        if (name == "..") continue;
        
        std::string full_path = explorer_dir + "/" + name;
        struct stat st;
        if (stat(full_path.c_str(), &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                dirs.push_back(name + "/");
            } else {
                files.push_back(name);
            }
        }
    }
    closedir(dir);
    
    std::sort(dirs.begin(), dirs.end());
    std::sort(files.begin(), files.end());
    
    if (explorer_dir != "/") {
        explorer_files.push_back("../");
    }
    
    for (const auto& d : dirs) explorer_files.push_back(d);
    for (const auto& f : files) explorer_files.push_back(f);
    
    if (explorer_idx >= (int)explorer_files.size()) {
        explorer_idx = std::max(0, (int)explorer_files.size() - 1);
    }
}

void draw_explorer_widget(WINDOW* win, bool focused) {
    draw_box(win, "FILE EXPLORER", focused);
    int h, w;
    getmaxyx(win, h, w);
    
    std::string path_lbl = " Path: " + explorer_dir;
    if ((int)path_lbl.length() > w - 4) {
        path_lbl = " Path: ..." + path_lbl.substr(path_lbl.length() - (w - 10));
    }
    safe_addstr(win, 2, 2, path_lbl, COLOR_PAIR(3) | A_BOLD);
    
    int list_h = h - 5;
    int start = std::max(0, explorer_idx - list_h + 1);
    
    for (int idx = 0; idx < list_h; ++idx) {
        int f_idx = start + idx;
        if (f_idx < (int)explorer_files.size()) {
            std::string name = explorer_files[f_idx];
            bool is_d = name.back() == '/';
            std::string icon = is_d ? "[D] " : "[F] ";
            std::string line_str = icon + name;
            
            if (f_idx == explorer_idx && focused) {
                safe_addstr(win, 3 + idx, 2, " " + line_str + " ", COLOR_PAIR(1) | A_REVERSE | A_BOLD);
            } else {
                int col = is_d ? COLOR_PAIR(5) : COLOR_PAIR(2);
                safe_addstr(win, 3 + idx, 2, " " + line_str, col);
            }
        }
    }
    
    
    if ((int)explorer_files.size() > list_h) {
        int scroll_h = h - 5;
        int scroll_y = 3;
        int scroll_x = w - 2;
        
        for (int i = 0; i < scroll_h; ++i) {
            safe_addstr(win, scroll_y + i, scroll_x, "|", COLOR_PAIR(2));
        }
        
        int slider_h = std::max(1, (list_h * scroll_h) / (int)explorer_files.size());
        int slider_pos = (start * (scroll_h - slider_h)) / ((int)explorer_files.size() - list_h);
        
        for (int i = 0; i < slider_h; ++i) {
            safe_addstr(win, scroll_y + slider_pos + i, scroll_x, "#", COLOR_PAIR(1) | A_BOLD);
        }
    }
}

void view_file_modal(WINDOW* parent, const std::string& file_path) {
    std::ifstream f(file_path);
    if (!f.is_open()) {
        show_modal(parent, "Error", {"Could not read target file:", file_path});
        return;
    }
    std::vector<std::string> lines;
    std::string line;
    int count = 100;
    while (std::getline(f, line) && count-- > 0) {
        lines.push_back(line);
    }
    show_modal(parent, "Preview: " + get_basename(file_path), lines);
}

void show_system_info_modal(WINDOW* parent) {
    char hostname[256] = "localhost";
    gethostname(hostname, sizeof(hostname));
    
    char* path = getenv("PATH");
    std::string path_str = path ? std::string(path) : "N/A";
    if (path_str.length() > 45) path_str = path_str.substr(0, 42) + "...";
    
    std::vector<std::string> info = {
        "Host Name  : " + std::string(hostname),
        "OS Kernel  : Linux POSIX",
        "Environment: PATH=" + path_str,
        "",
        "Compiler   : GCC C++20 Standard Build",
        "Framework  : Native Ncurses Engine",
        "",
        "Note Files : ~/.local/share/tiwut-cli/notes.txt"
    };
    
    show_modal(parent, "Shell Environment Specifications", info);
}
