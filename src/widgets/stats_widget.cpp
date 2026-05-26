#include "stats_widget.hpp"
#include "../globals.hpp"
#include "../utils.hpp"
#include "../system_monitor.hpp"
#include <sstream>
#include <iomanip>

void draw_stats_widget(WINDOW* win, bool focused) {
    draw_box(win, "SYSTEM MONITOR", focused);
    int h, w;
    getmaxyx(win, h, w);
    
    int cpu = get_cpu_usage();
    int ram_pct; double ram_used, ram_total; get_ram_usage(ram_pct, ram_used, ram_total);
    int disk_pct; double disk_used, disk_total; get_disk_usage(disk_pct, disk_used, disk_total);
    std::string uptime = get_uptime();
    
    auto get_bar = [](int pct, int width) {
        if (pct < 0) pct = 0;
        if (pct > 100) pct = 100;
        int filled = (pct * width) / 100;
        return std::string(filled, '#') + std::string(width - filled, ' ');
    };
    
    int bar_w = std::min(15, w - 14);
    if (bar_w < 5) bar_w = 5;
    
    safe_addstr(win, 2, 2, "CPU Usage: " + std::to_string(cpu) + "%", COLOR_PAIR(3) | A_BOLD);
    safe_addstr(win, 3, 2, "[" + get_bar(cpu, bar_w) + "]", COLOR_PAIR(1));
    
    safe_addstr(win, 5, 2, "RAM Stats: " + std::to_string(ram_pct) + "%", COLOR_PAIR(3) | A_BOLD);
    std::stringstream ss_ram; ss_ram << "[" << get_bar(ram_pct, bar_w) << "] " << std::fixed << std::setprecision(1) << ram_used << "/" << ram_total << "GB";
    safe_addstr(win, 6, 2, ss_ram.str(), COLOR_PAIR(1));
    
    safe_addstr(win, 8, 2, "Disk Space: " + std::to_string(disk_pct) + "%", COLOR_PAIR(3) | A_BOLD);
    std::stringstream ss_disk; ss_disk << "[" << get_bar(disk_pct, bar_w) << "] " << std::fixed << std::setprecision(0) << disk_used << "/" << disk_total << "GB";
    safe_addstr(win, 9, 2, ss_disk.str(), COLOR_PAIR(1));
    
    safe_addstr(win, 11, 2, "System Uptime:", COLOR_PAIR(3) | A_BOLD);
    safe_addstr(win, 12, 2, "* " + uptime, COLOR_PAIR(5) | A_BOLD);
}
