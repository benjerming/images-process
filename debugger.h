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

#define CV_COLOR_RED cv::Scalar(0, 0, 255)
#define CV_COLOR_GREEN cv::Scalar(0, 255, 0)
#define CV_COLOR_BLUE cv::Scalar(255, 0, 0)
#define CV_COLOR_YELLOW cv::Scalar(0, 255, 255)
#define CV_COLOR_PURPLE cv::Scalar(255, 0, 255)
#define CV_COLOR_CYAN cv::Scalar(255, 255, 0)
#define CV_COLOR_WHITE cv::Scalar(255, 255, 255)
#define CV_COLOR_BLACK cv::Scalar(0, 0, 0)

class Debugger
{
public:
    Debugger(const Args &args) : m_args(args)
    {
        // 初始化 FreeType
        FT_Init_FreeType(&m_ft_library);
        // 加载字体
        FT_New_Face(m_ft_library, args.font.c_str(), 0, &m_face);

        auto image = std::shared_ptr<Pix>(pixRead(args.img.c_str()), [](Pix *p)
                                          { pixDestroy(&p); });

        // 获取图像的宽度和高度
        l_int32 width, height, depth;
        if (pixGetDimensions(image.get(), &width, &height, &depth))
        {
            std::println(stderr, "Could not get image dimensions.");
            return;
        }
        std::println("width: {}, height: {}, depth: {}", width, height, depth);

        m_bitmap = cv::Mat(height, width, CV_8UC3, cv::Scalar(255, 255, 255));
    }

    ~Debugger()
    {
        flush();
        FT_Done_Face(m_face);
        FT_Done_FreeType(m_ft_library);
    }

    void on_line(const Rect &line_bbox)
    {
        m_line_bboxes.insert(line_bbox);
    }

    void on_word(const Rect &word_bbox)
    {
        m_word_bboxes.insert(word_bbox);
    }

    void on_char(const Rect &char_bbox, const std::string &text)
    {
        m_char_bboxes.insert(char_bbox);
    }

    void putChineseText(const std::string &text, Rect rect, cv::Scalar color)
    {
        // 设置字体大小
        FT_Set_Pixel_Sizes(m_face, rect.width(), rect.height());

        unsigned int baseline = 0;
        for (int i = 0; i < text.size(); ++i)
        {
            // 加载字符
            if (FT_Load_Char(m_face, text[i], FT_LOAD_RENDER))
            {
                continue;
            }
            FT_GlyphSlot g = m_face->glyph;

            // 计算位置
            cv::Mat bitmap(g->bitmap.rows, g->bitmap.width, CV_8UC1, g->bitmap.buffer);
            cv::Mat roi = m_bitmap(cv::Rect(rect.left + g->bitmap_left, rect.top - g->bitmap_top, g->bitmap.width, g->bitmap.rows));
            double opacity = 1.0;
            cv::addWeighted(roi, 1.0 - opacity, bitmap, opacity, 0.0, roi);

            // 更新下一个字的位置
            rect.left += g->advance.x >> 6;
            baseline = std::max(baseline, g->bitmap.rows - g->bitmap_top);
        }
    }

    void flush()
    {
        for (const auto &line_bbox : m_line_bboxes)
        {
            println("line bbox: {}", line_bbox.to_string());
            cv::rectangle(m_bitmap, cv::Point(line_bbox.left, line_bbox.top), cv::Point(line_bbox.right, line_bbox.bottom), CV_COLOR_YELLOW, 1, cv::LINE_AA, 0);
        }
        for (const auto &word_bbox : m_word_bboxes)
        {
            println("word bbox: {}", word_bbox.to_string());
            cv::rectangle(m_bitmap, cv::Point(word_bbox.left, word_bbox.top), cv::Point(word_bbox.right, word_bbox.bottom), CV_COLOR_GREEN, 1, cv::LINE_AA, 0);
        }
        for (const auto &char_bbox : m_char_bboxes)
        {
            println("char bbox: {}", char_bbox.to_string());
            cv::rectangle(m_bitmap, cv::Point(char_bbox.left, char_bbox.top), cv::Point(char_bbox.right, char_bbox.bottom), CV_COLOR_RED, 1, cv::LINE_AA, 0);
        }
        m_line_bboxes.clear();
        m_word_bboxes.clear();
        m_char_bboxes.clear();
        cv::imwrite(std::format("{}.dbg.png", std::filesystem::path(m_args.img).filename().string()).c_str(), m_bitmap);
    }

private:
    Args m_args;
    FT_Library m_ft_library;
    FT_Face m_face;
    cv::Mat m_bitmap;
    std::unordered_set<Rect> m_line_bboxes;
    std::unordered_set<Rect> m_word_bboxes;
    std::unordered_set<Rect> m_char_bboxes;
};
