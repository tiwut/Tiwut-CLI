#include "globals.hpp"


int current_theme = 0;
int active_pane = 0;
bool maximized = false;
int active_menu = 0;
int menu_idx = 0;


const std::vector<Theme> themes = {
    {"Cyberpunk Neon", COLOR_MAGENTA, COLOR_BLUE, COLOR_CYAN, COLOR_YELLOW, COLOR_GREEN},
    {"Matrix Code", COLOR_GREEN, COLOR_BLACK, COLOR_GREEN, COLOR_WHITE, COLOR_GREEN},
    {"Midnight Ocean", COLOR_CYAN, COLOR_BLUE, COLOR_BLUE, COLOR_WHITE, COLOR_CYAN},
    {"Monochrome Sleek", COLOR_WHITE, COLOR_BLACK, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE}
};

void init_theme_colors() {
    Theme t = themes[current_theme];
    
    
    if (COLORS >= 256) {
        if (current_theme == 0) { 
            init_pair(1, 201, -1); 
            init_pair(2, 240, -1); 
            init_pair(3, 51, -1);  
            init_pair(4, 196, -1); 
            init_pair(5, 220, -1); 
        } else if (current_theme == 1) { 
            init_pair(1, 46, -1);  
            init_pair(2, 235, -1); 
            init_pair(3, 82, -1);  
            init_pair(4, 196, -1);
            init_pair(5, 46, -1);
        } else if (current_theme == 2) { 
            init_pair(1, 81, -1);  
            init_pair(2, 239, -1); 
            init_pair(3, 39, -1);  
            init_pair(4, 196, -1);
            init_pair(5, 81, -1);
        } else { 
            init_pair(1, 255, -1); 
            init_pair(2, 238, -1); 
            init_pair(3, 250, -1); 
            init_pair(4, 255, -1);
            init_pair(5, 255, -1);
        }
    } else {
        
        init_pair(1, t.border_active, -1);
        init_pair(2, t.border_inactive, -1);
        init_pair(3, t.header_fg, -1);
        init_pair(4, COLOR_RED, -1);
        init_pair(5, t.success, -1);
    }
}
