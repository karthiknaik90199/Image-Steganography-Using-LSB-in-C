#include <stdio.h>
#include "encode.h"
#include "common.h"
#include <string.h>
Opr_type check_operation(char *option)
{
    if (strcmp(option, "-e") == 0)
    {
        return e_encode;
    }
    else if (strcmp(option, "-d") == 0)
    {
        return e_decode;
    }
    else
    {
        return unsupported;
    }
}


unsigned int get_file_size(FILE *fptr)
{
    unsigned int size;
    fseek(fptr, 0, SEEK_END);
    size = ftell(fptr);
    rewind(fptr);
    return size;
}
unsigned int get_image_size_for_bmp(FILE *fptr_image)
{
    unsigned int width, height;
    // Seek to 18th byte
    fseek(fptr_image, 18, SEEK_SET);

    // Read the width (an int)
    fread(&width, sizeof(int), 1, fptr_image);
    printf("width = %u\n", width);

    // Read the height (an int)22
    fread(&height, sizeof(int), 1, fptr_image);
    printf("height = %u\n", height);

    // Return image capacity
    return width * height * 3;
}




Status open_files(EncodeInfo *encInfo)
{
    encInfo->src_image_fptr =
        fopen(encInfo->src_image_fname, "rb");

    if (encInfo->src_image_fptr == NULL)
    {
        perror("Unable to open source image");
        return failure;
    }

    encInfo->secret_fptr =
        fopen(encInfo->secret_fname, "rb");

    if (encInfo->secret_fptr == NULL)
    {
        perror("Unable to open secret file");
        fclose(encInfo->src_image_fptr);
        encInfo->src_image_fptr = NULL;
        return failure;
    }

    encInfo->output_image_fptr =
        fopen(encInfo->output_image_fname, "wb");

    if (encInfo->output_image_fptr == NULL)
    {
        perror("Unable to create output image");
        fclose(encInfo->src_image_fptr);
        fclose(encInfo->secret_fptr);
        encInfo->src_image_fptr = NULL;
        encInfo->secret_fptr = NULL;
        return failure;
    }

    return success;
}


Status check_capacity(EncodeInfo *encInfo)
{
    unsigned int image_size;
    unsigned int secret_size;
    unsigned int required_size;

    
    image_size = get_image_size_for_bmp(encInfo->src_image_fptr);

    secret_size = get_file_size(encInfo->secret_fptr);

    encInfo->secret_file_size = secret_size;

    required_size =
        8U * (strlen(MAGIC_STRING)+ 4U + encInfo->secret_extn_size + 4U + secret_size);

    printf("Image capacity: %u bytes\n", image_size);
    printf("Secret file size: %u bytes\n", secret_size);
    printf("Required capacity: %u bytes\n", required_size);

    if (image_size >= required_size)
    {
        printf("Image capacity is sufficient.\n");
        rewind(encInfo->src_image_fptr);
        rewind(encInfo->secret_fptr);
        return success;
    }

    printf("Error: Image capacity is insufficient.\n");
    return failure;
}
Status copy_bmp_header(EncodeInfo *encInfo)
{
    char header[BMP_HEADER_SIZE];

    
    if (fread(header, 1, BMP_HEADER_SIZE,
              encInfo->src_image_fptr) != BMP_HEADER_SIZE)
    {
        printf("Error: Unable to read BMP header\n");
        return failure;
    }


    if (fwrite(header, 1, BMP_HEADER_SIZE,
               encInfo->output_image_fptr) != BMP_HEADER_SIZE)
    {
        printf("Error: Unable to write BMP header\n");
        return failure;
    }

    printf("BMP header copied successfully.\n");
    return success;
}
Status encode_1byte_to_lsb(char data, char *buffer_8)
{
    int i;

    for (i = 0; i < 8; i++)
    {
        /* Extract one bit from secret data */
        int bit = (data >> (7 - i)) & 1;

        /* Replace the LSB of the image byte */
        buffer_8[i] = (buffer_8[i] & 0xFE) | bit;
    }

    return success;
}

Status encode_magic_string(const char *magic_string,
                           EncodeInfo *encInfo)
{
    char buffer[8];
    int i;

    for (i = 0; magic_string[i] != '\0'; i++)
    {
        /* Read 8 image bytes */
        if (fread(buffer, 1, 8,
                  encInfo->src_image_fptr) != 8)
        {
            return failure;
        }

        /* Hide one magic-string character */
        if (encode_1byte_to_lsb(magic_string[i],
                                buffer) != success)
        {
            return failure;
        }

        /* Write modified bytes to output image */
        if (fwrite(buffer, 1, 8,
                   encInfo->output_image_fptr) != 8)
        {
            return failure;
        }
    }

    printf("Magic string encoded successfully.\n");
    return success;
}
Status encode_4byte_to_lsb(int data, char *buffer_32)
{
    int i;

    for (i = 0; i < 32; i++)
    {
        /* Extract one bit from the integer */
        int bit = ((unsigned int)data >> (31 - i)) & 1U;

        /* Replace the image byte's LSB */
        buffer_32[i] =
            (buffer_32[i] & 0xFE) | bit;
    }

    return success;
}
Status encode_secret_file_extn(EncodeInfo *encInfo)
{
    char buffer[32];
    int i;

    /* Step 1: Encode extension length */
    if (fread(buffer, 1, 32, encInfo->src_image_fptr) != 32)
    {
        return failure;
    }

    if (encode_4byte_to_lsb(encInfo->secret_extn_size,
                            buffer) != success)
    {
        return failure;
    }

    if (fwrite(buffer, 1, 32,
               encInfo->output_image_fptr) != 32)
    {
        return failure;
    }

    /* Step 2: Encode each extension character */
    for (i = 0; i < encInfo->secret_extn_size; i++)
    {
        char byte_buffer[8];

        if (fread(byte_buffer, 1, 8,
                  encInfo->src_image_fptr) != 8)
        {
            return failure;
        }

        if (encode_1byte_to_lsb(encInfo->secret_extn[i],
                                byte_buffer) != success)
        {
            return failure;
        }

        if (fwrite(byte_buffer, 1, 8,
                   encInfo->output_image_fptr) != 8)
        {
            return failure;
        }
    }

    printf("Secret file extension encoded successfully.\n");
    return success;
}
Status encode_secret_file_data(EncodeInfo *encInfo)
{
    char data;
    char buffer[8];
    unsigned long int i;


    rewind(encInfo->secret_fptr);

    for (i = 0; i < encInfo->secret_file_size; i++)
    {
        if (fread(&data, 1, 1,
                  encInfo->secret_fptr) != 1)
        {
            return failure;
        }

        
        if (fread(buffer, 1, 8,
                  encInfo->src_image_fptr) != 8)
        {
            return failure;
        }

        
        if (encode_1byte_to_lsb(data, buffer) != success)
        {
            return failure;
        }

        
        if (fwrite(buffer, 1, 8,
                   encInfo->output_image_fptr) != 8)
        {
            return failure;
        }
    }

    printf("Secret file data encoded successfully.\n");

    return success;
}

Status validate_encode_args(char *argv[], EncodeInfo *encInfo)
{
    if (argv == NULL || encInfo == NULL ||
        argv[2] == NULL || argv[3] == NULL)
    {
        return failure;
    }

    encInfo->src_image_fname = argv[2];
    encInfo->secret_fname = argv[3];

    encInfo->output_image_fname =
        (argv[4] != NULL) ? argv[4] : "stego.bmp";

    // Extract secret file extension
    const char *dot = strrchr(encInfo->secret_fname, '.');

    if (dot == NULL || strlen(dot) >= sizeof(encInfo->secret_extn))
    {
        printf("Error: Invalid or unsupported secret file extension.\n");
        return failure;
    }

    strcpy(encInfo->secret_extn, dot);
    encInfo->secret_extn_size = (int)strlen(dot);

    return success;
}

Status encode_secret_file_size(EncodeInfo *encInfo)
{
    char buffer[32];

    if (fread(buffer, 1, 32, encInfo->src_image_fptr) != 32)
    {
        return failure;
    }

    if (encode_4byte_to_lsb(
            (int)encInfo->secret_file_size, buffer) != success)
    {
        return failure;
    }

    if (fwrite(buffer, 1, 32, encInfo->output_image_fptr) != 32)
    {
        return failure;
    }

    return success;
}

Status copy_remaining_img_data(EncodeInfo *encInfo)
{
    unsigned char buffer[4096];
    size_t bytes_read;
    unsigned long total_copied = 0;

    printf("Source position before copy: %ld\n",
           ftell(encInfo->src_image_fptr));

    while ((bytes_read = fread(buffer, 1, sizeof(buffer),
                              encInfo->src_image_fptr)) > 0)
    {
        size_t bytes_written = fwrite(
            buffer, 1, bytes_read, encInfo->output_image_fptr);

        if (bytes_written != bytes_read)
        {
            perror("fwrite");
            return failure;
        }

        total_copied += bytes_written;
    }

    printf("Bytes copied: %lu\n", total_copied);
    printf("Source position after copy: %ld\n",
           ftell(encInfo->src_image_fptr));

    if (ferror(encInfo->src_image_fptr))
    {
        perror("fread");
        return failure;
    }

    return success;
}

Status do_encoding(char *argv[], EncodeInfo *encInfo)
{
    Status result = failure;

    (void)argv;

    if (encInfo == NULL)
    {
        return failure;
    }

    if (open_files(encInfo) != success)
    {
        return failure;
    }

    if (check_capacity(encInfo) != success)
    {
        goto cleanup;
    }

    if (copy_bmp_header(encInfo) != success)
    {
        printf("ERROR: Copy BMP header failed.\n");
        goto cleanup;
    }

    if (encode_magic_string(MAGIC_STRING, encInfo) != success)
    {
        printf("ERROR: Encoding magic string failed.\n");
        goto cleanup;
    }

    if (encode_secret_file_extn(encInfo) != success)
    {
        printf("ERROR: Encoding extension failed.\n");
        goto cleanup;
    }

    if (encode_secret_file_size(encInfo) != success)
    {
        printf("ERROR: Encoding file size failed.\n");
        goto cleanup;
    }

    if (encode_secret_file_data(encInfo) != success)
    {
        printf("ERROR: Encoding secret data failed.\n");
        goto cleanup;
    }

    if (copy_remaining_img_data(encInfo) != success)
    {
        printf("ERROR: Copying remaining image data failed.\n");
        goto cleanup;
    }

    printf("DEBUG: Source position = %ld\n",
           ftell(encInfo->src_image_fptr));

    printf("DEBUG: Output position = %ld\n",
           ftell(encInfo->output_image_fptr));

    if (fflush(encInfo->output_image_fptr) != 0)
    {
        perror("Output flush failed");
        goto cleanup;
    }

    result = success;

cleanup:
    if (encInfo->src_image_fptr != NULL)
    {
        fclose(encInfo->src_image_fptr);
        encInfo->src_image_fptr = NULL;
    }

    if (encInfo->secret_fptr != NULL)
    {
        fclose(encInfo->secret_fptr);
        encInfo->secret_fptr = NULL;
    }

    if (encInfo->output_image_fptr != NULL)
    {
        fclose(encInfo->output_image_fptr);
        encInfo->output_image_fptr = NULL;
    }

    return result;
}
