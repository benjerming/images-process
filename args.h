#pragma once

#include <string>
#include <print>

#define default_confidence 90
#define default_img "/home/atlas/PDF2Word_libs/build/Rect(71.69999694824219, 208.49996948242188, 528.9500122070312, 505.8499755859375)_py.png"
#define default_lang "chi_sim"
#define default_tessdata "/home/atlas/Downloads/tessdata"
#define default_font "/usr/share/fonts/TTF/msyh.ttc"

struct Args
{
    int confidence{};
    std::string img;
    std::string lang;
    std::string tessdata;
    std::string font;

    void print() const
    {
        std::println("confidence: {}", confidence);
        std::println("img: {}", img);
        std::println("lang: {}", lang);
        std::println("tessdata: {}", tessdata);
        std::println("font: {}", font);
    }
};
