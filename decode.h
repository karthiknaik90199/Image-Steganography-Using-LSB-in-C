#ifndef DECODE_H
#define DECODE_H

#include <stdio.h>
#include <stdint.h>
#include "common.h"

typedef struct DecodeInfo
{
    char *stego_image_fname;
    FILE *stego_image_fptr;

    char *output_fname;
    FILE *output_fptr;

    char secret_extn[7];
    uint32_t secret_extn_size;
    uint32_t secret_file_size;

} DecodeInfo;

/* Validate command-line arguments */
Status read_and_validate_decode_args(
    char *argv[], int argc, DecodeInfo *decInfo);

/* Open files */
Status open_decode_files(DecodeInfo *decInfo);
/* Main decoding function */
Status do_decoding(DecodeInfo *decInfo);

/* Decode metadata */
Status decode_magic_string(DecodeInfo *decInfo);
Status decode_secret_file_extn(DecodeInfo *decInfo);
Status decode_secret_file_size(DecodeInfo *decInfo);
Status decode_secret_file_data(DecodeInfo *decInfo);

/* Extract bits */
Status decode_byte_from_lsb(
    unsigned char buffer[8], unsigned char *data);

Status decode_4byte_from_lsb(
    unsigned char buffer[32], uint32_t *data);

#endif