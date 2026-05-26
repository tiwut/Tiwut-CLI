#include "system_monitor.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <sys/statvfs.h>


static long long prev_total = 0;
static long long prev_idle = 0;

int get_cpu_usage() {
    std::ifstream stat_file("/proc/stat");
    if (!stat_file.is_open()) return 0;
    
    std::string cpu_label;
    long long user, nice, system, idle, iowait, irq, softirq, steal;
    if (stat_file >> cpu_label >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal) {
        long long current_idle = idle + iowait;
        long long current_total = user + nice + system + idle + iowait + irq + softirq + steal;
        
        long long diff_total = current_total - prev_total;
        long long diff_idle = current_idle - prev_idle;
        
        prev_total = current_total;
        prev_idle = current_idle;
        
        if (diff_total == 0) return 0;
        return (int)(100 * (diff_total - diff_idle) / diff_total);
    }
    return 0;
}

void get_ram_usage(int& pct, double& used_gb, double& total_gb) {
    std::ifstream meminfo("/proc/meminfo");
    if (!meminfo.is_open()) { pct = 0; used_gb = 0; total_gb = 0; return; }
    
    std::string line;
    long long mem_total = 0, mem_free = 0, mem_avail = -1, cached = 0, buffers = 0;
    while (std::getline(meminfo, line)) {
        std::stringstream ss(line);
        std::string label;
        long long value;
        ss >> label >> value;
        if (label == "MemTotal:") mem_total = value;
        else if (label == "MemFree:") mem_free = value;
        else if (label == "MemAvailable:") mem_avail = value;
        else if (label == "Cached:") cached = value;
        else if (label == "Buffers:") buffers = value;
    }
    
    if (mem_total == 0) { pct = 0; used_gb = 0; total_gb = 0; return; }
    
    long long mem_used;
    if (mem_avail != -1) {
        mem_used = mem_total - mem_avail;
    } else {
        mem_used = mem_total - mem_free - cached - buffers;
    }
    
    pct = (int)(100 * mem_used / mem_total);
    used_gb = (double)mem_used / (1024.0 * 1024.0);
    total_gb = (double)mem_total / (1024.0 * 1024.0);
}

std::string get_uptime() {
    std::ifstream uptime_file("/proc/uptime");
    if (!uptime_file.is_open()) return "N/A";
    
    double uptime_seconds;
    if (uptime_file >> uptime_seconds) {
        int days = (int)(uptime_seconds / 86400.0);
        int hours = (int)(((long long)uptime_seconds % 86400) / 3600);
        int mins = (int)(((long long)uptime_seconds % 3600) / 60);
        
        if (days > 0) {
            return std::to_string(days) + "d " + std::to_string(hours) + "h " + std::to_string(mins) + "m";
        }
        return std::to_string(hours) + "h " + std::to_string(mins) + "m";
    }
    return "N/A";
}

void get_disk_usage(int& pct, double& used_gb, double& total_gb) {
    struct statvfs stat;
    if (statvfs("/", &stat) == 0 && stat.f_blocks > 0) {
        unsigned long long total = stat.f_blocks * stat.f_frsize;
        unsigned long long free = stat.f_bfree * stat.f_frsize;
        unsigned long long used = total - free;
        
        pct = (int)(100 * used / total);
        used_gb = (double)used / (1024.0 * 1024.0 * 1024.0);
        total_gb = (double)total / (1024.0 * 1024.0 * 1024.0);
    } else {
        pct = 0; used_gb = 0; total_gb = 0;
    }
}
