#include "launcher_widget.hpp"
#include "../globals.hpp"
#include "../utils.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unistd.h>


int launcher_idx = 0;
std::vector<AppIcon> loaded_apps;


std::string find_index_json(std::string& base_path) {
    char* home = getenv("HOME");
    std::string home_dir = home ? std::string(home) : "";
    std::vector<std::string> candidates = {
        "./",
        home_dir + "/.local/share/tiwut-cli/"
    };
    for (const auto& base : candidates) {
        std::string full_path = base + "desktop/index.json";
        std::ifstream f(full_path);
        if (f.is_open()) {
            base_path = base;
            return full_path;
        }
    }
    base_path = "";
    return "";
}


std::vector<std::string> parse_index_json(const std::string& filepath) {
    std::vector<std::string> paths;
    std::ifstream f(filepath);
    if (!f.is_open()) return paths;
    
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    
    size_t array_start = content.find("[");
    size_t array_end = content.find("]", array_start);
    if (array_start == std::string::npos || array_end == std::string::npos) return paths;
    
    std::string array_content = content.substr(array_start + 1, array_end - array_start - 1);
    
    size_t pos = 0;
    while (true) {
        size_t q1 = array_content.find("\"", pos);
        if (q1 == std::string::npos) break;
        size_t q2 = array_content.find("\"", q1 + 1);
        if (q2 == std::string::npos) break;
        
        paths.push_back(array_content.substr(q1 + 1, q2 - q1 - 1));
        pos = q2 + 1;
    }
    return paths;
}


AppIcon parse_app_json(const std::string& filepath) {
    AppIcon app;
    app.name = "Unknown App";
    app.icon = "[ ]";
    app.exec = "";
    
    std::ifstream f(filepath);
    if (!f.is_open()) return app;
    
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    
    auto get_value = [&](const std::string& key) -> std::string {
        size_t key_pos = content.find("\"" + key + "\"");
        if (key_pos == std::string::npos) return "";
        
        size_t colon_pos = content.find(":", key_pos);
        if (colon_pos == std::string::npos) return "";
        
        size_t q1 = content.find("\"", colon_pos);
        if (q1 == std::string::npos) return "";
        size_t q2 = content.find("\"", q1 + 1);
        if (q2 == std::string::npos) return "";
        
        return content.substr(q1 + 1, q2 - q1 - 1);
    };
    
    std::string name_val = get_value("name");
    std::string icon_val = get_value("icon");
    std::string exec_val = get_value("exec");
    
    if (!name_val.empty()) app.name = name_val;
    if (!icon_val.empty()) app.icon = icon_val;
    if (!exec_val.empty()) app.exec = exec_val;
    
    return app;
}


void load_desktop_apps() {
    loaded_apps.clear();
    std::string base_path = "";
    std::string index_path = find_index_json(base_path);
    
    if (!index_path.empty()) {
        std::vector<std::string> app_paths = parse_index_json(index_path);
        for (const auto& sub_path : app_paths) {
            std::string full_app_path = base_path + sub_path;
            AppIcon app = parse_app_json(full_app_path);
            loaded_apps.push_back(app);
        }
    }
    
    
    if (loaded_apps.empty()) {
        loaded_apps = {
            {"Tiwut Website", "[W]", "xdg-open https://tiwut.org/"},
            {"Tiwut GitHub", "[G]", "xdg-open https://github.com/tiwut"},
            {"Cycle Color Theme", "[T]", "theme"},
            {"About Tiwut TUI", "[I]", "about"}
        };
    }
}

void draw_launcher_widget(WINDOW* win, bool focused) {
    draw_box(win, "APP LAUNCHER", focused);
    int h, w;
    getmaxyx(win, h, w);
    
    safe_addstr(win, 2, 2, "Select and press Enter:", COLOR_PAIR(3));
    
    
    for (size_t idx = 0; idx < loaded_apps.size(); ++idx) {
        int y = 4 + (idx * 2);
        std::string title = loaded_apps[idx].icon + " " + loaded_apps[idx].name;
        std::string desc = "Exec: " + loaded_apps[idx].exec;
        
        if ((int)idx == launcher_idx && focused) {
            safe_addstr(win, y, 2, " >> " + title + " ", COLOR_PAIR(1) | A_REVERSE | A_BOLD);
            safe_addstr(win, y + 1, 5, "-- " + desc, COLOR_PAIR(5));
        } else {
            safe_addstr(win, y, 2, "    " + title + " ", COLOR_PAIR(2));
            safe_addstr(win, y + 1, 5, "-- " + desc, COLOR_PAIR(2));
        }
    }
}
