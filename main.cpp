#include "args.h"
#include "recognise.h"
#include "test_ocr.h"

int main(int argc, char **argv)
{
    const auto args = Args::from(argc, argv);
    args.print();

    Recognise::ocr_recognise(args);
    Recognise::tables_recognise(args);

    // test_ocr(args.images.front(), args.tessdata, args.lang);
    return 0;
}
