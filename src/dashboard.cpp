#include "dashboard.hpp"
#include "globals.hpp"
#include "utils.hpp"
#include "menus.hpp"
#include "widgets/stats_widget.hpp"
#include "widgets/notepad_widget.hpp"
#include "widgets/launcher_widget.hpp"
#include "widgets/explorer_widget.hpp"
#include "widgets/git_terminal.hpp"
#include "widgets/app_store.hpp"
#include <chrono>
#include <thread>
#include <cstring>
#include <algorithm>
#include <time.h>
#include <unistd.h>

void run_dashboard() {
    clear();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    curs_set(0);
    refresh();
    
    
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        explorer_dir = std::string(cwd);
    }
    load_explorer_files();
    
    int prev_h = 0, prev_w = 0;
    WINDOW* stats_win = nullptr;
    WINDOW* notepad_win = nullptr;
    WINDOW* launcher_win = nullptr;
    WINDOW* explorer_win = nullptr;
    WINDOW* max_win = nullptr;
    
    while (true) {
        int h, w;
        getmaxyx(stdscr, h, w);
        log_debug("Loop step. Screen size: h=" + std::to_string(h) + ", w=" + std::to_string(w));
        
        if (h < 24 || w < 80) {
            log_debug("Resize required active. h=" + std::to_string(h) + ", w=" + std::to_string(w));
            if (stats_win) { delwin(stats_win); stats_win = nullptr; }
            if (notepad_win) { delwin(notepad_win); notepad_win = nullptr; }
            if (launcher_win) { delwin(launcher_win); launcher_win = nullptr; }
            if (explorer_win) { delwin(explorer_win); explorer_win = nullptr; }
            if (max_win) { delwin(max_win); max_win = nullptr; }
            prev_h = 0; prev_w = 0;
            
            erase();
            std::string msg = " Please resize terminal to at least 80x24! ";
            safe_addstr(stdscr, h / 2, std::max(0, (w - (int)msg.length()) / 2), msg, COLOR_PAIR(4) | A_REVERSE | A_BOLD);
            refresh();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }
        
        
        int grid_h = h - 6;
        int half_h = grid_h / 2;
        int half_w = w / 2;
        
        
        if (h != prev_h || w != prev_w) {
            log_debug("Recreating windows. grid_h=" + std::to_string(grid_h) + ", half_h=" + std::to_string(half_h) + ", half_w=" + std::to_string(half_w));
            if (stats_win) delwin(stats_win);
            if (notepad_win) delwin(notepad_win);
            if (launcher_win) delwin(launcher_win);
            if (explorer_win) delwin(explorer_win);
            if (max_win) delwin(max_win);
            
            stats_win = derwin(stdscr, half_h, half_w, 3, 0);
            notepad_win = derwin(stdscr, half_h, w - half_w, 3, half_w);
            launcher_win = derwin(stdscr, grid_h - half_h, half_w, 3 + half_h, 0);
            explorer_win = derwin(stdscr, grid_h - half_h, w - half_w, 3 + half_h, half_w);
            max_win = derwin(stdscr, grid_h, w, 3, 0);
            
            log_debug("Windows allocated: stats=" + std::to_string(stats_win != nullptr) +
                      ", notepad=" + std::to_string(notepad_win != nullptr) +
                      ", launcher=" + std::to_string(launcher_win != nullptr) +
                      ", explorer=" + std::to_string(explorer_win != nullptr) +
                      ", max=" + std::to_string(max_win != nullptr));
            
            prev_h = h;
            prev_w = w;
            clear();
        }
        
        erase();
        
        
        wattron(stdscr, COLOR_PAIR(3) | A_BOLD);
        mvhline(0, 0, '=', w);
        wattroff(stdscr, COLOR_PAIR(3) | A_BOLD);
        
        safe_addstr(stdscr, 1, 2, " * TiwutOS ", COLOR_PAIR(1) | A_BOLD);
        
        
        safe_addstr(stdscr, 1, 14, "[F1] File", active_menu == 1 ? (COLOR_PAIR(1) | A_REVERSE) : COLOR_PAIR(2));
        safe_addstr(stdscr, 1, 26, "[F2] Settings", active_menu == 2 ? (COLOR_PAIR(1) | A_REVERSE) : COLOR_PAIR(2));
        safe_addstr(stdscr, 1, 41, "[F3] Help", active_menu == 3 ? (COLOR_PAIR(1) | A_REVERSE) : COLOR_PAIR(2));
        safe_addstr(stdscr, 1, 53, "[F4] Dev", active_menu == 4 ? (COLOR_PAIR(1) | A_REVERSE) : COLOR_PAIR(2));
        
        std::string theme_str = "Theme: " + themes[current_theme].name;
        if (maximized) theme_str += " [MAXIMIZED]";
        safe_addstr(stdscr, 1, half_w + 10, theme_str, COLOR_PAIR(5));
        
        time_t t = time(nullptr);
        struct tm* tm_info = localtime(&t);
        char date_buf[64];
        strftime(date_buf, sizeof(date_buf), "%Y-%m-%d %H:%M:%S", tm_info);
        safe_addstr(stdscr, 1, w - (int)strlen(date_buf) - 2, date_buf, COLOR_PAIR(3) | A_BOLD);
        
        wattron(stdscr, COLOR_PAIR(2));
        mvhline(2, 0, '-', w);
        wattroff(stdscr, COLOR_PAIR(2));
        
        
        draw_dock(stdscr, w, h);
        
        
        if (maximized) {
            if (max_win) {
                if (active_pane == 0) draw_stats_widget(max_win, true);
                else if (active_pane == 1) draw_notepad_widget(max_win, true);
                else if (active_pane == 2) draw_launcher_widget(max_win, true);
                else if (active_pane == 3) draw_explorer_widget(max_win, true);
            }
        } else {
            if (stats_win) draw_stats_widget(stats_win, active_pane == 0);
            if (notepad_win) draw_notepad_widget(notepad_win, active_pane == 1);
            if (launcher_win) draw_launcher_widget(launcher_win, active_pane == 2);
            if (explorer_win) draw_explorer_widget(explorer_win, active_pane == 3);
        }
        
        
        draw_top_menus(stdscr);
        
        
        int key = getch();
        if (key != -1) {
            log_debug("getch() key received: " + std::to_string(key));
        }
        
        
        if (active_menu != 0) { 
            int items_count = (active_menu == 1) ? 4 : ((active_menu == 4) ? 3 : 3);
            
            if (key == 27) { 
                active_menu = 0;
            }
            else if (key == KEY_UP) {
                menu_idx = (menu_idx - 1 + items_count) % items_count;
            }
            else if (key == KEY_DOWN) {
                menu_idx = (menu_idx + 1) % items_count;
            }
            else if (key == KEY_LEFT) { 
                active_menu = (active_menu == 1) ? 4 : (active_menu - 1);
                menu_idx = 0;
            }
            else if (key == KEY_RIGHT) { 
                active_menu = (active_menu == 4) ? 1 : (active_menu + 1);
                menu_idx = 0;
            }
            else if (key == KEY_F(1)) {
                active_menu = (active_menu == 1) ? 0 : 1;
                menu_idx = 0;
            }
            else if (key == KEY_F(2)) {
                active_menu = (active_menu == 2) ? 0 : 2;
                menu_idx = 0;
            }
            else if (key == KEY_F(3)) {
                active_menu = (active_menu == 3) ? 0 : 3;
                menu_idx = 0;
            }
            else if (key == KEY_F(4)) {
                active_menu = (active_menu == 4) ? 0 : 4;
                menu_idx = 0;
            }
            else if (key == 10 || key == 13 || key == KEY_ENTER) {
                
                if (active_menu == 1) { 
                    if (menu_idx == 0) { 
                        if (active_pane == 3 && explorer_idx < (int)explorer_files.size()) {
                            std::string sel = explorer_files[explorer_idx];
                            if (sel.back() != '/') {
                                view_file_modal(stdscr, explorer_dir + "/" + sel);
                            }
                        } else {
                            show_modal(stdscr, "Explorer Required", {"Please switch to the File Explorer widget", "to preview selected documents."});
                        }
                    }
                    else if (menu_idx == 1) { 
                        show_system_info_modal(stdscr);
                    }
                    else if (menu_idx == 2) { 
                        notepad_lines = {""};
                        notepad_y = 0;
                        notepad_x = 0;
                        save_notes();
                        show_modal(stdscr, "Scratchpad Reset", {"Cleaned notepad scratch space successfully."});
                    }
                    else if (menu_idx == 3) { 
                        break;
                    }
                }
                else if (active_menu == 2) { 
                    if (menu_idx == 0) { 
                        current_theme = (current_theme + 1) % themes.size();
                        init_theme_colors();
                    }
                    else if (menu_idx == 1) { 
                        maximized = !maximized;
                    }
                    else if (menu_idx == 2) { 
                        maximized = false;
                        active_pane = 0;
                    }
                }
                else if (active_menu == 3) { 
                    if (menu_idx == 0) { 
                        std::string cmd = "xdg-open \"https://tiwut.org/\" >/dev/null 2>&1 &";
                        int rc = std::system(cmd.c_str());
                    }
                    else if (menu_idx == 1) { 
                        std::string cmd = "xdg-open \"https://github.com/tiwut\" >/dev/null 2>&1 &";
                        int rc = std::system(cmd.c_str());
                    }
                    else if (menu_idx == 2) { 
                        show_modal(stdscr, "About Tiwut TUI", {
                            "Tiwut Desktop Terminal Dashboard v1.0.0",
                            "-----------------------------------------",
                            "Designed exclusively for digital autonomy.",
                            "",
                            "Created by: Tiwut",
                            "Website: https://tiwut.org/",
                            "GitHub: https://github.com/tiwut",
                            "",
                            "Pure Code. Total Freedom."
                        });
                    }
                }
                else if (active_menu == 4) { 
                    if (menu_idx == 0) { 
                        run_integrated_terminal(stdscr);
                    }
                    else if (menu_idx == 1) { 
                        run_github_repos_menu(stdscr);
                    }
                    else if (menu_idx == 2) { 
                        run_app_store(stdscr);
                    }
                }
                active_menu = 0; 
            }
        }
        else { 
            if (key == 'q' || key == 'Q') {
                log_debug("Exiting because 'q' or 'Q' pressed");
                break;
            }
            else if (key == 9) { 
                active_pane = (active_pane + 1) % 4;
                curs_set(0);
            }
            else if (key == 'c' || key == 'C') {
                current_theme = (current_theme + 1) % themes.size();
                init_theme_colors();
            }
            else if (key == 'm' || key == 'M') { 
                maximized = !maximized;
            }
            else if (key == KEY_F(1)) {
                active_menu = 1;
                menu_idx = 0;
            }
            else if (key == KEY_F(2)) {
                active_menu = 2;
                menu_idx = 0;
            }
            else if (key == KEY_F(3)) {
                active_menu = 3;
                menu_idx = 0;
            }
            else if (key == KEY_F(4)) {
                active_menu = 4;
                menu_idx = 0;
            }
            
            
            else if (active_pane == 1) { 
                if (key >= 32 && key <= 126) {
                    std::string& line = notepad_lines[notepad_y];
                    line.insert(notepad_x, 1, (char)key);
                    notepad_x++;
                    save_notes();
                }
                else if (key == 10 || key == 13 || key == KEY_ENTER) {
                    std::string& line = notepad_lines[notepad_y];
                    std::string rest = line.substr(notepad_x);
                    line = line.substr(0, notepad_x);
                    notepad_lines.insert(notepad_lines.begin() + notepad_y + 1, rest);
                    notepad_y++;
                    notepad_x = 0;
                    save_notes();
                }
                else if (key == KEY_BACKSPACE || key == 127 || key == 8) {
                    if (notepad_x > 0) {
                        std::string& line = notepad_lines[notepad_y];
                        line.erase(notepad_x - 1, 1);
                        notepad_x--;
                        save_notes();
                    } else if (notepad_y > 0) {
                        int prev_len = notepad_lines[notepad_y - 1].length();
                        notepad_lines[notepad_y - 1] += notepad_lines[notepad_y];
                        notepad_lines.erase(notepad_lines.begin() + notepad_y);
                        notepad_y--;
                        notepad_x = prev_len;
                        save_notes();
                    }
                }
                else if (key == KEY_UP) {
                    if (notepad_y > 0) {
                        notepad_y--;
                        notepad_x = std::min(notepad_x, (int)notepad_lines[notepad_y].length());
                    }
                }
                else if (key == KEY_DOWN) {
                    if (notepad_y < (int)notepad_lines.size() - 1) {
                        notepad_y++;
                        notepad_x = std::min(notepad_x, (int)notepad_lines[notepad_y].length());
                    }
                }
                else if (key == KEY_LEFT) {
                    if (notepad_x > 0) {
                        notepad_x--;
                    } else if (notepad_y > 0) {
                        notepad_y--;
                        notepad_x = notepad_lines[notepad_y].length();
                    }
                }
                else if (key == KEY_RIGHT) {
                    if (notepad_x < (int)notepad_lines[notepad_y].length()) {
                        notepad_x++;
                    } else if (notepad_y < (int)notepad_lines.size() - 1) {
                        notepad_y++;
                        notepad_x = 0;
                    }
                }
            }
            else if (active_pane == 2) { 
                if (key == KEY_UP) {
                    launcher_idx = (launcher_idx - 1 + loaded_apps.size()) % loaded_apps.size();
                }
                else if (key == KEY_DOWN) {
                    launcher_idx = (launcher_idx + 1) % loaded_apps.size();
                }
                else if (key == 10 || key == 13 || key == KEY_ENTER) {
                    if (launcher_idx < (int)loaded_apps.size()) {
                        AppIcon item = loaded_apps[launcher_idx];
                        if (item.exec == "theme") {
                            current_theme = (current_theme + 1) % themes.size();
                            init_theme_colors();
                        }
                        else if (item.exec == "about") {
                            show_modal(stdscr, "About Tiwut TUI", {
                                "Tiwut Desktop Terminal Dashboard v1.0.0",
                                "-----------------------------------------",
                                "Designed exclusively for digital autonomy.",
                                "",
                                "Created by: Tiwut",
                                "Website: https://tiwut.org/",
                                "GitHub: https://github.com/tiwut",
                                "",
                                "Pure Code. Total Freedom."
                            });
                        }
                        else if (!item.exec.empty()) {
                            std::string cmd = item.exec + " >/dev/null 2>&1 &";
                            int rc = std::system(cmd.c_str());
                            show_modal(stdscr, "App Launched", {"Successfully requested execution:", item.name, "Command: " + item.exec});
                        }
                    }
                }
            }
            else if (active_pane == 3) { 
                if (key == KEY_UP) {
                    if (explorer_idx > 0) explorer_idx--;
                }
                else if (key == KEY_DOWN) {
                    if (explorer_idx < (int)explorer_files.size() - 1) explorer_idx++;
                }
                else if (key == KEY_BACKSPACE || key == 127 || key == 8 || key == 'b' || key == 'B') {
                    char resolved[2048];
                    std::string parent = explorer_dir + "/..";
                    if (realpath(parent.c_str(), resolved) != nullptr) {
                        explorer_dir = std::string(resolved);
                        explorer_idx = 0;
                        load_explorer_files();
                    }
                }
                else if (key == 10 || key == 13 || key == KEY_ENTER) {
                    if (explorer_idx < (int)explorer_files.size()) {
                        std::string sel = explorer_files[explorer_idx];
                        std::string full_path = explorer_dir + "/" + sel;
                        
                        if (sel.back() == '/') {
                            char resolved[2048];
                            if (realpath(full_path.c_str(), resolved) != nullptr) {
                                explorer_dir = std::string(resolved);
                                explorer_idx = 0;
                                load_explorer_files();
                            }
                        } else {
                            view_file_modal(stdscr, full_path);
                        }
                    }
                }
            }
        }
        
        wrefresh(stdscr);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
    }
    
    
    if (stats_win) delwin(stats_win);
    if (notepad_win) delwin(notepad_win);
    if (launcher_win) delwin(launcher_win);
    if (explorer_win) delwin(explorer_win);
    if (max_win) delwin(max_win);
}
