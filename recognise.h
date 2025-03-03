#pragma once

#include <print>
#include <algorithm>
#include <tuple>
#include <vector>
#include <unordered_set>

#include "algo.h"
#include "args.h"
#include "debugger.h"
#include "fixed_debugger.h"
#include "fixed2_debugger.h"

#include <tesseract/baseapi.h>

class Recognise
{
public:
    static void ocr_recognise(const Args &args)
    {
        for (const auto &image_path : args.images)
        {
            std::println("{} ----------------------------------------------------------------------------", image_path);
            Debugger debugger(args);
            fixed_debugger::Debugger fixed_debugger(args);
            fixed2_debugger::Debugger fixed2_debugger(args);
            debugger.set_image(image_path);
            fixed_debugger.set_image(image_path);
            fixed2_debugger.set_image(image_path);
            auto api = std::make_shared<tesseract::TessBaseAPI>();
            if (api->Init(args.tessdata.c_str(), args.lang.c_str()))
            {
                std::println(stderr, "Could not initialize tesseract.");
                return;
            }
            auto image = std::shared_ptr<Pix>(pixRead(image_path.c_str()), [](Pix *p)
                                              { pixDestroy(&p); });

            // 获取图像的宽度和高度
            l_int32 width, height, depth;
            if (pixGetDimensions(image.get(), &width, &height, &depth))
            {
                std::println(stderr, "Could not get image dimensions.");
                return;
            }
            std::println("width: {}, height: {}, depth: {}", width, height, depth);

            // 获取图像分辨率
            l_int32 xres, yres;
            if (pixGetResolution(image.get(), &xres, &yres))
            {
                std::println(stderr, "Could not get image resolution.");
                return;
            }
            std::println("xres: {}, yres: {}", xres, yres);

            api->SetImage(image.get());

            // 设置图像分辨率
            l_int32 xres_new = 300, yres_new = 300;
            if (pixSetResolution(image.get(), xres_new, yres_new))
            {
                std::println(stderr, "Could not set image resolution.");
                return;
            }
            std::println("xres: {} -> {}, yres: {} -> {}", xres, xres_new, yres, yres_new);

            if (pixGetDimensions(image.get(), &width, &height, &depth))
            {
                std::println(stderr, "Could not get image dimensions.");
                return;
            }
            std::println("width: {}, height: {}, depth: {}", width, height, depth);

            if (api->Recognize(nullptr))
            {
                std::println(stderr, "Recognize failed");
                return;
            }

            auto res_it = std::shared_ptr<tesseract::ResultIterator>(api->GetIterator());

            while (!res_it->Empty(tesseract::RIL_TEXTLINE))
            {
                if (res_it->Empty(tesseract::RIL_WORD))
                {
                    res_it->Next(tesseract::RIL_WORD);
                    continue;
                }

                Rect line_bbox, word_bbox;
                int line_conf, word_conf;
                res_it->BoundingBox(tesseract::RIL_TEXTLINE, &line_bbox.x0, &line_bbox.y0, &line_bbox.x1, &line_bbox.y1);
                res_it->BoundingBox(tesseract::RIL_WORD, &word_bbox.x0, &word_bbox.y0, &word_bbox.x1, &word_bbox.y1);
                line_conf = res_it->Confidence(tesseract::RIL_TEXTLINE);
                word_conf = res_it->Confidence(tesseract::RIL_WORD);

                debugger.on_line(line_bbox);
                debugger.on_word(word_bbox);
                // std::println("line {} conf: {}", line_bbox.to_string(), line_conf);
                // std::println("word {} conf: {}", word_bbox.to_string(), word_conf);

                do
                {
                    Rect char_bbox;
                    res_it->BoundingBox(tesseract::RIL_SYMBOL, &char_bbox.x0, &char_bbox.y0, &char_bbox.x1, &char_bbox.y1);
                    auto conf = res_it->Confidence(tesseract::RIL_SYMBOL);
                    auto text = std::shared_ptr<char>(res_it->GetUTF8Text(tesseract::RIL_SYMBOL));
                    bool bold, italic, underline, monospace, serif, smallcaps;
                    int size, font_id;

                    auto font_name = res_it->WordFontAttributes(&bold, &italic, &underline, &monospace, &serif, &smallcaps, &size, &font_id);

                    // char_bbox.right = std::min(char_bbox.right, char_bbox.left + (int)(size * 1.5));
                    if (conf > args.confidence)
                    {
                        // std::println("{} char {} conf: {} size: {}", text.get(), char_bbox.to_string(), conf, size);

                        debugger.on_char(char_bbox, std::string(text.get()), size);
                        fixed_debugger.on_char(line_bbox, word_bbox, char_bbox, std::string(text.get()), size);
                        fixed2_debugger.on_char(line_bbox, word_bbox, char_bbox, std::string(text.get()), size);
                    }

                    res_it->Next(tesseract::RIL_SYMBOL);
                } while (!res_it->Empty(tesseract::RIL_BLOCK) && !res_it->IsAtBeginningOf(tesseract::RIL_WORD));
            }

            pixWrite(std::format("{}.ori.png", std::filesystem::path(image_path).filename().string()).c_str(), image.get(), IFF_PNG);
        }
    }

    [[maybe_unused]] static void findConnectedComponents(const std::vector<cv::Vec4i> &lines)
    {
        const auto n = lines.size();
        std::vector<int> visited(n, 0); // 初始化visited数组为0
        int component_id = 1;
        std::stack<int> s;
        s.push(0);

        while (!s.empty())
        {
            // // 如果栈为空，检查是否还有未访问的节点
            // if (s.empty())
            // {
            //     bool found = false;
            //     for (int i = 0; i < n; ++i)
            //     {
            //         if (visited[i] == 0)
            //         {
            //             s.push(i);
            //             found = true;
            //             break;
            //         }
            //     }
            //     if (!found)
            //         break;      // 所有节点都已访问，退出循环
            //     component_id++; // 新的连通分量id自增
            // }

            const auto v = s.top();
            s.pop();

            // 如果该顶点已经访问过，跳过
            if (visited[v] != 0)
                continue;

            // 标记当前顶点为已访问
            visited[v] = component_id;

            bool isolated = true;
            for (int j = 0; j < n; ++j)
            {
                if (lines[v][j] != 0)
                {
                    // 如果找到非0元素，则该顶点不是孤立的
                    isolated = false;
                    if (visited[j] == 0)
                    { // 如果邻接顶点未被访问
                        s.push(j);
                    }
                }
            }

            // 如果是孤立的顶点，则component_id需要自增以准备下一个连通分量
            if (isolated)
                component_id++;
        }

        // 输出每个节点所属的连通分量
        std::println("Node : Component ID");
        for (int i = 0; i < n; ++i)
        {
            std::println("{} : {}", i, visited[i]);
        }
    }

    static void tables_recognise(const Args &args)
    {
        for (const auto &image_path : args.images)
        {
            auto mat = cv::imread(image_path, cv::IMREAD_GRAYSCALE);
            auto blurred = cv::Mat(mat.size(), CV_8UC1);
            blurred = cv::Scalar(255);
            auto edges = cv::Mat(mat.size(), CV_8UC1);
            edges = cv::Scalar(255);
            auto lines = cv::Mat(mat.size(), CV_8UC1);
            lines = cv::Scalar(255);
            std::vector<cv::Vec4i> lines_vector;

            cv::GaussianBlur(mat, blurred, cv::Size(5, 5), 0);
            cv::Canny(blurred, edges, 150, 200);
            cv::HoughLinesP(edges, lines_vector, 1, CV_PI / 180, 100, 10, 2);
            lines = cv::Scalar(255, 255, 255);

            for (int i = 0; i < lines_vector.size(); i++)
            {
                cv::Vec4i l = lines_vector[i];
                const auto x0 = l[0], y0 = l[1], x1 = l[2], y1 = l[3];
                if (x0 != x1 && y0 != y1)
                {
                    continue;
                }
                cv::line(lines, cv::Point(x0, y0), cv::Point(x1, y1), cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
            }

            cv::imwrite(std::format("{}.blur.png", std::filesystem::path(image_path).stem().string()), blurred);
            cv::imwrite(std::format("{}.edges.png", std::filesystem::path(image_path).stem().string()), edges);
            cv::imwrite(std::format("{}.lines.png", std::filesystem::path(image_path).stem().string()), lines);
        }
    }

    static fixed2_debugger::Page texts_recognise(const std::string &image_path, const Args &args)
    {
        auto api = std::make_shared<tesseract::TessBaseAPI>();
        if (api->Init(args.tessdata.c_str(), args.lang.c_str()))
        {
            std::println(stderr, "Could not initialize tesseract.");
            return {};
        }
        auto image = std::shared_ptr<Pix>(pixRead(image_path.c_str()), [](Pix *p)
                                          { pixDestroy(&p); });

        // 获取图像的宽度和高度
        l_int32 width, height, depth;
        if (pixGetDimensions(image.get(), &width, &height, &depth))
        {
            std::println(stderr, "Could not get image dimensions.");
            return {};
        }
        std::println("width: {}, height: {}, depth: {}", width, height, depth);

        // 获取图像分辨率
        l_int32 xres, yres;
        if (pixGetResolution(image.get(), &xres, &yres))
        {
            std::println(stderr, "Could not get image resolution.");
            return {};
        }
        std::println("xres: {}, yres: {}", xres, yres);

        api->SetImage(image.get());

        // 设置图像分辨率
        l_int32 xres_new = 300, yres_new = 300;
        if (pixSetResolution(image.get(), xres_new, yres_new))
        {
            std::println(stderr, "Could not set image resolution.");
            return {};
        }
        std::println("xres: {} -> {}, yres: {} -> {}", xres, xres_new, yres, yres_new);

        if (pixGetDimensions(image.get(), &width, &height, &depth))
        {
            std::println(stderr, "Could not get image dimensions.");
            return {};
        }
        std::println("width: {}, height: {}, depth: {}", width, height, depth);

        if (api->Recognize(nullptr))
        {
            std::println(stderr, "Recognize failed");
            return {};
        }

        auto res_it = std::shared_ptr<tesseract::ResultIterator>(api->GetIterator());

        fixed2_debugger::Page page;

        while (!res_it->Empty(tesseract::RIL_TEXTLINE))
        {
            if (res_it->Empty(tesseract::RIL_WORD))
            {
                res_it->Next(tesseract::RIL_WORD);
                continue;
            }

            Rect line_bbox, word_bbox;
            int line_conf, word_conf;
            res_it->BoundingBox(tesseract::RIL_TEXTLINE, &line_bbox.x0, &line_bbox.y0, &line_bbox.x1, &line_bbox.y1);
            res_it->BoundingBox(tesseract::RIL_WORD, &word_bbox.x0, &word_bbox.y0, &word_bbox.x1, &word_bbox.y1);
            line_conf = res_it->Confidence(tesseract::RIL_TEXTLINE);
            word_conf = res_it->Confidence(tesseract::RIL_WORD);

            do
            {
                Rect char_bbox;
                res_it->BoundingBox(tesseract::RIL_SYMBOL, &char_bbox.x0, &char_bbox.y0, &char_bbox.x1, &char_bbox.y1);
                auto conf = res_it->Confidence(tesseract::RIL_SYMBOL);
                auto text = std::shared_ptr<char>(res_it->GetUTF8Text(tesseract::RIL_SYMBOL));
                bool bold, italic, underline, monospace, serif, smallcaps;
                int size, font_id;

                auto font_name = res_it->WordFontAttributes(&bold, &italic, &underline, &monospace, &serif, &smallcaps, &size, &font_id);

                page.append_char(line_bbox, word_bbox, char_bbox, std::string(text.get()), size);

                res_it->Next(tesseract::RIL_SYMBOL);
            } while (!res_it->Empty(tesseract::RIL_BLOCK) && !res_it->IsAtBeginningOf(tesseract::RIL_WORD));
        }

        return page;
    }

    static Rects segments_recognise(const std::string &image_path, const Args &args)
    {
        auto mat = cv::imread(image_path, cv::IMREAD_GRAYSCALE);
        auto blurred = cv::Mat(mat.size(), CV_8UC1);
        blurred = cv::Scalar(255);
        auto edges = cv::Mat(mat.size(), CV_8UC1);
        edges = cv::Scalar(255);
        auto lines = cv::Mat(mat.size(), CV_8UC1);
        lines = cv::Scalar(255);
        std::vector<cv::Vec4i> lines_vector;

        cv::GaussianBlur(mat, blurred, cv::Size(5, 5), 0);
        cv::Canny(blurred, edges, 150, 200);
        cv::HoughLinesP(edges, lines_vector, 1, CV_PI / 180, 100, 10, 2);
        lines = cv::Scalar(255, 255, 255);

        Rects segments;
        segments.reserve(lines_vector.size());
        for (int i = 0; i < lines_vector.size(); i++)
        {
            cv::Vec4i l = lines_vector[i];
            const auto x0 = l[0], y0 = l[1], x1 = l[2], y1 = l[3];
            if (x0 != x1 && y0 != y1)
            {
                continue;
            }
            const auto minx = std::min(x0, x1);
            const auto maxx = std::max(x0, x1);
            const auto miny = std::min(y0, y1);
            const auto maxy = std::max(y0, y1);
            cv::line(lines, cv::Point(minx, miny), cv::Point(maxx, maxy), cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
            segments.push_back({minx, miny, maxx, maxy});
        }

        cv::imwrite(std::format("{}.blur.png", std::filesystem::path(image_path).stem().string()), blurred);
        cv::imwrite(std::format("{}.edges.png", std::filesystem::path(image_path).stem().string()), edges);
        cv::imwrite(std::format("{}.lines.png", std::filesystem::path(image_path).stem().string()), lines);

        return segments;
    }

    template <typename T>
    static _Rect<T> BoundingBox(const _Rects<T> &segs)
    {
        auto bbox = segs.front();
        for (const auto &seg : segs)
        {
            // do not use |, because seg is empty
            bbox.x0 = std::min(bbox.x0, seg.x0);
            bbox.y0 = std::min(bbox.y0, seg.y0);
            bbox.x1 = std::max(bbox.x1, seg.x1);
            bbox.y1 = std::max(bbox.y1, seg.y1);
        }
        return bbox;
    }

    // static bool is_noisy(const Rectsf32 &segs, const fixed2_debugger::Page &page)
    // {
    //     if (!std::ranges::any_of(segs, [](const Rectsf32 &seg)
    //                              { return seg.is_horizontal(); }) ||
    //         !std::ranges::any_of(segs, [](const Rectsf32 &seg)
    //                              { return seg.is_vertical(); }))
    //     {
    //         return true;
    //     }

    //     Rectsf32 bbox = BoundingBox(segs);

    //     for (const auto &line : page.m_lines)
    //     {
    //         if (bbox.contains(line.bbox))
    //         {
    //             return false;
    //         }
    //         for (const auto &word : line.words)
    //         {
    //             if (bbox.contains(word.bbox))
    //             {
    //                 return false;
    //             }
    //             // all segments in one word, this noise
    //             if (word.bbox.contains(bbox))
    //             {
    //                 return true;
    //             }
    //             for (const auto &c : word.chars)
    //             {
    //                 // any segment in one char, this noise
    //                 for (const auto &seg : segs)
    //                 {
    //                     if (c.bbox.contains(seg))
    //                     {
    //                         return true;
    //                     }
    //                 }
    //             }
    //         }
    //     }
    //     return false;
    // }

    static Rects filter_segments(const Rects &segments, const fixed2_debugger::Page &page, const std::string &image_path = "")
    {
        cv::Mat mat;
        if (!image_path.empty())
        {
            mat = cv::imread(image_path, cv::IMREAD_COLOR);
        }

        auto segs = std::ranges::views::transform(segments, [](const Rect &seg)
                                                  { return Rectf32::from(seg); }) |
                    std::ranges::to<Rectsf32>();

        auto groups = algo::Algo::group_by_connectivity(segs);

        for (const auto &group : groups)
        {
            if (group.size() == 1)
            {
                continue;
            }

            const auto bbox = Rectf32::from(BoundingBox(group)).expand(0.5f).to_cv_rect();
            cv::rectangle(mat, bbox, cv::Scalar(0, 0, 0xff), 3);

            if (!image_path.empty())
            {
                cv::imwrite(std::format("{}.grouped_segments.png", std::filesystem::path(image_path).stem().string()), mat);
            }

            continue;
        }

        if (!image_path.empty())
        {
            cv::imwrite(std::format("{}.grouped_segments.png", std::filesystem::path(image_path).stem().string()), mat);
        }

        return segments;
    }
};
