#include "git_terminal.hpp"
#include "../globals.hpp"
#include "../utils.hpp"
#include <unistd.h>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <algorithm>


extern std::string explorer_dir;
extern void load_explorer_files();


struct RemoteFile {
    std::string name;
    std::string path;
    std::string type; 
    std::string download_url;
};


std::vector<GithubRepo> parse_github_repos(const std::string& owner, const std::string& json_data) {
    std::vector<GithubRepo> repos;
    size_t pos = 0;
    
    while (true) {
        size_t next_obj = json_data.find("\"name\":", pos);
        if (next_obj == std::string::npos) break;
        
        GithubRepo repo;
        repo.owner = owner;
        
        auto get_str_val = [&](const std::string& key) -> std::string {
            size_t start_pos = next_obj > 30 ? next_obj - 30 : 0;
            size_t k_pos = json_data.find("\"" + key + "\"", start_pos);
            size_t limit = json_data.find("\"name\":", next_obj + 1);
            if (k_pos == std::string::npos || (limit != std::string::npos && k_pos > limit)) return "";
            
            size_t colon = json_data.find(":", k_pos);
            if (colon == std::string::npos) return "";
            size_t q1 = json_data.find("\"", colon);
            if (q1 == std::string::npos) return "";
            size_t q2 = json_data.find("\"", q1 + 1);
            if (q2 == std::string::npos) return "";
            return json_data.substr(q1 + 1, q2 - q1 - 1);
        };
        
        auto get_int_val = [&](const std::string& key) -> int {
            size_t start_pos = next_obj > 30 ? next_obj - 30 : 0;
            size_t k_pos = json_data.find("\"" + key + "\"", start_pos);
            size_t limit = json_data.find("\"name\":", next_obj + 1);
            if (k_pos == std::string::npos || (limit != std::string::npos && k_pos > limit)) return 0;
            
            size_t colon = json_data.find(":", k_pos);
            if (colon == std::string::npos) return 0;
            size_t num_start = json_data.find_first_of("0123456789", colon);
            if (num_start == std::string::npos) return 0;
            size_t num_end = json_data.find_first_not_of("0123456789", num_start);
            std::string num_str = json_data.substr(num_start, num_end - num_start);
            try {
                return std::stoi(num_str);
            } catch (...) {
                return 0;
            }
        };
        
        repo.name = get_str_val("name");
        repo.html_url = get_str_val("html_url");
        repo.description = get_str_val("description");
        repo.stars = get_int_val("stargazers_count");
        
        if (!repo.name.empty() && !repo.html_url.empty()) {
            repos.push_back(repo);
        }
        
        pos = next_obj + 1;
    }
    return repos;
}


std::vector<RemoteFile> parse_remote_contents(const std::string& json_data) {
    std::vector<RemoteFile> files;
    size_t pos = 0;
    
    while (true) {
        size_t next_obj = json_data.find("\"name\":", pos);
        if (next_obj == std::string::npos) break;
        
        RemoteFile file;
        
        auto get_str_val = [&](const std::string& key) -> std::string {
            size_t start_pos = next_obj > 20 ? next_obj - 20 : 0;
            size_t k_pos = json_data.find("\"" + key + "\"", start_pos);
            size_t limit = json_data.find("\"name\":", next_obj + 1);
            if (k_pos == std::string::npos || (limit != std::string::npos && k_pos > limit)) return "";
            
            size_t colon = json_data.find(":", k_pos);
            if (colon == std::string::npos) return "";
            size_t q1 = json_data.find("\"", colon);
            if (q1 == std::string::npos) return "";
            size_t q2 = json_data.find("\"", q1 + 1);
            if (q2 == std::string::npos) return "";
            return json_data.substr(q1 + 1, q2 - q1 - 1);
        };
        
        file.name = get_str_val("name");
        file.path = get_str_val("path");
        file.type = get_str_val("type");
        file.download_url = get_str_val("download_url");
        
        if (!file.name.empty()) {
            files.push_back(file);
        }
        pos = next_obj + 1;
    }
    return files;
}


std::string fetch_github_api(const std::string& url) {
    std::string cmd = "curl -s -H \"User-Agent: TiwutOS-CLI\" \"" + url + "\"";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    
    char buffer[1024];
    std::string response = "";
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        response += buffer;
    }
    pclose(pipe);
    return response;
}


std::string fetch_github_readme(const std::string& owner, const std::string& repo) {
    std::string api_url = "https://api.github.com/repos/" + owner + "/" + repo + "/readme";
    std::string meta = fetch_github_api(api_url);
    
    size_t d_pos = meta.find("\"download_url\":");
    if (d_pos == std::string::npos) {
        std::string fallback = fetch_github_api("https://raw.githubusercontent.com/" + owner + "/" + repo + "/main/README.md");
        if (fallback.empty() || fallback.rfind("404", 0) == 0) {
            fallback = fetch_github_api("https://raw.githubusercontent.com/" + owner + "/" + repo + "/master/README.md");
        }
        return fallback;
    }
    
    size_t colon = meta.find(":", d_pos);
    if (colon == std::string::npos) return "";
    size_t q1 = meta.find("\"", colon);
    if (q1 == std::string::npos) return "";
    size_t q2 = meta.find("\"", q1 + 1);
    if (q2 == std::string::npos) return "";
    std::string download_url = meta.substr(q1 + 1, q2 - q1 - 1);
    
    return fetch_github_api(download_url);
}


void draw_terminal_interface(WINDOW* win, const std::vector<std::string>& history, const std::string& current_input, int scroll_offset) {
    int h, w;
    getmaxyx(win, h, w);
    wclear(win);
    
    wattron(win, COLOR_PAIR(1) | A_BOLD);
    box(win, 0, 0);
    std::string title = " TIWUTOS INTEGRATED DEV CONSOLE SHELL ";
    safe_addstr(win, 0, (w - (int)title.length()) / 2, title, COLOR_PAIR(1) | A_BOLD);
    wattroff(win, COLOR_PAIR(1) | A_BOLD);
    
    int max_lines = h - 4;
    int total_lines = history.size();
    
    std::string footer = " Press ESC/exit to return | Type 'clear' | Page Up/Down to scroll ";
    safe_addstr(win, h - 1, (w - (int)footer.length()) / 2, footer, COLOR_PAIR(5) | A_REVERSE);
    
    int start_idx = 0;
    if (total_lines > max_lines) {
        start_idx = total_lines - max_lines - scroll_offset;
        if (start_idx < 0) start_idx = 0;
    }
    
    for (int i = 0; i < max_lines; ++i) {
        int idx = start_idx + i;
        if (idx < total_lines) {
            safe_addstr(win, 1 + i, 2, history[idx], COLOR_PAIR(2));
        }
    }
    
    char cwd[1024];
    std::string cwd_str = "/";
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        cwd_str = get_basename(std::string(cwd));
        if (cwd_str.empty()) cwd_str = "/";
    }
    
    std::string prompt = "tiwutos@cli:" + cwd_str + " $ " + current_input;
    safe_addstr(win, h - 3, 2, prompt, COLOR_PAIR(3) | A_BOLD);
}


void execute_shell_cmd(WINDOW* win, const std::string& cmd, std::vector<std::string>& history, int& scroll_offset) {
    history.push_back("tiwutos@cli $ " + cmd);
    scroll_offset = 0;
    
    if (cmd == "clear") {
        history.clear();
        return;
    }
    
    if (cmd.rfind("cd ", 0) == 0) {
        std::string dir = cmd.substr(3);
        dir.erase(0, dir.find_first_not_of(" \t\r\n"));
        dir.erase(dir.find_last_not_of(" \t\r\n") + 1);
        if (chdir(dir.c_str()) == 0) {
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != nullptr) {
                explorer_dir = std::string(cwd);
                load_explorer_files();
                history.push_back("Directory successfully loaded: " + explorer_dir);
            }
        } else {
            history.push_back("cd: " + dir + ": No such directory exists.");
        }
        return;
    }
    
    std::string full_cmd = cmd + " 2>&1";
    FILE* pipe = popen(full_cmd.c_str(), "r");
    if (!pipe) {
        history.push_back("Error: Exec failed.");
        return;
    }
    
    char buffer[512];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        std::string line(buffer);
        if (!line.empty() && line.back() == '\n') line.pop_back();
        if (!line.empty() && line.back() == '\r') line.pop_back();
        history.push_back(line);
        
        
        int h, w;
        getmaxyx(win, h, w);
        wclear(win);
        wattron(win, COLOR_PAIR(1) | A_BOLD);
        box(win, 0, 0);
        safe_addstr(win, 0, (w - 38) / 2, " TIWUTOS INTEGRATED DEV CONSOLE SHELL ", COLOR_PAIR(1) | A_BOLD);
        wattroff(win, COLOR_PAIR(1) | A_BOLD);
        
        int max_lines = h - 4;
        int total_lines = history.size();
        int start_idx = total_lines > max_lines ? total_lines - max_lines : 0;
        
        for (int i = 0; i < max_lines; ++i) {
            int idx = start_idx + i;
            if (idx < total_lines) {
                safe_addstr(win, 1 + i, 2, history[idx], COLOR_PAIR(2));
            }
        }
        safe_addstr(win, h - 3, 2, "Running: " + cmd + " [■]", COLOR_PAIR(5) | A_BOLD | A_BLINK);
        wrefresh(win);
    }
    pclose(pipe);
}


void run_integrated_terminal(WINDOW* parent) {
    int ph, pw;
    getmaxyx(parent, ph, pw);
    
    WINDOW* term_win = derwin(parent, ph - 4, pw - 4, 2, 2);
    if (!term_win) return;
    
    nodelay(term_win, FALSE);
    keypad(term_win, TRUE);
    
    std::vector<std::string> history = {
        "TiwutOS Integrated Developer Shell Shell Console v1.0",
        "Type standard shell statements, compile utilities, or check files.",
        "Internal built-in features: 'clear' to empty console, 'exit' to escape.",
        "----------------------------------------------------------------------"
    };
    std::vector<std::string> typed_commands;
    int typed_idx = -1;
    std::string current_input = "";
    int scroll_offset = 0;
    
    while (true) {
        draw_terminal_interface(term_win, history, current_input, scroll_offset);
        wrefresh(term_win);
        
        int key = wgetch(term_win);
        if (key == 27) { 
            break;
        }
        else if (key == 10 || key == 13 || key == KEY_ENTER) {
            if (!current_input.empty()) {
                if (current_input == "exit") {
                    break;
                }
                typed_commands.push_back(current_input);
                typed_idx = -1;
                execute_shell_cmd(term_win, current_input, history, scroll_offset);
                current_input = "";
            }
        }
        else if (key == KEY_BACKSPACE || key == 127 || key == 8) {
            if (!current_input.empty()) {
                current_input.pop_back();
            }
        }
        else if (key == KEY_UP) { 
            if (!typed_commands.empty()) {
                if (typed_idx == -1) typed_idx = typed_commands.size() - 1;
                else if (typed_idx > 0) typed_idx--;
                current_input = typed_commands[typed_idx];
            }
        }
        else if (key == KEY_DOWN) { 
            if (!typed_commands.empty() && typed_idx != -1) {
                if (typed_idx < (int)typed_commands.size() - 1) {
                    typed_idx++;
                    current_input = typed_commands[typed_idx];
                } else {
                    typed_idx = -1;
                    current_input = "";
                }
            }
        }
        else if (key == KEY_PPAGE) { 
            int max_lines = (ph - 4) - 4;
            if ((int)history.size() > max_lines) {
                scroll_offset = std::min((int)history.size() - max_lines, scroll_offset + 5);
            }
        }
        else if (key == KEY_NPAGE) { 
            scroll_offset = std::max(0, scroll_offset - 5);
        }
        else if (key >= 32 && key <= 126) {
            current_input.push_back((char)key);
        }
    }
    
    delwin(term_win);
    nodelay(parent, TRUE);
}


void show_readme_modal(WINDOW* parent, const std::string& owner, const std::string& repo) {
    int ph, pw;
    getmaxyx(parent, ph, pw);
    
    
    int lw = 50, lh = 5;
    WINDOW* load_win = derwin(parent, lh, lw, (ph - lh) / 2, (pw - lw) / 2);
    if (load_win) {
        wattron(load_win, COLOR_PAIR(1) | A_BOLD);
        box(load_win, 0, 0);
        safe_addstr(load_win, 2, 4, "Fetching README contents... Please wait", COLOR_PAIR(5) | A_BOLD);
        wattroff(load_win, COLOR_PAIR(1) | A_BOLD);
        wrefresh(load_win);
        delwin(load_win);
    }
    
    std::string content = fetch_github_readme(owner, repo);
    if (content.empty() || content.rfind("404", 0) == 0) {
        show_modal(parent, "README Error", {
            "Could not retrieve README.md for this repository.",
            "It may not exist or is private."
        });
        return;
    }
    
    std::vector<std::string> lines;
    std::stringstream ss(content);
    std::string line;
    while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(line);
    }
    
    int modal_h = ph - 6;
    int modal_w = pw - 6;
    int y = 3;
    int x = 3;
    
    WINDOW* readme_win = derwin(parent, modal_h, modal_w, y, x);
    if (!readme_win) return;
    
    nodelay(readme_win, FALSE);
    keypad(readme_win, TRUE);
    
    int active_line = 0;
    int visible_height = modal_h - 4;
    
    while (true) {
        wclear(readme_win);
        wattron(readme_win, COLOR_PAIR(1) | A_BOLD);
        box(readme_win, 0, 0);
        std::string title = " README.md: " + owner + "/" + repo + " ";
        safe_addstr(readme_win, 0, (modal_w - (int)title.length()) / 2, title, COLOR_PAIR(1) | A_BOLD);
        wattroff(readme_win, COLOR_PAIR(1) | A_BOLD);
        
        std::string footer = " UP/DOWN/PgUp/PgDn: Scroll | ESC or 'q' to close ";
        safe_addstr(readme_win, modal_h - 1, (modal_w - (int)footer.length()) / 2, footer, COLOR_PAIR(5) | A_REVERSE);
        
        if (active_line >= (int)lines.size()) active_line = (int)lines.size() - 1;
        if (active_line < 0) active_line = 0;
        
        int start_line = active_line;
        
        for (int i = 0; i < visible_height; ++i) {
            int line_idx = start_line + i;
            if (line_idx < (int)lines.size()) {
                std::string draw_line = lines[line_idx];
                
                
                int attr = COLOR_PAIR(2);
                if (!draw_line.empty() && draw_line[0] == '#') {
                    attr = COLOR_PAIR(1) | A_BOLD; 
                } else if (!draw_line.empty() && (draw_line[0] == '-' || draw_line[0] == '*')) {
                    attr = COLOR_PAIR(3) | A_BOLD; 
                } else if (!draw_line.empty() && draw_line[0] == '>') {
                    attr = COLOR_PAIR(5) | A_BOLD; 
                }
                
                safe_addstr(readme_win, 1 + i, 2, draw_line, attr);
            }
        }
        
        
        if ((int)lines.size() > visible_height) {
            int scroll_h = visible_height;
            int scroll_y = 1;
            int scroll_x = modal_w - 2;
            
            wattron(readme_win, COLOR_PAIR(2));
            for (int i = 0; i < scroll_h; ++i) {
                mvwaddch(readme_win, scroll_y + i, scroll_x, '|');
            }
            wattroff(readme_win, COLOR_PAIR(2));
            
            int slider_h = std::max(1, (visible_height * scroll_h) / (int)lines.size());
            int slider_pos = (start_line * (scroll_h - slider_h)) / ((int)lines.size() - visible_height);
            if (slider_pos < 0) slider_pos = 0;
            if (slider_pos + slider_h > scroll_h) slider_pos = scroll_h - slider_h;
            
            wattron(readme_win, COLOR_PAIR(1) | A_BOLD);
            for (int i = 0; i < slider_h; ++i) {
                mvwaddch(readme_win, scroll_y + slider_pos + i, scroll_x, '#');
            }
            wattroff(readme_win, COLOR_PAIR(1) | A_BOLD);
        }
        
        wrefresh(readme_win);
        
        int r_key = wgetch(readme_win);
        if (r_key == 27 || r_key == 'q' || r_key == 'Q') {
            break;
        }
        else if (r_key == KEY_UP) {
            if (active_line > 0) active_line--;
        }
        else if (r_key == KEY_DOWN) {
            if (active_line < (int)lines.size() - 1) active_line++;
        }
        else if (r_key == KEY_PPAGE) {
            active_line = std::max(0, active_line - visible_height);
        }
        else if (r_key == KEY_NPAGE) {
            active_line = std::min((int)lines.size() - 1, active_line + visible_height);
        }
    }
    
    delwin(readme_win);
}


void run_remote_file_browser(WINDOW* parent, const std::string& owner, const std::string& repo) {
    int ph, pw;
    getmaxyx(parent, ph, pw);
    
    std::string current_path = "";
    std::vector<RemoteFile> files_list;
    int active_idx = 0;
    
    
    auto load_contents = [&](WINDOW* p, const std::string& path) -> bool {
        int lw = 50, lh = 5;
        WINDOW* load_win = derwin(p, lh, lw, (ph - lh) / 2, (pw - lw) / 2);
        if (load_win) {
            wattron(load_win, COLOR_PAIR(1) | A_BOLD);
            box(load_win, 0, 0);
            safe_addstr(load_win, 2, 4, "Fetching remote repository index...", COLOR_PAIR(5) | A_BOLD);
            wattroff(load_win, COLOR_PAIR(1) | A_BOLD);
            wrefresh(load_win);
            delwin(load_win);
        }
        
        std::string url = "https://api.github.com/repos/" + owner + "/" + repo + "/contents/" + path;
        std::string data = fetch_github_api(url);
        
        files_list = parse_remote_contents(data);
        
        
        std::sort(files_list.begin(), files_list.end(), [](const RemoteFile& a, const RemoteFile& b) {
            if (a.type != b.type) {
                return a.type == "dir";
            }
            return a.name < b.name;
        });
        
        
        if (!path.empty()) {
            RemoteFile parent_dir;
            parent_dir.name = "../";
            parent_dir.type = "parent";
            files_list.insert(files_list.begin(), parent_dir);
        }
        return !files_list.empty();
    };
    
    load_contents(parent, current_path);
    
    WINDOW* explorer_win = derwin(parent, ph - 4, pw - 4, 2, 2);
    if (!explorer_win) return;
    
    nodelay(explorer_win, FALSE);
    keypad(explorer_win, TRUE);
    
    while (true) {
        wclear(explorer_win);
        wattron(explorer_win, COLOR_PAIR(1) | A_BOLD);
        box(explorer_win, 0, 0);
        std::string title = " REMOTE REPO EXPLORER: " + owner + "/" + repo + " ";
        safe_addstr(explorer_win, 0, (pw - 4 - (int)title.length()) / 2, title, COLOR_PAIR(1) | A_BOLD);
        wattroff(explorer_win, COLOR_PAIR(1) | A_BOLD);
        
        std::string footer = " UP/DOWN/PgUp/PgDn: Select | ENTER: Open | Backspace: Parent dir | ESC: exit ";
        safe_addstr(explorer_win, ph - 5, (pw - 4 - (int)footer.length()) / 2, footer, COLOR_PAIR(5) | A_REVERSE);
        
        std::string path_lbl = " Path: /" + current_path;
        if (path_lbl.length() > (size_t)pw - 10) path_lbl = " Path: .../" + get_basename(current_path);
        safe_addstr(explorer_win, 2, 4, path_lbl, COLOR_PAIR(3) | A_BOLD);
        
        wattron(explorer_win, COLOR_PAIR(2));
        mvwhline(explorer_win, 3, 1, ACS_HLINE, pw - 6);
        wattroff(explorer_win, COLOR_PAIR(2));
        
        int visible_lines = ph - 11;
        if (visible_lines < 1) visible_lines = 1;
        
        if (active_idx >= (int)files_list.size()) active_idx = (int)files_list.size() - 1;
        if (active_idx < 0) active_idx = 0;
        
        int start = std::max(0, active_idx - visible_lines + 1);
        
        if (files_list.empty()) {
            safe_addstr(explorer_win, 5, 4, "No contents found inside this directory.", COLOR_PAIR(4) | A_BOLD);
        } else {
            for (int idx = 0; idx < visible_lines; ++idx) {
                int f_idx = start + idx;
                if (f_idx < (int)files_list.size()) {
                    const auto& item = files_list[f_idx];
                    int y = 4 + idx;
                    
                    std::string draw_lbl = "";
                    int col = COLOR_PAIR(2);
                    
                    if (item.type == "dir") {
                        draw_lbl = "[D] " + item.name + "/";
                        col = COLOR_PAIR(5); 
                    } else if (item.type == "parent") {
                        draw_lbl = "[D] ../";
                        col = COLOR_PAIR(5) | A_BOLD;
                    } else {
                        draw_lbl = "[F] " + item.name;
                        col = COLOR_PAIR(2);
                    }
                    
                    if (f_idx == active_idx) {
                        safe_addstr(explorer_win, y, 4, " >> " + draw_lbl + " ", COLOR_PAIR(1) | A_REVERSE | A_BOLD);
                    } else {
                        safe_addstr(explorer_win, y, 4, "    " + draw_lbl, col);
                    }
                }
            }
            
            
            if ((int)files_list.size() > visible_lines) {
                int scroll_h = visible_lines;
                int scroll_y = 4;
                int scroll_x = pw - 6;
                
                wattron(explorer_win, COLOR_PAIR(2));
                for (int i = 0; i < scroll_h; ++i) {
                    mvwaddch(explorer_win, scroll_y + i, scroll_x, '|');
                }
                wattroff(explorer_win, COLOR_PAIR(2));
                
                int slider_h = std::max(1, (visible_lines * scroll_h) / (int)files_list.size());
                int slider_pos = (start * (scroll_h - slider_h)) / ((int)files_list.size() - visible_lines);
                if (slider_pos < 0) slider_pos = 0;
                if (slider_pos + slider_h > scroll_h) slider_pos = scroll_h - slider_h;
                
                wattron(explorer_win, COLOR_PAIR(1) | A_BOLD);
                for (int i = 0; i < slider_h; ++i) {
                    mvwaddch(explorer_win, scroll_y + slider_pos + i, scroll_x, '#');
                }
                wattroff(explorer_win, COLOR_PAIR(1) | A_BOLD);
            }
        }
        
        wrefresh(explorer_win);
        
        int key = wgetch(explorer_win);
        if (key == 27) { 
            break;
        }
        else if (key == KEY_UP) {
            if (active_idx > 0) active_idx--;
        }
        else if (key == KEY_DOWN) {
            if (active_idx < (int)files_list.size() - 1) active_idx++;
        }
        else if (key == KEY_PPAGE) {
            active_idx = std::max(0, active_idx - visible_lines);
        }
        else if (key == KEY_NPAGE) {
            active_idx = std::min((int)files_list.size() - 1, active_idx + visible_lines);
        }
        else if (key == KEY_BACKSPACE || key == 127 || key == 8) { 
            if (!current_path.empty()) {
                size_t slash = current_path.find_last_of('/');
                if (slash != std::string::npos) {
                    current_path = current_path.substr(0, slash);
                } else {
                    current_path = "";
                }
                load_contents(explorer_win, current_path);
                active_idx = 0;
            }
        }
        else if (key == 10 || key == 13 || key == KEY_ENTER) {
            if (!files_list.empty() && active_idx >= 0 && active_idx < (int)files_list.size()) {
                const auto& selected = files_list[active_idx];
                
                if (selected.type == "dir") {
                    current_path = selected.path;
                    load_contents(explorer_win, current_path);
                    active_idx = 0;
                }
                else if (selected.type == "parent") {
                    if (!current_path.empty()) {
                        size_t slash = current_path.find_last_of('/');
                        if (slash != std::string::npos) {
                            current_path = current_path.substr(0, slash);
                        } else {
                            current_path = "";
                        }
                        load_contents(explorer_win, current_path);
                        active_idx = 0;
                    }
                }
                else { 
                    int lw = 50, lh = 5;
                    WINDOW* load_win = derwin(explorer_win, lh, lw, (ph - 4 - lh) / 2, (pw - 4 - lw) / 2);
                    if (load_win) {
                        wattron(load_win, COLOR_PAIR(1) | A_BOLD);
                        box(load_win, 0, 0);
                        safe_addstr(load_win, 2, 4, "Downloading remote file contents...", COLOR_PAIR(5) | A_BOLD);
                        wattroff(load_win, COLOR_PAIR(1) | A_BOLD);
                        wrefresh(load_win);
                        delwin(load_win);
                    }
                    
                    std::string file_content = fetch_github_api(selected.download_url);
                    if (file_content.empty() || file_content.rfind("404", 0) == 0) {
                        show_modal(explorer_win, "Download Error", {
                            "Failed to retrieve file contents remotely.",
                            "The file may be binary, too large, or private."
                        });
                    } else {
                        
                        std::vector<std::string> lines;
                        std::stringstream ss(file_content);
                        std::string f_line;
                        while (std::getline(ss, f_line)) {
                            if (!f_line.empty() && f_line.back() == '\r') f_line.pop_back();
                            lines.push_back(f_line);
                        }
                        
                        int view_h = ph - 6;
                        int view_w = pw - 6;
                        WINDOW* view_win = derwin(explorer_win, view_h, view_w, 3, 3);
                        if (view_win) {
                            nodelay(view_win, FALSE);
                            keypad(view_win, TRUE);
                            
                            int view_idx = 0;
                            int vis_h = view_h - 4;
                            
                            while (true) {
                                wclear(view_win);
                                wattron(view_win, COLOR_PAIR(1) | A_BOLD);
                                box(view_win, 0, 0);
                                std::string view_title = " View Remote File: " + selected.name + " (" + std::to_string(lines.size()) + " lines) ";
                                safe_addstr(view_win, 0, (view_w - (int)view_title.length()) / 2, view_title, COLOR_PAIR(1) | A_BOLD);
                                wattroff(view_win, COLOR_PAIR(1) | A_BOLD);
                                
                                std::string view_footer = " UP/DOWN/PgUp/PgDn: Scroll | ESC/q: Exit ";
                                safe_addstr(view_win, view_h - 1, (view_w - (int)view_footer.length()) / 2, view_footer, COLOR_PAIR(5) | A_REVERSE);
                                
                                if (view_idx >= (int)lines.size()) view_idx = (int)lines.size() - 1;
                                if (view_idx < 0) view_idx = 0;
                                
                                for (int i = 0; i < vis_h; ++i) {
                                    int l_idx = view_idx + i;
                                    if (l_idx < (int)lines.size()) {
                                        safe_addstr(view_win, 1 + i, 2, lines[l_idx], COLOR_PAIR(2));
                                    }
                                }
                                
                                
                                if ((int)lines.size() > vis_h) {
                                    int scr_h = vis_h;
                                    int scr_y = 1;
                                    int scr_x = view_w - 2;
                                    
                                    wattron(view_win, COLOR_PAIR(2));
                                    for (int i = 0; i < scr_h; ++i) {
                                        mvwaddch(view_win, scr_y + i, scr_x, '|');
                                    }
                                    wattroff(view_win, COLOR_PAIR(2));
                                    
                                    int sld_h = std::max(1, (vis_h * scr_h) / (int)lines.size());
                                    int sld_pos = (view_idx * (scr_h - sld_h)) / ((int)lines.size() - vis_h);
                                    if (sld_pos < 0) sld_pos = 0;
                                    if (sld_pos + sld_h > scr_h) sld_pos = scr_h - sld_h;
                                    
                                    wattron(view_win, COLOR_PAIR(1) | A_BOLD);
                                    for (int i = 0; i < sld_h; ++i) {
                                        mvwaddch(view_win, scr_y + sld_pos + i, scr_x, '#');
                                    }
                                    wattroff(view_win, COLOR_PAIR(1) | A_BOLD);
                                }
                                
                                wrefresh(view_win);
                                int v_key = wgetch(view_win);
                                if (v_key == 27 || v_key == 'q' || v_key == 'Q') {
                                    break;
                                }
                                else if (v_key == KEY_UP) {
                                    if (view_idx > 0) view_idx--;
                                }
                                else if (v_key == KEY_DOWN) {
                                    if (view_idx < (int)lines.size() - 1) view_idx++;
                                }
                                else if (v_key == KEY_PPAGE) {
                                    view_idx = std::max(0, view_idx - vis_h);
                                }
                                else if (v_key == KEY_NPAGE) {
                                    view_idx = std::min((int)lines.size() - 1, view_idx + vis_h);
                                }
                            }
                            delwin(view_win);
                        }
                    }
                }
            }
        }
    }
    
    delwin(explorer_win);
    nodelay(parent, TRUE);
}


void run_github_repos_menu(WINDOW* parent) {
    int ph, pw;
    getmaxyx(parent, ph, pw);
    
    
    int lw = 50, lh = 5;
    WINDOW* load_win = derwin(parent, lh, lw, (ph - lh) / 2, (pw - lw) / 2);
    if (load_win) {
        wattron(load_win, COLOR_PAIR(1) | A_BOLD);
        box(load_win, 0, 0);
        safe_addstr(load_win, 2, 4, "Connecting to GitHub API... Please wait", COLOR_PAIR(5) | A_BOLD);
        wattroff(load_win, COLOR_PAIR(1) | A_BOLD);
        wrefresh(load_win);
        delwin(load_win);
    }
    
    
    std::string data_tiwut = fetch_github_api("https://api.github.com/users/tiwut/repos");
    std::string data_nexus = fetch_github_api("https://api.github.com/users/nexus-titan/repos");
    
    std::vector<GithubRepo> r_tiwut = parse_github_repos("tiwut", data_tiwut);
    std::vector<GithubRepo> r_nexus = parse_github_repos("nexus-titan", data_nexus);
    
    if (r_tiwut.empty() && r_nexus.empty()) {
        show_modal(parent, "GitHub Error", {
            "Failed to fetch repositories for both users.",
            "Verify your internet connection and try again.",
            "API URL: api.github.com/users/{tiwut,nexus-titan}/repos"
        });
        return;
    }
    
    
    WINDOW* repos_win = derwin(parent, ph - 4, pw - 4, 2, 2);
    if (!repos_win) return;
    
    nodelay(repos_win, FALSE);
    keypad(repos_win, TRUE);
    
    int current_tab = 0; 
    int active_idx = 0;
    
    while (true) {
        wclear(repos_win);
        wattron(repos_win, COLOR_PAIR(1) | A_BOLD);
        box(repos_win, 0, 0);
        std::string title = " GITHUB REPOSITORY MANAGER ";
        safe_addstr(repos_win, 0, (pw - 4 - (int)title.length()) / 2, title, COLOR_PAIR(1) | A_BOLD);
        wattroff(repos_win, COLOR_PAIR(1) | A_BOLD);
        
        
        std::string footer = " Tab/Left/Right: Switch user | UP/DOWN/PgUp/PgDn: Scroll | ENTER: Actions | ESC: exit ";
        safe_addstr(repos_win, ph - 5, (pw - 4 - (int)footer.length()) / 2, footer, COLOR_PAIR(5) | A_REVERSE);
        
        
        std::string tab0 = " [1] tiwut (" + std::to_string(r_tiwut.size() ) + " Repos) ";
        std::string tab1 = " [2] nexus-titan (" + std::to_string(r_nexus.size()) + " Repos) ";
        
        int t0_x = 4;
        int t1_x = t0_x + tab0.length() + 4;
        
        
        wattron(repos_win, current_tab == 0 ? (COLOR_PAIR(1) | A_REVERSE | A_BOLD) : COLOR_PAIR(2));
        mvwaddstr(repos_win, 2, t0_x, tab0.c_str());
        wattroff(repos_win, current_tab == 0 ? (COLOR_PAIR(1) | A_REVERSE | A_BOLD) : COLOR_PAIR(2));
        
        
        wattron(repos_win, current_tab == 1 ? (COLOR_PAIR(1) | A_REVERSE | A_BOLD) : COLOR_PAIR(2));
        mvwaddstr(repos_win, 2, t1_x, tab1.c_str());
        wattroff(repos_win, current_tab == 1 ? (COLOR_PAIR(1) | A_REVERSE | A_BOLD) : COLOR_PAIR(2));
        
        
        wattron(repos_win, COLOR_PAIR(2));
        mvwhline(repos_win, 3, 1, ACS_HLINE, pw - 6);
        wattroff(repos_win, COLOR_PAIR(2));
        
        
        const std::vector<GithubRepo>& active_list = (current_tab == 0) ? r_tiwut : r_nexus;
        
        
        if (active_list.empty()) {
            safe_addstr(repos_win, 6, 4, "No public repositories found for this user account.", COLOR_PAIR(4) | A_BOLD);
        } else {
            
            int visible_repos_count = (ph - 11) / 2;
            if (visible_repos_count < 1) visible_repos_count = 1;
            
            
            if (active_idx >= (int)active_list.size()) active_idx = (int)active_list.size() - 1;
            if (active_idx < 0) active_idx = 0;
            
            int start = std::max(0, active_idx - visible_repos_count + 1);
            
            for (int idx = 0; idx < visible_repos_count; ++idx) {
                int r_idx = start + idx;
                if (r_idx < (int)active_list.size()) {
                    const auto& repo = active_list[r_idx];
                    int y = 5 + (idx * 2);
                    
                    std::string header_lbl = repo.name + " (★ " + std::to_string(repo.stars) + ")";
                    std::string desc_lbl = "      " + (repo.description.empty() ? "(No description provided)" : repo.description);
                    if (desc_lbl.length() > (size_t)pw - 12) desc_lbl = desc_lbl.substr(0, pw - 15) + "...";
                    
                    if (r_idx == active_idx) {
                        safe_addstr(repos_win, y, 2, " >> " + header_lbl + " ", COLOR_PAIR(1) | A_REVERSE | A_BOLD);
                        safe_addstr(repos_win, y + 1, 2, desc_lbl, COLOR_PAIR(5) | A_BOLD);
                    } else {
                        safe_addstr(repos_win, y, 2, "    " + header_lbl, COLOR_PAIR(3));
                        safe_addstr(repos_win, y + 1, 2, desc_lbl, COLOR_PAIR(2));
                    }
                }
            }
            
            
            if ((int)active_list.size() > visible_repos_count) {
                int scroll_h = visible_repos_count * 2;
                int scroll_y = 5;
                int scroll_x = pw - 6;
                
                wattron(repos_win, COLOR_PAIR(2));
                for (int i = 0; i < scroll_h; ++i) {
                    mvwaddch(repos_win, scroll_y + i, scroll_x, '|');
                }
                wattroff(repos_win, COLOR_PAIR(2));
                
                int slider_h = std::max(1, (visible_repos_count * scroll_h) / (int)active_list.size());
                int slider_pos = (start * (scroll_h - slider_h)) / ((int)active_list.size() - visible_repos_count);
                if (slider_pos < 0) slider_pos = 0;
                if (slider_pos + slider_h > scroll_h) slider_pos = scroll_h - slider_h;
                
                wattron(repos_win, COLOR_PAIR(1) | A_BOLD);
                for (int i = 0; i < slider_h; ++i) {
                    mvwaddch(repos_win, scroll_y + slider_pos + i, scroll_x, '#');
                }
                wattroff(repos_win, COLOR_PAIR(1) | A_BOLD);
            }
        }
        
        wrefresh(repos_win);
        
        int key = wgetch(repos_win);
        if (key == 27) { 
            break;
        }
        else if (key == KEY_UP) {
            if (active_idx > 0) active_idx--;
        }
        else if (key == KEY_DOWN) {
            const std::vector<GithubRepo>& active_list = (current_tab == 0) ? r_tiwut : r_nexus;
            if (active_idx < (int)active_list.size() - 1) active_idx++;
        }
        else if (key == KEY_LEFT || key == 9) { 
            current_tab = (current_tab == 0) ? 1 : 0;
            active_idx = 0;
        }
        else if (key == KEY_RIGHT) { 
            current_tab = (current_tab == 0) ? 1 : 0;
            active_idx = 0;
        }
        else if (key == KEY_PPAGE) { 
            int visible_repos_count = (ph - 11) / 2;
            active_idx = std::max(0, active_idx - visible_repos_count);
        }
        else if (key == KEY_NPAGE) { 
            int visible_repos_count = (ph - 11) / 2;
            const std::vector<GithubRepo>& active_list = (current_tab == 0) ? r_tiwut : r_nexus;
            active_idx = std::min((int)active_list.size() - 1, active_idx + visible_repos_count);
        }
        else if (key == 10 || key == 13 || key == KEY_ENTER) {
            const std::vector<GithubRepo>& active_list = (current_tab == 0) ? r_tiwut : r_nexus;
            if (!active_list.empty() && active_idx >= 0 && active_idx < (int)active_list.size()) {
                const auto& selected = active_list[active_idx];
                
                
                int mw = 56, mh = 14;
                WINDOW* act_win = derwin(repos_win, mh, mw, (ph - 14 - mh) / 2, (pw - 4 - mw) / 2);
                if (act_win) {
                    int opt_idx = 0;
                    nodelay(act_win, FALSE);
                    keypad(act_win, TRUE);
                    
                    while (true) {
                        wclear(act_win);
                        wattron(act_win, COLOR_PAIR(1) | A_BOLD);
                        box(act_win, 0, 0);
                        std::string act_title = " Select Action: " + selected.name + " ";
                        safe_addstr(act_win, 0, (mw - (int)act_title.length()) / 2, act_title, COLOR_PAIR(1) | A_BOLD);
                        wattroff(act_win, COLOR_PAIR(1) | A_BOLD);
                        
                        std::vector<std::string> opts = {
                            "1. Git Clone Repository here",
                            "2. View README.md file contents",
                            "3. Browse Repository Files (Remote)",
                            "4. Open Repository URL in Browser",
                            "5. Cancel / Go Back"
                        };
                        
                        for (size_t i = 0; i < opts.size(); ++i) {
                            if ((int)i == opt_idx) {
                                safe_addstr(act_win, 2 + i * 2, 4, " >> " + opts[i] + " ", COLOR_PAIR(1) | A_REVERSE | A_BOLD);
                            } else {
                                safe_addstr(act_win, 2 + i * 2, 4, "    " + opts[i], COLOR_PAIR(2));
                            }
                        }
                        
                        wrefresh(act_win);
                        int a_key = wgetch(act_win);
                        if (a_key == 27) {
                            break;
                        }
                        else if (a_key == KEY_UP) {
                            opt_idx = (opt_idx - 1 + opts.size()) % opts.size();
                        }
                        else if (a_key == KEY_DOWN) {
                            opt_idx = (opt_idx + 1) % opts.size();
                        }
                        else if (a_key == 10 || a_key == 13 || a_key == KEY_ENTER) {
                            if (opt_idx == 0) { 
                                std::vector<std::string> clone_history = {
                                    "Starting: git clone " + selected.html_url
                                };
                                wclear(act_win);
                                wattron(act_win, COLOR_PAIR(1) | A_BOLD);
                                box(act_win, 0, 0);
                                safe_addstr(act_win, 0, (mw - 15) / 2, " Cloning... ", COLOR_PAIR(1) | A_BOLD);
                                wattroff(act_win, COLOR_PAIR(1) | A_BOLD);
                                safe_addstr(act_win, 3, 4, "Running: git clone inside workspace...", COLOR_PAIR(5) | A_BOLD | A_BLINK);
                                wrefresh(act_win);
                                
                                std::string cmd = "git clone \"https://github.com/" + selected.owner + "/" + selected.name + ".git\" 2>&1";
                                FILE* p_clone = popen(cmd.c_str(), "r");
                                if (p_clone) {
                                    char c_buf[512];
                                    while (fgets(c_buf, sizeof(c_buf), p_clone) != nullptr) {
                                        std::string cl(c_buf);
                                        if (!cl.empty() && cl.back() == '\n') cl.pop_back();
                                        if (!cl.empty() && cl.back() == '\r') cl.pop_back();
                                        clone_history.push_back(cl);
                                    }
                                    pclose(p_clone);
                                }
                                
                                load_explorer_files();
                                
                                std::vector<std::string> modal_lines;
                                for (size_t k = std::max((size_t)0, clone_history.size() - 8); k < clone_history.size(); ++k) {
                                    modal_lines.push_back(clone_history[k]);
                                }
                                show_modal(repos_win, "Git Cloning Complete", modal_lines);
                                break;
                            }
                            else if (opt_idx == 1) { 
                                show_readme_modal(repos_win, selected.owner, selected.name);
                                break;
                            }
                            else if (opt_idx == 2) { 
                                run_remote_file_browser(repos_win, selected.owner, selected.name);
                                break;
                            }
                            else if (opt_idx == 3) { 
                                std::string cmd = "xdg-open \"" + selected.html_url + "\" >/dev/null 2>&1 &";
                                int rc = std::system(cmd.c_str());
                                show_modal(repos_win, "Browser Request Sent", {"Successfully requested browser startup:", selected.html_url});
                                break;
                            }
                            else { 
                                break;
                            }
                        }
                    }
                    delwin(act_win);
                }
            }
        }
    }
    
    delwin(repos_win);
    nodelay(parent, TRUE);
}
