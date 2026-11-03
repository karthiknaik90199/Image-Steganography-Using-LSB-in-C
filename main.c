
#include <stdio.h>
#include "encode.h"
#include "decode.h"

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        printf("Error: No arguments provided.\n");
        printf("Usage:\n");
        printf("Encode: ./stego -e input.bmp secret.txt [output.bmp]\n");
        printf("Decode: ./stego -d stego.bmp [output.txt]\n");
        return 1;
    }

    int opr = check_operation(argv[1]);

    if (opr == e_encode)
    {
        if (argc < 4 || argc > 5)
        {
            printf("Error: Invalid number of arguments.\n");
            printf("Usage: ./stego -e input.bmp secret.txt [output.bmp]\n");
            return 1;
        }

        EncodeInfo encInfo = {0};

        if (validate_encode_args(argv, &encInfo) != success)
        {
            printf("Error: Invalid encoding arguments.\n");
            return 1;
        }

        if (do_encoding(argv, &encInfo) != success)
        {
            printf("Encoding failed.\n");
            return 1;
        }

        printf("Encoding completed successfully.\n");
    }
    else if (opr == e_decode)
    {
        if (argc < 3 || argc > 4)
        {
            printf("Error: Invalid number of arguments.\n");
            printf("Usage: ./stego -d stego.bmp [output.txt]\n");
            return 1;
        }

        DecodeInfo decInfo = {0};

        if (read_and_validate_decode_args(argv, argc, &decInfo) != success)
        {
            printf("Error: Invalid decoding arguments.\n");
            return 1;
        }

        if (do_decoding(&decInfo) != success)
        {
            printf("Decoding failed.\n");
            return 1;
        }

        printf("Decoding completed successfully.\n");
    }
    else
    {
        printf("Error: Unsupported operation.\n");
        printf("Use -e for encoding or -d for decoding.\n");
        return 1;
    }

    return 0;
}
