#pragma once

#include <memory>
#include <string>
#include <print>
#include <unordered_set>
#include <ranges>
#include <filesystem>

#include "args.h"
#include "common.h"

#include <ft2build.h>
#include FT_FREETYPE_H

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

struct CharInfo
{
    std::string text;
    Rect bbox;
    int pointsize;

    bool operator==(const CharInfo &other) const
    {
        return text == other.text && bbox == other.bbox && pointsize == other.pointsize;
    }
};

namespace std
{
    template <>
    struct hash<CharInfo>
    {
        size_t operator()(const CharInfo &info) const
        {
            return std::hash<std::string>()(info.text);
        }
    };
}

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
        m_bitmap = cv::Scalar(255, 255, 255);
    }

    ~Debugger()
    {
        flush();
    }

    void on_line(const Rect &line_bbox)
    {
        m_line_bboxes.insert(line_bbox);
    }

    void on_word(const Rect &word_bbox)
    {
        m_word_bboxes.insert(word_bbox);
    }

    void on_char(const Rect &char_bbox, const std::string &text, int pointsize)
    {
        m_chars.insert(CharInfo{text, char_bbox, pointsize});
    }

    void putChineseText(const std::string &text, Rect rect, cv::Scalar color)
    {
        int baseline = 0;
        m_ft2->getTextSize(text, rect.height(), -1, &baseline);
        m_ft2->putText(m_bitmap, text, cv::Point(rect.x0, rect.y0 + baseline), rect.height(), color, -1, cv::LINE_AA, false);
    }

    void flush()
    {
        for (const auto &line_bbox : m_line_bboxes)
        {
            // println("line bbox: {}", line_bbox.to_string());
            cv::rectangle(m_bitmap, cv::Point(line_bbox.x0, line_bbox.y0), cv::Point(line_bbox.x1, line_bbox.y1), CV_COLOR_YELLOW, 2, cv::LINE_AA, 0);
        }
        for (const auto &word_bbox : m_word_bboxes)
        {
            // println("word bbox: {}", word_bbox.to_string());
            cv::rectangle(m_bitmap, cv::Point(word_bbox.x0, word_bbox.y0), cv::Point(word_bbox.x1, word_bbox.y1), CV_COLOR_GREEN, 2, cv::LINE_8, 0);
        }
        for (const auto &char_info : m_chars)
        {
            // println("char bbox: {}", char_info.bbox.to_string());
            putChineseText(char_info.text, char_info.bbox, CV_COLOR_BLACK);
            cv::rectangle(m_bitmap, cv::Point(char_info.bbox.x0, char_info.bbox.y0), cv::Point(char_info.bbox.x1, char_info.bbox.y1), CV_COLOR_RED, 1, cv::LINE_4, 0);
        }
        m_line_bboxes.clear();
        m_word_bboxes.clear();
        m_chars.clear();
        cv::imwrite(std::format("{}.dbg.png", std::filesystem::path(m_image_path).filename().string()).c_str(), m_bitmap);
    }

private:
    Args m_args;
    std::string m_image_path;
    cv::Ptr<cv::freetype::FreeType2> m_ft2;
    cv::Mat m_bitmap;
    std::unordered_set<Rect> m_line_bboxes;
    std::unordered_set<Rect> m_word_bboxes;
    std::unordered_set<CharInfo> m_chars;
};
