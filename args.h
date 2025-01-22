#pragma once

#include <string>
#include <print>
#include <ranges>

#include <cxxopts.hpp>

#define default_confidence 90
#define default_img "/home/atlas/PDF2Word_libs/build/Rect(71.69999694824219, 208.49996948242188, 528.9500122070312, 505.8499755859375)_py.png"
#define default_lang "chi_sim"
#define default_tessdata "/home/atlas/Downloads/tessdata"
#define default_font "/usr/share/fonts/TTF/msyh.ttc"

struct Args
{
    int confidence{};
    std::vector<std::string> images;
    std::string lang;
    std::string tessdata;
    std::string font;

    void print(FILE *stream = stdout) const
    {
        for (const auto &[i, image] : std::ranges::views::enumerate(images))
        {
            std::println(stream, "image[{}]: {}", i, image);
        }
        std::println(stream, "confidence: {}", confidence);
        std::println(stream, "lang: {}", lang);
        std::println(stream, "tessdata: {}", tessdata);
        std::println(stream, "font: {}", font);
    }

    static Args from(int argc, char **argv)
    {
        cxxopts::Options options("main", "Tesseract OCR");
        options.allow_unrecognised_options();

        auto opts_adder = options.add_options();
        opts_adder("i,images", "Image paths", cxxopts::value<std::vector<std::string>>());
        opts_adder("t,tessdata", "Tessdata path", cxxopts::value<std::string>()->default_value(default_tessdata));
        opts_adder("l,lang", "Language", cxxopts::value<std::string>()->default_value(default_lang));
        opts_adder("f,font", "Font path", cxxopts::value<std::string>()->default_value(default_font));
        opts_adder("c,confidence", "Confidence", cxxopts::value<int>()->default_value(std::to_string(default_confidence)));

        const auto result = options.parse(argc, argv);

        return {
            result["confidence"].as<int>(),
            result["images"].as<std::vector<std::string>>(),
            result["lang"].as<std::string>(),
            result["tessdata"].as<std::string>(),
            result["font"].as<std::string>(),
        };
    }
};
