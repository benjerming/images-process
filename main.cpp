#include "args.h"
#include "recognise.h"
#include "test_ocr.h"

int main(int argc, char **argv)
{
    const auto args = Args::from(argc, argv);
    args.print();

    // Recognise::ocr_recognise(args);
    // Recognise::tables_recognise(args);

    // auto page = Recognise::texts_recognise(args.images.front(), args);
    fixed2_debugger::Page page;
    auto segments = Recognise::segments_recognise(args.images.front(), args);

    Recognise::filter_segments(segments, page, args.images.front());

    // test_ocr(args.images.front(), args.tessdata, args.lang);
    return 0;
}
