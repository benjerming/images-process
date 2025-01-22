#include "args.h"
#include "recognise.h"

int main(int argc, char **argv)
{
    const auto args = Args::from(argc, argv);
    args.print();

    Recognise::ocr_recognise(args);
    Recognise::tables_recognise(args);

    return 0;
}
