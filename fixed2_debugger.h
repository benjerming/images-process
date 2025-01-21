#pragma once

#include <memory>
#include <string>
#include <print>
#include <unordered_map>
#include <filesystem>
#include <numeric>
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

namespace fixed2_debugger
{
    struct Char
    {
        Rect bbox;
        std::string text;
        int pointsize;

        void draw(cv::Ptr<cv::freetype::FreeType2> &ft2, cv::Mat &bitmap) const
        {
            // println("{} bbox: {}", __func__, bbox.to_string());
            draw_text(ft2, bitmap);
            cv::rectangle(bitmap, cv::Point(bbox.x0, bbox.y0), cv::Point(bbox.x1, bbox.y1), CV_COLOR_RED, 1, cv::LINE_4, 0);
        }

        void draw_text(cv::Ptr<cv::freetype::FreeType2> &ft2, cv::Mat &bitmap) const
        {
            int baseline = 0;
            try
            {
                ft2->getTextSize(text, bbox.height(), -1, &baseline);
                ft2->putText(bitmap, text, cv::Point(bbox.x0, bbox.y0 + baseline), bbox.height(), CV_COLOR_BLACK, -1, cv::LINE_AA, false);
            }
            catch (const cv::Exception &e)
            {
                std::println("{}", e.what());
            }
        }
    };

    struct Word
    {
        Rect bbox;
        std::vector<Char> chars;

        void reflow()
        {
            Word word;

            for (auto &&[lhs, rhs] : std::ranges::views::pairwise(chars))
            {
                auto ch = lhs;
                // limit the width of the character to fit the height of the character
                ch.bbox = ch.bbox.clip_left(std::min(ch.bbox.x0 + ch.bbox.height() * 6 / 5, rhs.bbox.x0 - 1));
                word.bbox |= ch.bbox;
                word.chars.emplace_back(ch);
            }

            // last character
            auto ch = chars.back();
            ch.bbox = ch.bbox.clip_left(ch.bbox.x0 + ch.bbox.height() * 6 / 5);
            word.bbox |= ch.bbox;
            word.chars.emplace_back(ch);

            word.resize();

            *this = std::move(word);
        }

        void resize()
        {
            std::unordered_map<int, int> counts;
            for (const auto &ch : chars)
            {
                counts[ch.bbox.y0]++;
            }
            auto y0 = std::max_element(counts.cbegin(), counts.cend(), [](const auto &lhs, const auto &rhs)
                                       { return lhs.second < rhs.second; })
                          ->first;
            counts.clear();

            for (const auto &ch : chars)
            {
                counts[ch.bbox.y1]++;
            }
            auto y1 = std::max_element(counts.cbegin(), counts.cend(), [](const auto &lhs, const auto &rhs)
                                       { return lhs.second < rhs.second; })
                          ->first;

            for (auto &ch : chars)
            {
                ch.pointsize = y1 - y0;
                ch.bbox = ch.bbox.clip_y(y0, y1);
            }
            bbox = bbox.clip_y(y0, y1);
        }

        void draw(cv::Ptr<cv::freetype::FreeType2> &ft2, cv::Mat &bitmap) const
        {
            // println("{} bbox: {}", __func__, bbox.to_string());
            cv::rectangle(bitmap, cv::Point(bbox.x0, bbox.y0), cv::Point(bbox.x1, bbox.y1), CV_COLOR_GREEN, 2, cv::LINE_8, 0);
            for (const auto &ch : chars)
            {
                ch.draw(ft2, bitmap);
            }
        }

        Word operator|(const Word &rhs) const
        {
            Word lhs = *this;
            lhs.bbox |= rhs.bbox;
            lhs.chars.insert(lhs.chars.end(), rhs.chars.begin(), rhs.chars.end());
            return lhs;
        }

        Word &operator|=(const Word &rhs)
        {
            bbox |= rhs.bbox;
            chars.insert(chars.end(), rhs.chars.begin(), rhs.chars.end());
            return *this;
        }
    };

    struct Line
    {
        Rect bbox;
        std::vector<Word> words;

        void draw(cv::Ptr<cv::freetype::FreeType2> &ft2, cv::Mat &bitmap) const
        {
            // println("{} bbox: {}", __func__, bbox.to_string());
            cv::rectangle(bitmap, cv::Point(bbox.x0, bbox.y0), cv::Point(bbox.x1, bbox.y1), CV_COLOR_YELLOW, 2, cv::LINE_AA, 0);
            for (const auto &word : words)
            {
                word.draw(ft2, bitmap);
            }
        }

        void reflow(bool recursive = false)
        {
            std::vector<Word> new_words;
            new_words.reserve(words.size());

            if (recursive)
            {
                words.front().reflow();
            }
            new_words.emplace_back(words.front());

            for (auto &&rhs : std::ranges::views::drop(words, 1))
            {
                if (recursive)
                {
                    rhs.reflow();
                }
                auto &lhs = new_words.back();
                const auto &lhs_char = lhs.chars.back();
                const auto &rhs_char = rhs.chars.front();
                if (lhs_char.bbox.x1 + lhs_char.bbox.height() >= rhs_char.bbox.x0)
                { // nearby, merge them
                    lhs |= rhs;
                    lhs.resize();
                }
                else
                { // faraway, keep split
                    new_words.emplace_back(rhs);
                }
            }

            words = new_words;
        }
    };

    struct Page
    {
        std::vector<Line> m_lines;

        void append_char(const Rect &line_bbox, const Rect &word_bbox, const Rect &char_bbox, const std::string &text, int pointsize)
        {
            const auto limited_word_bbox = limit_to_line_height(line_bbox, word_bbox);
            const auto limited_char_bbox = limit_to_line_height(line_bbox, char_bbox);

            if (m_lines.empty() || line_bbox != m_lines.back().bbox)
            {
                m_lines.emplace_back(Line{line_bbox});
            }
            auto &line = m_lines.back();

            if (line.words.empty() || limited_word_bbox != line.words.back().bbox)
            {
                line.words.emplace_back(Word{limited_word_bbox});
            }
            auto &word = line.words.back();

            word.chars.emplace_back(Char{limited_char_bbox, text, pointsize});
        }

        void draw(cv::Ptr<cv::freetype::FreeType2> &ft2, cv::Mat &bitmap) const
        {
            for (const auto &line : m_lines)
            {
                line.draw(ft2, bitmap);
            }
        }

        void reflow()
        {
            for (auto &line : m_lines)
            {
                line.reflow(true);
            }
        }

    private:
        static Rect limit_to_line_height(const Rect &line_bbox, const Rect &bbox)
        {
            Rect fit_bbox = bbox;
            if (bbox.height() > line_bbox.height())
            {
                fit_bbox.y0 = line_bbox.y0;
                fit_bbox.y1 = line_bbox.y1;
            }
            return fit_bbox;
        }
    };
}

namespace std
{
    template <>
    struct hash<fixed2_debugger::Char>
    {
        size_t operator()(const fixed2_debugger::Char &info) const
        {
            return std::hash<std::string>()(info.text);
        }
    };

}

namespace fixed2_debugger
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
        }

        ~Debugger()
        {
            dump(std::filesystem::path(m_image_path).filename().replace_extension(".fixed2.png"));
            reflow();
        }

        void on_char(const Rect &line_bbox, const Rect &word_bbox, const Rect &char_bbox, const std::string &text, int pointsize)
        {
            m_page.append_char(line_bbox, word_bbox, char_bbox, text, pointsize);
        }

        void dump(const std::filesystem::path &filepath)
        {
            auto bitmap = cv::imread(m_image_path, cv::IMREAD_COLOR);
            bitmap = CV_COLOR_WHITE;
            m_page.draw(m_ft2, bitmap);
            cv::imwrite(filepath.generic_string(), bitmap);
        }

        void reflow()
        {
#if 1
            // reflow the words
            for (auto &line : m_page.m_lines)
            {
                for (auto &word : line.words)
                {
                    word.reflow();
                }
            }
            auto filepath1 = std::filesystem::path(m_image_path);
            filepath1.replace_extension(".fixed2_reflow_words.png");
            dump(filepath1.filename());
            // reflow the lines
            for (auto &line : m_page.m_lines)
            {
                line.reflow(false);
            }
            auto filepath2 = std::filesystem::path(m_image_path);
            filepath2.replace_extension(".fixed2_reflow_lines.png");
            dump(filepath2.filename());
#else
            m_page.reflow();
            filepath.replace_extension(".fixed2_reflow.png");
            dump(filepath.filename());
#endif
        }

    private:
        Args m_args;
        std::string m_image_path;
        cv::Ptr<cv::freetype::FreeType2> m_ft2;
        Page m_page;
    };
}