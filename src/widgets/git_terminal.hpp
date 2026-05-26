#pragma once
#include <curses.h>
#include <string>
#include <vector>


struct GithubRepo {
    std::string name;
    std::string owner;
    std::string html_url;
    std::string description;
    int stars = 0;
};


void run_integrated_terminal(WINDOW* parent);


void run_github_repos_menu(WINDOW* parent);


void run_remote_file_browser(WINDOW* parent, const std::string& owner, const std::string& repo);
