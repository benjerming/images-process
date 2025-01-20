#pragma once

#include <memory>
#include <string>
#include <print>
#include <unordered_map>
#include <filesystem>

#include "args.h"
#include "common.h"

#include <leptonica/allheaders.h>

#include <opencv4/opencv2/opencv.hpp>
#include <opencv4/opencv2/imgproc.hpp>
#include <opencv4/opencv2/imgcodecs.hpp>
#include <opencv4/opencv2/freetype.hpp>

#define CV_COLOR_RED cv::Scalar(0, 0, 255)
#define CV_COLOR_GREEN cv::Scalar(0, 255, 0)
#define CV_COLOR_BLUE cv::Scalar(255, 0, 0)
#define CV_COLOR_YELLOW cv::Scalar(0, 255, 255)
#define CV_COLOR_PURPLE cv::Scalar(255, 0, 255)
#define CV_COLOR_CYAN cv::Scalar(255, 255, 0)
#define CV_COLOR_WHITE cv::Scalar(255, 255, 255)
#define CV_COLOR_BLACK cv::Scalar(0, 0, 0)

namespace fixed_debugger
{
    struct Char
    {
        Rect bbox;
        std::string text;
        int pointsize;

        bool operator==(const Char &other) const
        {
            return text == other.text && bbox == other.bbox && pointsize == other.pointsize;
        }
    };

    struct Word
    {
        Rect bbox;
        std::unordered_map<Rect, Char> chars;
    };

    struct Line
    {
        Rect bbox;
        std::unordered_map<Rect, Word> words;
    };

}

namespace std
{
    template <>
    struct hash<fixed_debugger::Char>
    {
        size_t operator()(const fixed_debugger::Char &info) const
        {
            return std::hash<std::string>()(info.text);
        }
    };

}

namespace fixed_debugger
{
    class Debugger
    {

    public:
        Debugger(const Args &args) : m_args(args)
        {
            m_ft2 = cv::freetype::createFreeType2();
            m_ft2->loadFontData(m_args.font, 0);
        }

        void set_image(const std::string &image_path)
        {
            m_image_path = image_path;
            m_bitmap = cv::imread(image_path, cv::IMREAD_COLOR);
            m_bitmap = CV_COLOR_WHITE;
        }

        ~Debugger()
        {
            flush();
        }

        Rect fit_line_bbox(const Rect &line_bbox, const Rect &bbox)
        {
            Rect fit_bbox = bbox;
            fit_bbox.top = line_bbox.top;
            fit_bbox.bottom = line_bbox.bottom;
            return fit_bbox;
        }

        void on_char(const Rect &line_bbox, const Rect &word_bbox, const Rect &char_bbox, const std::string &text, int pointsize)
        {
            auto fit_word_bbox = fit_line_bbox(line_bbox, word_bbox);
            auto fit_char_bbox = fit_line_bbox(line_bbox, char_bbox);

            auto &line = m_lines[line_bbox];
            line.bbox = line_bbox;

            auto &word = line.words[fit_word_bbox];
            word.bbox = fit_word_bbox;

            auto &ch = word.chars[fit_char_bbox];
            ch = Char{fit_char_bbox, text, pointsize};
        }

        void putChineseText(const Char &ch, cv::Scalar color)
        {
            int baseline = 0;
            m_ft2->getTextSize(ch.text, ch.bbox.height(), -1, &baseline);
            m_ft2->putText(m_bitmap, ch.text, cv::Point(ch.bbox.left, ch.bbox.top + baseline), ch.bbox.height(), color, -1, cv::LINE_AA, false);
        }

        void flush_char(const Char &ch)
        {
            println("{} bbox: {}", __func__, ch.bbox.to_string());
            putChineseText(ch, CV_COLOR_BLACK);
            cv::rectangle(m_bitmap, cv::Point(ch.bbox.left, ch.bbox.top), cv::Point(ch.bbox.right, ch.bbox.bottom), CV_COLOR_RED, 1, cv::LINE_AA, 0);
        }

        void flush_word(const Word &word)
        {
            println("{} bbox: {}", __func__, word.bbox.to_string());
            cv::rectangle(m_bitmap, cv::Point(word.bbox.left, word.bbox.top), cv::Point(word.bbox.right, word.bbox.bottom), CV_COLOR_GREEN, 1, cv::LINE_AA, 0);
            for (const auto &[char_bbox, ch] : word.chars)
            {
                flush_char(ch);
            }
        }

        void flush_line(const Line &line)
        {
            println("{} bbox: {}", __func__, line.bbox.to_string());
            cv::rectangle(m_bitmap, cv::Point(line.bbox.left, line.bbox.top), cv::Point(line.bbox.right, line.bbox.bottom), CV_COLOR_YELLOW, 1, cv::LINE_AA, 0);
            for (const auto &[word_bbox, word] : line.words)
            {
                flush_word(word);
            }
        }

        void flush()
        {
            for (auto &[line_bbox, line] : m_lines)
            {
                flush_line(line);
            }
            m_lines.clear();
            cv::imwrite(std::format("{}.fixed_dbg.png", std::filesystem::path(m_image_path).filename().string()).c_str(), m_bitmap);
        }

    private:
        Args m_args;
        std::string m_image_path;
        cv::Ptr<cv::freetype::FreeType2> m_ft2;
        cv::Mat m_bitmap;
        std::unordered_map<Rect, Line> m_lines;
    };
}