#pragma once

#include <memory>
#include <string>

#include <stdio.h>
#include <tesseract/capi.h>
#include <leptonica/allheaders.h>

static int ocr(const std::string &image_path, const std::string &tessdata, const std::string &lang)
{
    auto api = std::shared_ptr<TessBaseAPI>(
        TessBaseAPICreate(), 
        [](TessBaseAPI *p) { TessBaseAPIDelete(p); }
    );
    if (api->Init(tessdata.c_str(), lang.c_str()))
    {
        fprintf(stderr, "Could not initialize tesseract.\n");
        return -1;
    }
    auto image = std::shared_ptr<Pix>(
        pixRead(image_path.c_str()),
        [](Pix *p) { pixDestroy(&p); }
    );

    api->SetImage(image.get());

    if (api->Recognize(nullptr))
    {
        fprintf(stderr, "Recognize failed\n");
        return -1;
    }

    auto res_it = std::shared_ptr<tesseract::ResultIterator>(api->GetIterator());


    fprintf(stderr, "%4s %4s %4s %4s %4s\n", "c", "l", "t", "r", "b");

    while (!res_it->Empty(tesseract::RIL_TEXTLINE))
    {
        if (res_it->Empty(tesseract::RIL_WORD))
        {
            res_it->Next(tesseract::RIL_WORD);
            continue;
        }

        int line_bbox[4], word_bbox[4];
        int line_conf, word_conf;
        res_it->BoundingBox(tesseract::RIL_TEXTLINE, &line_bbox[0], &line_bbox[1], &line_bbox[2], &line_bbox[3]);
        res_it->BoundingBox(tesseract::RIL_WORD, &word_bbox[0], &word_bbox[1], &word_bbox[2], &word_bbox[3]);
        line_conf = res_it->Confidence(tesseract::RIL_TEXTLINE);
        word_conf = res_it->Confidence(tesseract::RIL_WORD);

        // auto line_box = std::shared_ptr<Box>(
        //     boxCreate(line_bbox[0], line_bbox[1], line_bbox[2] - line_bbox[0], line_bbox[3] - line_bbox[1]),
        //     [](Box *p){ boxDestroy(&p);}
        // );
        // pixRenderBoxArb(image.get(), line_box.get(), 1, 0xff, 0xff, 0);

        // auto word_box = std::shared_ptr<Box>(
        //     boxCreate(word_bbox[0], word_bbox[1], word_bbox[2] - word_bbox[0], word_bbox[3] - word_bbox[1]),
        //     [](Box *p){ boxDestroy(&p);}
        // );
        // pixRenderBoxArb(image.get(), word_box.get(), 1, 0, 0xff, 0);

        do
        {
            int char_bbox[4];
            res_it->BoundingBox(tesseract::RIL_SYMBOL, &char_bbox[0], &char_bbox[1], &char_bbox[2], &char_bbox[3]);
            auto text = std::shared_ptr<char>(res_it->GetUTF8Text(tesseract::RIL_SYMBOL));

            fprintf(stderr, "%4s %4d %4d %4d %4d\n",
                    text.get(), char_bbox[0], char_bbox[1], char_bbox[2], char_bbox[3]);

            auto box = std::shared_ptr<Box>(
                boxCreate(char_bbox[0], char_bbox[1], char_bbox[2] - char_bbox[0], char_bbox[3] - char_bbox[1]),
                [](Box *p){ boxDestroy(&p);}
            );
            pixRenderBoxArb(image.get(), box.get(), 1, 0, 0, 0xff);

            res_it->Next(tesseract::RIL_SYMBOL);
        } while (!res_it->Empty(tesseract::RIL_BLOCK) && !res_it->IsAtBeginningOf(tesseract::RIL_WORD));
    }

    const auto ocr_box_image_path = image_path + ".ocr_box.png";
    if (pixWrite(ocr_box_image_path.c_str(), image.get(), IFF_PNG))
    {
        fprintf(stderr, "Failed to write ocr box image to %s\n", ocr_box_image_path.c_str());
        return -1;
    }

    return 0;
}
