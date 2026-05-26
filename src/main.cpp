#include "globals.hpp"
#include "widgets/notepad_widget.hpp"
#include "widgets/launcher_widget.hpp"
#include "splashscreen.hpp"
#include "dashboard.hpp"
#include <iostream>
#include <clocale>

int main() {
    
    std::setlocale(LC_ALL, "");
    
    
    initscr();

    start_color();
    use_default_colors();
    noecho();
    cbreak();
    
    
    init_theme_colors();
    
    
    load_notes();
    
    
    load_desktop_apps();

    
    
    run_splashscreen();
    
    
    run_dashboard();
    
    
    clear();
    refresh();
    endwin();
    
    std::cout << "\n\033[1;32m✔\033[0m Thanks for using Tiwut CLI. Keep coding!\n\n";
    return 0;
}
