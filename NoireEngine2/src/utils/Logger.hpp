#pragma once

#include <string>
#include <iostream>
#include <format>

class Logger
{
public:
    enum c_color { BLACK = 30, RED = 31, GREEN = 32, YELLOW = 33, BLUE = 34, MAGENTA = 35, CYAN = 36, WHITE = 37 };
    enum c_decoration { NORMAL = 0, BOLD = 1, FAINT = 2, ITALIC = 3, UNDERLINE = 4, RIVERCED = 26, FRAMED = 51 };

    template <class... _Types>
    static void INFO(const std::format_string<_Types...> _Fmt, _Types&&... _Args) {
        INFO(std::format(_Fmt, std::move(_Args)...));
    }

    template <class... _Types>
    static void INFO(const std::format_string<_Types...> _Fmt, _Types&... _Args) {
        INFO(std::format(_Fmt, std::move(_Args)...));
    }

    static void INFO(const std::string str)
    {
        std::cout << "\033[" << NORMAL << ";" << GREEN << "m" << "[Info] " << str << "\033[0m" << std::endl;
    }

    template <class... _Types>
    static void WARN(const std::format_string<_Types...> _Fmt, _Types&... _Args) {
        WARN(std::format(_Fmt, std::move(_Args)...));
    }

    template <class... _Types>
    static void WARN(const std::format_string<_Types...> _Fmt, _Types&&... _Args) {
        WARN(std::format(_Fmt, std::move(_Args)...));
    }

    static void WARN(const std::string str)
    {
        std::cout << "\033[" << ITALIC << ";" << YELLOW << "m" << "[Warning] " << str << "\033[0m" << std::endl;
    }

    template <class... _Types>
    static void ERROR(const std::format_string<_Types...> _Fmt, _Types&... _Args) {
        ERROR(std::format(_Fmt, std::move(_Args)...));
    }

    template <class... _Types>
    static void ERROR(const std::format_string<_Types...> _Fmt, _Types&&... _Args) {
        ERROR(std::format(_Fmt, std::move(_Args)...));
    }

    [[noreturn]] static void ERROR(const std::string str)
    {
        std::cout << "\033[" << BOLD << ";" << RED << "m" << "[Error] " << str << "\033[0m" << std::endl;
        abort();
    }

    static void DEBUG(const std::string str, c_color color, c_decoration decoration = c_decoration::NORMAL) 
    {
        std::cout << "\033[" << decoration << ";" << color << "m" << "[Debug] " << str << "\033[0m" << std::endl;
    }

    template <class... _Types>
    static void NULLIFY(_Types...Args) { }
};

//#define NE_DEBUG(...)   Logger::NULLIFY(__VA_ARGS__)
//#define NE_INFO(...)    Logger::NULLIFY(__VA_ARGS__)
//#define NE_WARN(...)    Logger::NULLIFY(__VA_ARGS__)

#define NE_DEBUG(...)   Logger::DEBUG(__VA_ARGS__)
#define NE_INFO(...)    Logger::INFO(__VA_ARGS__)
#define NE_WARN(...)    Logger::WARN(__VA_ARGS__)
#define NE_ERROR(...)   Logger::ERROR(__VA_ARGS__)
