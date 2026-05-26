#pragma once
#include <string>


int get_cpu_usage();
void get_ram_usage(int& pct, double& used_gb, double& total_gb);
std::string get_uptime();
void get_disk_usage(int& pct, double& used_gb, double& total_gb);
