#pragma once

#include <string>
#include <print>
#include <cxxopts.hpp>

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

    static Args from_args(int argc, char **argv)
    {
        cxxopts::Options options("main", "Tesseract OCR");
        options.allow_unrecognised_options();
        auto opts_adder = options.add_options();
        Args args;
        opts_adder("c,confidence", "Confidence", cxxopts::value<int>()->default_value(std::to_string(default_confidence)));
        opts_adder("i,image", "Image path", cxxopts::value<std::string>()->default_value(default_img));
        opts_adder("l,lang", "Language", cxxopts::value<std::string>()->default_value(default_lang));
        opts_adder("t,tessdata", "Tessdata path", cxxopts::value<std::string>()->default_value(default_tessdata));

        const auto result = options.parse(argc, argv);

        args.tessdata = result["tessdata"].as<std::string>();
        args.lang = result["lang"].as<std::string>();
        args.img = result["image"].as<std::string>();
        args.confidence = result["confidence"].as<int>();

        return args;
    }
};
