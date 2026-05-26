#include "splashscreen.hpp"
#include "globals.hpp"
#include "utils.hpp"
#include <chrono>
#include <thread>
#include <algorithm>

void run_splashscreen() {
    clear();
    std::vector<std::string> logo_lines = read_logo();
    
    std::vector<std::string> steps = {
        "Initializing Tiwut TUI...",
        "Checking system sensors...",
        "Establishing kernel link...",
        "Spawning desktop environments...",
        "Welcome to Tiwut CLI!"
    };
    
    double total_duration = 2.0;
    auto start_time = std::chrono::steady_clock::now();
    
    while (true) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time
        ).count() / 1000.0;
        
        double pct = std::min(1.0, elapsed / total_duration);
        
        int h, w;
        getmaxyx(stdscr, h, w);
        int logo_h = logo_lines.size();
        int logo_w = 0;
        for (const auto& line : logo_lines) {
            if ((int)line.length() > logo_w) logo_w = line.length();
        }
        
        int start_y = std::max(1, (h - logo_h - 4) / 2);
        int start_x = std::max(1, (w - logo_w) / 2);
        
        erase();
        
        
        for (size_t idx = 0; idx < logo_lines.size(); ++idx) {
            int color;
            if (idx < 4) color = COLOR_PAIR(3);
            else if (idx < 10) color = COLOR_PAIR(1);
            else color = COLOR_PAIR(2);
            
            safe_addstr(stdscr, start_y + idx, start_x, logo_lines[idx], color | A_BOLD);
        }
        
        
        int bar_y = start_y + logo_h + 1;
        int bar_w = std::min(40, w - 10);
        int filled_w = (int)(pct * bar_w);
        int empty_w = bar_w - filled_w;
        
        std::string bar_str = "[" + std::string(filled_w, '#') + std::string(empty_w, ' ') + "]";
        int bar_x = std::max(1, (w - (int)bar_str.length()) / 2);
        safe_addstr(stdscr, bar_y, bar_x, bar_str, COLOR_PAIR(1) | A_BOLD);
        
        
        int step_idx = std::min((int)steps.size() - 1, (int)(pct * steps.size()));
        std::string status_text = " " + steps[step_idx] + " " + std::to_string((int)(pct * 100)) + "% ";
        int status_x = std::max(1, (w - (int)status_text.length()) / 2);
        safe_addstr(stdscr, bar_y + 1, status_x, status_text, COLOR_PAIR(5));
        
        refresh();
        
        if (pct >= 1.0) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}
