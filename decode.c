#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>

#include "decode.h"
#include "common.h"

/* Validate arguments */
Status read_and_validate_decode_args(
    char *argv[], int argc, DecodeInfo *decInfo)
{
    char *ext;

    if (argc < 3 || argc > 4)
    {
        printf("Usage: ./stego -d stego.bmp [output.txt]\n");
        return failure;
    }

    ext = strrchr(argv[2], '.');

    if (ext == NULL || strcmp(ext, ".bmp") != 0)
    {
        printf("Error: Input image must have .bmp extension.\n");
        return failure;
    }

    decInfo->stego_image_fname = argv[2];

    if (argc == 4)
        decInfo->output_fname = argv[3];
    else
        decInfo->output_fname = "output.txt";

    if (strcmp(decInfo->stego_image_fname,
               decInfo->output_fname) == 0)
    {
        printf("Error: Input and output filenames must differ.\n");
        return failure;
    }

    return success;
}

/* Open the input image and output file */
Status open_decode_files(DecodeInfo *decInfo)
{
    decInfo->stego_image_fptr =
        fopen(decInfo->stego_image_fname, "rb");

    if (decInfo->stego_image_fptr == NULL)
    {
        perror("Error opening stego image");
        return failure;
    }

    decInfo->output_fptr =
        fopen(decInfo->output_fname, "wb");

    if (decInfo->output_fptr == NULL)
    {
        perror("Error creating output file");
        fclose(decInfo->stego_image_fptr);
        decInfo->stego_image_fptr = NULL;
        return failure;
    }

    return success;
}

/* Extract one byte from the LSBs of 8 image bytes */
Status decode_byte_from_lsb(
    unsigned char buffer[8], unsigned char *data)
{
    int i;
    unsigned char value = 0;

    for (i = 0; i < 8; i++)
    {
        value = (unsigned char)(
            (value << 1) | (buffer[i] & 1U));
    }

    *data = value;
    return success;
}

/* Extract a 32-bit integer from the LSBs of 32 image bytes */
Status decode_4byte_from_lsb(
    unsigned char buffer[32], uint32_t *data)
{
    int i;
    uint32_t value = 0;

    for (i = 0; i < 32; i++)
    {
        value = (value << 1) | (buffer[i] & 1U);
    }

    *data = value;
    return success;
}

/* Decode and verify magic string */
Status decode_magic_string(DecodeInfo *decInfo)
{
    unsigned char buffer[8];
    unsigned char ch;
    char magic_string[sizeof(MAGIC_STRING)];
    size_t i;
    size_t magic_len = strlen(MAGIC_STRING);

    for (i = 0; i < magic_len; i++)
    {
        if (fread(buffer, 1, 8,
                  decInfo->stego_image_fptr) != 8)
        {
            return failure;
        }

        if (decode_byte_from_lsb(buffer, &ch) != success)
            return failure;

        magic_string[i] = (char)ch;
    }

    magic_string[magic_len] = '\0';

    if (strcmp(magic_string, MAGIC_STRING) == 0)
    {
        printf("Magic string verified successfully.\n");
        return success;
    }

    printf("Error: Magic string does not match.\n");
    return failure;
}

/* Decode the secret file extension */
Status decode_secret_file_extn(DecodeInfo *decInfo)
{
    unsigned char buffer[32];
    unsigned char ch;
    uint32_t extn_size;
    uint32_t i;

    /* Read extension length */
    if (fread(buffer, 1, 32,
              decInfo->stego_image_fptr) != 32)
    {
        return failure;
    }

    //if (decode_4byte_from_lsb(buffer, &extn_size) != success)
      //  return failure;
       // printf("DEBUG: decoded extn_size = %u\n",
       //(unsigned int)extn_size);
       if (decode_4byte_from_lsb(buffer, &extn_size) != success)
{
    return failure;
}

printf("DEBUG: decoded extn_size = %u\n",
       (unsigned int)extn_size);


    /* secret_extn can store at most 6 characters plus '\0' */
    if (extn_size == 0 || extn_size >= sizeof(decInfo->secret_extn))
    {
        printf("Error: Invalid secret file extension length.\n");
        return failure;
    }

    decInfo->secret_extn_size = extn_size;

    /* Read each extension character */
    for (i = 0; i < extn_size; i++)
    {
        if (fread(buffer, 1, 8,
                  decInfo->stego_image_fptr) != 8)
        {
            return failure;
        }

        if (decode_byte_from_lsb(buffer, &ch) != success)
            return failure;

        decInfo->secret_extn[i] = (char)ch;
    }

    decInfo->secret_extn[extn_size] = '\0';

    printf("Secret file extension: %s\n", decInfo->secret_extn);

    return success;
}

/* Decode the secret file size */
Status decode_secret_file_size(DecodeInfo *decInfo)
{
    unsigned char buffer[32];
    uint32_t file_size;

    if (fread(buffer, 1, 32,
              decInfo->stego_image_fptr) != 32)
    {
        return failure;
    }

    if (decode_4byte_from_lsb(buffer, &file_size) != success)
        return failure;

    decInfo->secret_file_size = file_size;

    printf("Secret file size: %u bytes\n",
           (unsigned int)decInfo->secret_file_size);

    return success;
}

/* Extract the secret file data */
Status decode_secret_file_data(DecodeInfo *decInfo)
{
    unsigned char buffer[8];
    unsigned char data;
    uint32_t i;

    for (i = 0; i < decInfo->secret_file_size; i++)
    {
        if (fread(buffer, 1, 8,
                  decInfo->stego_image_fptr) != 8)
        {
            printf("Error: Unexpected end of image data.\n");
            return failure;
        }

        if (decode_byte_from_lsb(buffer, &data) != success)
            return failure;

        if (fwrite(&data, 1, 1,
                   decInfo->output_fptr) != 1)
        {
            printf("Error writing extracted data.\n");
            return failure;
        }
    }

    return success;
}

/* Main decoding controller */
Status do_decoding(DecodeInfo *decInfo)
{
    Status status = failure;
if (open_decode_files(decInfo) != success)
{
    return failure;
}

    /*
     * Basic version assumes a 54-byte BMP header.
     * For other BMP files, use the pixel-data offset
     * stored in the BMP header.
     */
    if (fseek(decInfo->stego_image_fptr, 54, SEEK_SET) != 0)
    {
        printf("Error: Unable to seek to image data.\n");
        goto cleanup;
    }

    if (decode_magic_string(decInfo) != success)
        goto cleanup;

    if (decode_secret_file_extn(decInfo) != success)
        goto cleanup;

    if (decode_secret_file_size(decInfo) != success)
        goto cleanup;

    if (decode_secret_file_data(decInfo) != success)
        goto cleanup;

    if (fflush(decInfo->output_fptr) != 0)
        goto cleanup;

    printf("Decoding completed successfully.\n");
    printf("Output file: %s\n", decInfo->output_fname);

    status = success;

cleanup:
    if (decInfo->stego_image_fptr != NULL)
    {
        fclose(decInfo->stego_image_fptr);
        decInfo->stego_image_fptr = NULL;
    }

    if (decInfo->output_fptr != NULL)
    {
        fclose(decInfo->output_fptr);
        decInfo->output_fptr = NULL;
    }

    return status;
}