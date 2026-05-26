#include "app_store.hpp"
#include "launcher_widget.hpp"
#include "../globals.hpp"
#include "../utils.hpp"
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <unistd.h>
#include <cstdlib>


struct AppStoreEntry {
    std::string id;
    std::string name;
    std::string icon;
    std::string version;
    std::string description;
    std::string supported_os;
    std::string dependencies;
    std::string download_url;
    std::string sha256;
    std::string install_linux;
    std::string install_macos;
    std::string exec;
};


std::vector<AppStoreEntry> parse_app_store_json(const std::string& json_data) {
    std::vector<AppStoreEntry> entries;
    size_t pos = 0;
    
    while (true) {
        size_t next_obj = json_data.find("\"id\":", pos);
        if (next_obj == std::string::npos) break;
        
        AppStoreEntry entry;
        
        auto get_str_val = [&](const std::string& key) -> std::string {
            size_t start_pos = next_obj > 30 ? next_obj - 30 : 0;
            size_t k_pos = json_data.find("\"" + key + "\"", start_pos);
            size_t limit = json_data.find("\"id\":", next_obj + 1);
            if (k_pos == std::string::npos || (limit != std::string::npos && k_pos > limit)) return "";
            
            size_t colon = json_data.find(":", k_pos);
            if (colon == std::string::npos) return "";
            size_t q1 = json_data.find("\"", colon);
            if (q1 == std::string::npos) return "";
            size_t q2 = json_data.find("\"", q1 + 1);
            if (q2 == std::string::npos) return "";
            return json_data.substr(q1 + 1, q2 - q1 - 1);
        };
        
        entry.id = get_str_val("id");
        entry.name = get_str_val("name");
        entry.icon = get_str_val("icon");
        entry.version = get_str_val("version");
        entry.description = get_str_val("description");
        entry.supported_os = get_str_val("supported_os");
        entry.dependencies = get_str_val("dependencies");
        entry.download_url = get_str_val("download_url");
        entry.sha256 = get_str_val("sha256");
        entry.install_linux = get_str_val("install_linux");
        entry.install_macos = get_str_val("install_macos");
        entry.exec = get_str_val("exec");
        
        if (!entry.id.empty()) {
            entries.push_back(entry);
        }
        pos = next_obj + 1;
    }
    return entries;
}


std::string fetch_store_api(const std::string& url) {
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


std::string read_local_file(const std::string& filepath) {
    std::ifstream f(filepath);
    if (!f.is_open()) return "";
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    return content;
}


std::string get_host_os() {
#ifdef __APPLE__
    return "macos";
#else
    return "linux";
#endif
}


bool check_system_dependency(const std::string& dep) {
    std::string cmd = "command -v " + dep + " >/dev/null 2>&1";
    int rc = std::system(cmd.c_str());
    return (rc == 0);
}


std::string get_registry_filepath() {
    char* home = getenv("HOME");
    std::string home_dir = home ? std::string(home) : "";
    return home_dir + "/.local/share/tiwut-cli/installed_apps.txt";
}

std::string get_installed_version(const std::string& app_id) {
    std::string filepath = get_registry_filepath();
    std::ifstream f(filepath);
    if (!f.is_open()) return "";
    
    std::string line;
    while (std::getline(f, line)) {
        std::stringstream ss(line);
        std::string id, ver;
        if (ss >> id >> ver) {
            if (id == app_id) return ver;
        }
    }
    return "";
}

void set_installed_version(const std::string& app_id, const std::string& version) {
    std::string filepath = get_registry_filepath();
    std::vector<std::pair<std::string, std::string>> entries;
    
    
    std::ifstream f_in(filepath);
    if (f_in.is_open()) {
        std::string line;
        while (std::getline(f_in, line)) {
            std::stringstream ss(line);
            std::string id, ver;
            if (ss >> id >> ver) {
                if (id != app_id) {
                    entries.push_back({id, ver});
                }
            }
        }
        f_in.close();
    }
    
    
    entries.push_back({app_id, version});
    
    
    char* home = getenv("HOME");
    if (home) {
        std::string dir = std::string(home) + "/.local/share/tiwut-cli";
        std::string mkdir_cmd = "mkdir -p \"" + dir + "\" >/dev/null 2>&1";
        int rc = std::system(mkdir_cmd.c_str());
        (void)rc;
    }
    
    
    std::ofstream f_out(filepath);
    if (f_out.is_open()) {
        for (const auto& entry : entries) {
            f_out << entry.first << " " << entry.second << "\n";
        }
    }
}


std::string calculate_file_sha256(const std::string& file_path) {
    std::string cmd;
#ifdef __APPLE__
    cmd = "shasum -a 256 \"" + file_path + "\" 2>/dev/null";
#else
    cmd = "sha256sum \"" + file_path + "\" 2>/dev/null";
#endif
    
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    
    char buffer[1024];
    std::string result = "";
    if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result = buffer;
    }
    pclose(pipe);
    
    size_t space = result.find_first_of(" \t\n\r");
    if (space != std::string::npos) {
        return result.substr(0, space);
    }
    return result;
}


void run_app_store(WINDOW* parent) {
    int ph, pw;
    getmaxyx(parent, ph, pw);
    
    std::vector<AppStoreEntry> apps;
    int active_idx = 0;
    
    
    auto load_catalog = [&](WINDOW* p) -> bool {
        int lw = 50, lh = 5;
        WINDOW* load_win = derwin(p, lh, lw, (ph - lh) / 2, (pw - lw) / 2);
        if (load_win) {
            wattron(load_win, COLOR_PAIR(1) | A_BOLD);
            box(load_win, 0, 0);
            safe_addstr(load_win, 2, 4, "Connecting to Remote App Store...", COLOR_PAIR(5) | A_BOLD);
            wattroff(load_win, COLOR_PAIR(1) | A_BOLD);
            wrefresh(load_win);
            delwin(load_win);
        }
        
        std::string url = "https://raw.githubusercontent.com/tiwut/Tiwut-CLI/refs/heads/main/Remote/AppStore.json";
        std::string data = fetch_store_api(url);
        
        
        if (data.empty() || data.rfind("404", 0) == 0) {
            char* home = getenv("HOME");
            std::string home_dir = home ? std::string(home) : "";
            std::vector<std::string> candidates = {
                "./Remote/AppStore.json",
                home_dir + "/.local/share/tiwut-cli/Remote/AppStore.json",
                "Remote/AppStore.json"
            };
            for (const auto& path : candidates) {
                data = read_local_file(path);
                if (!data.empty()) break;
            }
        }
        
        apps = parse_app_store_json(data);
        return !apps.empty();
    };
    
    if (!load_catalog(parent)) {
        show_modal(parent, "Connection Error", {
            "Failed to retrieve App Store Catalog.",
            "Please check your internet connection or",
            "verify local 'Remote/AppStore.json' exists."
        });
        return;
    }
    
    WINDOW* store_win = derwin(parent, ph - 4, pw - 4, 2, 2);
    if (!store_win) return;
    
    nodelay(store_win, FALSE);
    keypad(store_win, TRUE);
    
    while (true) {
        wclear(store_win);
        wattron(store_win, COLOR_PAIR(1) | A_BOLD);
        box(store_win, 0, 0);
        std::string title = " TIWUT APP STORE (REMOTE CATALOG) ";
        safe_addstr(store_win, 0, (pw - 4 - (int)title.length()) / 2, title, COLOR_PAIR(1) | A_BOLD);
        wattroff(store_win, COLOR_PAIR(1) | A_BOLD);
        
        std::string footer = " UP/DOWN: Select | ENTER/I: Install/Upgrade | R: Reload Store | ESC: Exit ";
        safe_addstr(store_win, ph - 5, (pw - 4 - (int)footer.length()) / 2, footer, COLOR_PAIR(5) | A_REVERSE);
        
        
        int list_w = 40;
        int list_h = ph - 8;
        
        wattron(store_win, COLOR_PAIR(2));
        for (int y = 1; y < ph - 5; ++y) {
            mvwaddch(store_win, y, list_w, ACS_VLINE);
        }
        wattroff(store_win, COLOR_PAIR(2));
        
        if (active_idx >= (int)apps.size()) active_idx = (int)apps.size() - 1;
        if (active_idx < 0) active_idx = 0;
        
        int visible_apps = ph - 9;
        int start = std::max(0, active_idx - visible_apps + 1);
        
        for (int idx = 0; idx < visible_apps; ++idx) {
            int app_idx = start + idx;
            if (app_idx < (int)apps.size()) {
                const auto& app = apps[app_idx];
                int y = 2 + idx * 2;
                
                std::string installed_ver = get_installed_version(app.id);
                std::string status_badge = "[Available]";
                int badge_color = COLOR_PAIR(3);
                
                if (!installed_ver.empty()) {
                    if (installed_ver == app.version) {
                        status_badge = "[Installed]";
                        badge_color = COLOR_PAIR(5); 
                    } else {
                        status_badge = "[UpdateAvail]";
                        badge_color = COLOR_PAIR(5) | A_BOLD | A_BLINK; 
                    }
                }
                
                std::string app_lbl = app.icon + " " + app.name;
                if (app_lbl.length() > 22) app_lbl = app_lbl.substr(0, 19) + "...";
                
                if (app_idx == active_idx) {
                    safe_addstr(store_win, y, 2, " >> " + app_lbl, COLOR_PAIR(1) | A_REVERSE | A_BOLD);
                    safe_addstr(store_win, y, list_w - 14, status_badge, badge_color | A_REVERSE | A_BOLD);
                } else {
                    safe_addstr(store_win, y, 2, "    " + app_lbl, COLOR_PAIR(2));
                    safe_addstr(store_win, y, list_w - 14, status_badge, badge_color);
                }
            }
        }
        
        
        if (!apps.empty() && active_idx >= 0 && active_idx < (int)apps.size()) {
            const auto& app = apps[active_idx];
            int rx = list_w + 3;
            
            
            safe_addstr(store_win, 2, rx, app.name + " (" + app.id + ")", COLOR_PAIR(1) | A_BOLD);
            safe_addstr(store_win, 3, rx, "Catalog Version: v" + app.version, COLOR_PAIR(5) | A_BOLD);
            
            std::string local_ver = get_installed_version(app.id);
            if (!local_ver.empty()) {
                safe_addstr(store_win, 4, rx, "Installed version: v" + local_ver, COLOR_PAIR(2));
            } else {
                safe_addstr(store_win, 4, rx, "Installed version: None", COLOR_PAIR(2));
            }
            
            
            wattron(store_win, COLOR_PAIR(2));
            mvwhline(store_win, 5, rx, ACS_HLINE, pw - list_w - 10);
            wattroff(store_win, COLOR_PAIR(2));
            
            
            safe_addstr(store_win, 7, rx, "Description:", COLOR_PAIR(3) | A_BOLD);
            int desc_y = 8;
            size_t d_pos = 0;
            int max_desc_w = pw - list_w - 10;
            while (d_pos < app.description.length() && desc_y < 12) {
                std::string chunk = app.description.substr(d_pos, max_desc_w);
                safe_addstr(store_win, desc_y++, rx + 2, chunk, COLOR_PAIR(2));
                d_pos += max_desc_w;
            }
            
            
            safe_addstr(store_win, 13, rx, "Supported Platforms:", COLOR_PAIR(3) | A_BOLD);
            std::string host_os = get_host_os();
            bool os_match = (app.supported_os.find(host_os) != std::string::npos);
            int os_color = os_match ? (COLOR_PAIR(5) | A_BOLD) : (COLOR_PAIR(1) | A_BOLD);
            std::string os_status = app.supported_os + (os_match ? " (COMPATIBLE)" : " (UNSUPPORTED ON THIS OS)");
            safe_addstr(store_win, 14, rx + 2, os_status, os_color);
            
            
            safe_addstr(store_win, 16, rx, "System Dependencies:", COLOR_PAIR(3) | A_BOLD);
            std::stringstream ss(app.dependencies);
            std::string dep;
            int dep_y = 17;
            bool all_deps_met = true;
            std::vector<std::string> missing_deps;
            
            while (std::getline(ss, dep, ',')) {
                if (dep.empty()) continue;
                bool met = check_system_dependency(dep);
                if (!met) {
                    all_deps_met = false;
                    missing_deps.push_back(dep);
                }
                std::string dep_lbl = " - " + dep + (met ? " [OK]" : " [MISSING]");
                int dep_col = met ? COLOR_PAIR(5) : (COLOR_PAIR(1) | A_BOLD);
                safe_addstr(store_win, dep_y++, rx + 2, dep_lbl, dep_col);
            }
            if (app.dependencies.empty()) {
                safe_addstr(store_win, 17, rx + 2, " - None required [OK]", COLOR_PAIR(5));
            }
            
            
            safe_addstr(store_win, 20, rx, "Secure SHA-256 Integrity Verification:", COLOR_PAIR(3) | A_BOLD);
            safe_addstr(store_win, 21, rx + 2, app.sha256, COLOR_PAIR(2));
            
            
            if (!app.exec.empty()) {
                safe_addstr(store_win, 23, rx, "Dashboard Command Integration:", COLOR_PAIR(3) | A_BOLD);
                safe_addstr(store_win, 24, rx + 2, "Launches: " + app.exec, COLOR_PAIR(2));
            }
        }
        
        wrefresh(store_win);
        int key = wgetch(store_win);
        
        if (key == 27 || key == 'q' || key == 'Q') { 
            break;
        }
        else if (key == KEY_UP) {
            if (active_idx > 0) active_idx--;
        }
        else if (key == KEY_DOWN) {
            if (active_idx < (int)apps.size() - 1) active_idx++;
        }
        else if (key == 'r' || key == 'R') { 
            load_catalog(store_win);
            active_idx = 0;
        }
        else if (key == 10 || key == 13 || key == KEY_ENTER || key == 'i' || key == 'I') {
            if (!apps.empty() && active_idx >= 0 && active_idx < (int)apps.size()) {
                const auto& app = apps[active_idx];
                
                
                std::string host_os = get_host_os();
                if (app.supported_os.find(host_os) == std::string::npos) {
                    show_modal(store_win, "OS Compatibility Error", {
                        "This application is not supported on this platform.",
                        "Required: " + app.supported_os,
                        "Your OS : " + host_os
                    });
                    continue;
                }
                
                
                std::stringstream ss(app.dependencies);
                std::string dep;
                std::vector<std::string> missing;
                while (std::getline(ss, dep, ',')) {
                    if (dep.empty()) continue;
                    if (!check_system_dependency(dep)) {
                        missing.push_back(dep);
                    }
                }
                if (!missing.empty()) {
                    std::vector<std::string> err_lines = {
                        "Cannot install due to missing dependencies:",
                        "Please install the following commands first:"
                    };
                    for (const auto& m : missing) {
                        err_lines.push_back(" - " + m);
                    }
                    show_modal(store_win, "Missing Dependencies", err_lines);
                    continue;
                }
                
                
                int lw = 50, lh = 6;
                WINDOW* progress_win = derwin(store_win, lh, lw, (ph - 4 - lh) / 2, (pw - 4 - lw) / 2);
                if (progress_win) {
                    wattron(progress_win, COLOR_PAIR(1) | A_BOLD);
                    box(progress_win, 0, 0);
                    safe_addstr(progress_win, 2, 4, "Downloading script...", COLOR_PAIR(5) | A_BOLD | A_BLINK);
                    wattroff(progress_win, COLOR_PAIR(1) | A_BOLD);
                    wrefresh(progress_win);
                }
                
                char* home = getenv("HOME");
                std::string home_dir = home ? std::string(home) : "";
                std::string tmp_dir = home_dir + "/.local/share/tiwut-cli/tmp";
                std::system(("mkdir -p \"" + tmp_dir + "\" >/dev/null 2>&1").c_str());
                
                
                std::string filename = "installer.sh";
                size_t last_slash = app.download_url.find_last_of('/');
                if (last_slash != std::string::npos) {
                    filename = app.download_url.substr(last_slash + 1);
                }
                
                std::string dest_path = tmp_dir + "/" + filename;
                std::string dl_cmd = "curl -sL -o \"" + dest_path + "\" \"" + app.download_url + "\"";
                int dl_rc = std::system(dl_cmd.c_str());
                
                if (progress_win) {
                    wclear(progress_win);
                    wattron(progress_win, COLOR_PAIR(1) | A_BOLD);
                    box(progress_win, 0, 0);
                    safe_addstr(progress_win, 2, 4, "Verifying SHA-256 integrity...", COLOR_PAIR(3) | A_BOLD);
                    wattroff(progress_win, COLOR_PAIR(1) | A_BOLD);
                    wrefresh(progress_win);
                }
                
                std::string computed_hash = calculate_file_sha256(dest_path);
                
                if (dl_rc != 0 || computed_hash.empty() || computed_hash.length() < 10) {
                    if (progress_win) delwin(progress_win);
                    show_modal(store_win, "Download Error", {
                        "Failed to retrieve installer package files.",
                        "Verification hash extraction failed."
                    });
                    std::remove(dest_path.c_str());
                    continue;
                }
                
                
                if (computed_hash != app.sha256) {
                    if (progress_win) delwin(progress_win);
                    show_modal(store_win, "SECURITY MISMATCH EXCEPTION", {
                        "CRITICAL: Integrity Verification Failure!",
                        "Downloaded file checksum does NOT match expectations.",
                        "",
                        "Expected: " + app.sha256,
                        "Calculated: " + computed_hash,
                        "",
                        "Execution aborted. Script destroyed for safety."
                    });
                    std::remove(dest_path.c_str());
                    continue;
                }
                
                
                if (progress_win) {
                    wclear(progress_win);
                    wattron(progress_win, COLOR_PAIR(1) | A_BOLD);
                    box(progress_win, 0, 0);
                    safe_addstr(progress_win, 2, 4, "Security OK. Installing...", COLOR_PAIR(5) | A_BOLD);
                    wattroff(progress_win, COLOR_PAIR(1) | A_BOLD);
                    wrefresh(progress_win);
                    delwin(progress_win);
                }
                
                std::string install_cmd = (host_os == "macos") ? app.install_macos : app.install_linux;
                
                std::string run_cmd = "cd \"" + tmp_dir + "\" && " + install_cmd + " 2>&1";
                
                
                std::vector<std::string> install_history = {
                    "Executing: " + install_cmd
                };
                
                FILE* p_install = popen(run_cmd.c_str(), "r");
                if (p_install) {
                    char c_buf[512];
                    while (fgets(c_buf, sizeof(c_buf), p_install) != nullptr) {
                        std::string line(c_buf);
                        if (!line.empty() && line.back() == '\n') line.pop_back();
                        if (!line.empty() && line.back() == '\r') line.pop_back();
                        install_history.push_back(line);
                    }
                    pclose(p_install);
                }
                
                
                set_installed_version(app.id, app.version);
                
                
                std::remove(dest_path.c_str());
                
                
                std::vector<std::string> modal_lines;
                modal_lines.push_back("Package " + app.name + " installed successfully.");
                modal_lines.push_back("Registry status updated cleanly.");
                modal_lines.push_back("------------------------------------");
                for (size_t k = std::max((size_t)0, install_history.size() - 8); k < install_history.size(); ++k) {
                    modal_lines.push_back(install_history[k]);
                }
                show_modal(store_win, "Installation Output", modal_lines);
                
                
                
                std::vector<std::string> base_dirs = {
                    home_dir + "/.local/share/tiwut-cli/",
                    "./"
                };
                
                if (!app.exec.empty()) {
                    for (const auto& base : base_dirs) {
                        std::string desktop_dir = base + "desktop";
                        std::string shortcut_path = desktop_dir + "/" + app.id + ".json";
                        std::string idx_path = desktop_dir + "/index.json";
                        
                        
                        std::string mkdir_cmd = "mkdir -p \"" + desktop_dir + "\" >/dev/null 2>&1";
                        std::system(mkdir_cmd.c_str());
                        
                        
                        std::ofstream sc(shortcut_path);
                        if (sc.is_open()) {
                            sc << "{\n";
                            sc << "  \"name\": \"" << app.name << "\",\n";
                            sc << "  \"icon\": \"" << app.icon << "\",\n";
                            sc << "  \"exec\": \"" << app.exec << "\"\n";
                            sc << "}\n";
                            sc.close();
                        }
                        
                        
                        std::ifstream idx_in(idx_path);
                        std::string idx_data = "";
                        if (idx_in.is_open()) {
                            idx_data = std::string((std::istreambuf_iterator<char>(idx_in)), std::istreambuf_iterator<char>());
                            idx_in.close();
                        }
                        
                        std::string shortcut_rel = "desktop/" + app.id + ".json";
                        if (!idx_data.empty() && idx_data.find(shortcut_rel) == std::string::npos) {
                            size_t array_close = idx_data.find_last_of(']');
                            if (array_close != std::string::npos) {
                                size_t last_quote = idx_data.find_last_of('\"', array_close);
                                if (last_quote != std::string::npos) {
                                    idx_data.insert(last_quote + 1, ",\n    \"" + shortcut_rel + "\"");
                                    std::ofstream idx_out(idx_path);
                                    if (idx_out.is_open()) {
                                        idx_out << idx_data;
                                        idx_out.close();
                                    }
                                }
                            }
                        }
                    }
                    
                    
                    load_desktop_apps();
                }
            }
        }
    }
    
    delwin(store_win);
}
