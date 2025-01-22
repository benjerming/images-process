#pragma once

#include <string>
#include <print>
#include <ranges>

#include <cxxopts.hpp>

#define default_confidence 90
#define default_lang "chi_sim+eng"
#define default_tessdata "/usr/share/tessdata"
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
        cxxopts::Options options(argv[0], "Tesseract OCR");
        options.allow_unrecognised_options();

        auto opts_adder = options.add_options();
        opts_adder("c,confidence", "Confidence", cxxopts::value<int>()->default_value(std::to_string(default_confidence)));
        opts_adder("i,images", "Image paths, can be multiple");
        opts_adder("l,lang", "Language", cxxopts::value<std::string>()->default_value(default_lang));
        opts_adder("t,tessdata", "Tessdata path", cxxopts::value<std::string>()->default_value(default_tessdata));
        opts_adder("f,font", "Font path", cxxopts::value<std::string>()->default_value(default_font));
        opts_adder("h,help", "Show help");

        const auto result = options.parse(argc, argv);

        if (result["help"].as<bool>() || result["h"].as<bool>())
        {
            puts(options.help().c_str());
            exit(0);
        }

        return {
            result["confidence"].as<int>(),
            result["images"].as<std::vector<std::string>>(),
            result["lang"].as<std::string>(),
            result["tessdata"].as<std::string>(),
            result["font"].as<std::string>(),
        };
    }
};
