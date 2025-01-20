#include "args.h"
#include "common.h"
#include "debugger.h"
#include <print>

#include <tesseract/capi.h>
#include <leptonica/allheaders.h>

using namespace tesseract;
using namespace std;
using namespace cxxopts;
using namespace std::string_literals;

static void
char_callback(const string &text,
              float confidence,
              [[maybe_unused]] const string &_font_name,
              [[maybe_unused]] Rect _line_bbox,
              [[maybe_unused]] Rect _word_bbox,
              [[maybe_unused]] Rect char_bbox,
              [[maybe_unused]] int pointsize,
              const Args &args)
{
    if (confidence < args.confidence)
    {
        println("ocr_recognise char:{}, confidence:{}, ignored\n", text, confidence);
        return;
    }

    static Rect prev_word_bbox;
    static string prev_text;

    if (char_bbox != prev_word_bbox)
    {
        int xbeg = 0, xend = 2;
        if (char_bbox.left + char_bbox.right < prev_word_bbox.left + prev_word_bbox.right)
        {
            xbeg = 2;
            xend = 0;
        }
        int curr_h_width = char_bbox.right - char_bbox.left;
        int curr_v_height = char_bbox.bottom - char_bbox.top;
        auto intersect_length = [](int a1, int a2, int b1, int b2)
        {
            return max(0, min(a2, b2) - max(a1, b1));
        };
        int h_intersect = intersect_length(prev_word_bbox.left, prev_word_bbox.right, char_bbox.left, char_bbox.right);
        int v_intersect = intersect_length(prev_word_bbox.top, prev_word_bbox.bottom, char_bbox.top, char_bbox.bottom);

        if (h_intersect > curr_h_width / 2 && v_intersect > curr_v_height / 2)
        {
            // prev horizontal is too wide
            // L2R: shrink prev char_bbox's right to current char_bbox's left
            // R2L: shrink prev char_bbox's left to current char_bbox's right
            prev_word_bbox[xend] = char_bbox[xbeg];
            println("shrinked <{}>'s prev word <{}> bbox: {}, {}", text, prev_text, prev_word_bbox[0], prev_word_bbox[2]);
        }

        prev_word_bbox = char_bbox;
    }

    prev_text = text;
}

void ocr_recognise(const Args &args,
                   std::function<void(const string &text,
                                      float confidence,
                                      const string &font_name,
                                      const Rect &line_bbox,
                                      const Rect &word_bbox,
                                      const Rect &char_bbox,
                                      int pointsize,
                                      const Args &args)>
                       callback)
{
    Debugger debugger(args);
    auto api = make_unique<TessBaseAPI>();
    if (api->Init(args.tessdata.c_str(), args.lang.c_str()))
    {
        println(stderr, "Could not initialize tesseract.");
        return;
    }
    auto image = shared_ptr<Pix>(pixRead(args.img.c_str()), [](Pix *p)
                                 { pixDestroy(&p); });

    // 获取图像的宽度和高度
    l_int32 width, height, depth;
    if (pixGetDimensions(image.get(), &width, &height, &depth))
    {
        println(stderr, "Could not get image dimensions.");
        return;
    }
    println("width: {}, height: {}, depth: {}", width, height, depth);

    // 获取图像分辨率
    l_int32 xres, yres;
    if (pixGetResolution(image.get(), &xres, &yres))
    {
        println(stderr, "Could not get image resolution.");
        return;
    }
    println("xres: {}, yres: {}", xres, yres);

    api->SetImage(image.get());

    // 设置图像分辨率
    l_int32 xres_new = 300, yres_new = 300;
    if (pixSetResolution(image.get(), xres_new, yres_new))
    {
        println(stderr, "Could not set image resolution.");
        return;
    }
    println("xres: {} -> {}, yres: {} -> {}", xres, xres_new, yres, yres_new);

    if (pixGetDimensions(image.get(), &width, &height, &depth))
    {
        println(stderr, "Could not get image dimensions.");
        return;
    }
    println("width: {}, height: {}, depth: {}", width, height, depth);

    if (api->Recognize(nullptr))
    {
        println(stderr, "Recognize failed");
        return;
    }

    auto res_it = shared_ptr<ResultIterator>(api->GetIterator());

    while (!res_it->Empty(RIL_TEXTLINE))
    {
        if (res_it->Empty(RIL_WORD))
        {
            res_it->Next(RIL_WORD);
            continue;
        }

        Rect line_bbox, word_bbox;
        int line_conf, word_conf;
        res_it->BoundingBox(RIL_TEXTLINE, &line_bbox.left, &line_bbox.top, &line_bbox.right, &line_bbox.bottom);
        res_it->BoundingBox(RIL_WORD, &word_bbox.left, &word_bbox.top, &word_bbox.right, &word_bbox.bottom);
        line_conf = res_it->Confidence(RIL_TEXTLINE);
        word_conf = res_it->Confidence(RIL_WORD);

        debugger.on_line(line_bbox);
        debugger.on_word(word_bbox);
        println("line {} conf: {}", line_bbox.to_string(), line_conf);
        println("word {} conf: {}", word_bbox.to_string(), word_conf);

        do
        {
            Rect char_bbox;
            res_it->BoundingBox(RIL_SYMBOL, &char_bbox.left, &char_bbox.top, &char_bbox.right, &char_bbox.bottom);
            auto conf = res_it->Confidence(RIL_SYMBOL);
            auto text = shared_ptr<char>(res_it->GetUTF8Text(RIL_SYMBOL));
            bool bold, italic, underline, monospace, serif, smallcaps;
            int size, font_id;

            auto font_name = res_it->WordFontAttributes(&bold, &italic, &underline, &monospace, &serif, &smallcaps, &size, &font_id);

            char_bbox.right = min(char_bbox.right, char_bbox.left + (int)(size * 1.5));

            println("{} char {} conf: {} size: {}", text.get(), char_bbox.to_string(), conf, size);

            callback(string(text.get()), conf, font_name ? string(font_name) : string(), line_bbox, word_bbox, char_bbox, size, args);

            debugger.on_char(char_bbox, string(text.get()));

            res_it->Next(RIL_SYMBOL);
        } while (!res_it->Empty(RIL_BLOCK) && !res_it->IsAtBeginningOf(RIL_WORD));
    }

    pixWrite(std::format("{}.ori.png", std::filesystem::path(args.img).filename().string()).c_str(), image.get(), IFF_PNG);
}

int main(int argc, char **argv)
{
    const auto args = Args::from_args(argc, argv);
    args.print();

    ocr_recognise(args, char_callback);

    return 0;
}
