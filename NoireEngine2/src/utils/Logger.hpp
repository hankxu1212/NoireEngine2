#pragma once

#include <string>
#include <iostream>

namespace std {
    enum c_color { BLACK = 30, RED = 31, GREEN = 32, YELLOW = 33, BLUE = 34, MAGENTA = 35, CYAN = 36, WHITE = 37 };
    enum c_decoration { NORMAL = 0, BOLD = 1, FAINT = 2, ITALIC = 3, UNDERLINE = 4, RIVERCED = 26, FRAMED = 51 };
    void pr(const string str, c_color color, c_decoration decoration = c_decoration::NORMAL) {
        cout << "\033[" << decoration << ";" << color << "m" << str << "\033[0m";
    }

    void prl(const string str, c_color color, c_decoration decoration = c_decoration::NORMAL) {
        cout << "\033[" << decoration << ";" << color << "m" << str << "\033[0m" << endl;
    }
}