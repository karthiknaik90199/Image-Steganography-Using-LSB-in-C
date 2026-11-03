# Image-Steganography-Using-LSB-in-C
A C-based Image Steganography project that uses the Least Significant Bit (LSB) technique to securely hide secret files inside BMP images and recover them through decoding. Implements magic string verification, file extension and size extraction, image capacity validation, and binary file handling.
# Image Steganography Using C

## Project Overview
Image Steganography is a technique used to hide secret information inside an image without visibly changing its appearance.

This project implements Least Significant Bit (LSB) encoding and decoding using the C programming language. It allows users to hide secret files inside BMP images and recover the original files.

## Features
- Encode secret files into BMP images.
- Decode and recover hidden files.
- LSB-based data hiding and extraction.
- Magic string verification.
- Secret file extension extraction.
- Secret file size extraction.
- Image capacity validation.
- Binary file handling.

## Technologies Used
- C Programming
- GCC Compiler
- File Handling
- Pointers and Structures
- Bitwise Operations
- LSB Steganography
- BMP Image Format

## Project Structure
- `main.c` – Main program and operation selection.
- `encode.c` / `encode.h` – Encoding implementation.
- `decode.c` / `decode.h` – Decoding implementation.
- `common.h` – Common definitions and declarations.

## Compilation

```bash
gcc -Wall -Wextra main.c encode.c decode.c -o stego
```

## Usage

### Encoding

```bash
./stego -e beautiful.bmp secret.txt
```

### Decoding

```bash
./stego -d stego.bmp
```

The decoded secret file is saved as `output.txt` in the current implementation.

## Testing and Verification

Successfully encoded a 28-byte text file into a BMP image and decoded it.

Verified the recovered file using:

```bash
fc.exe /b secret.txt output.txt
```

Result: `FC: no differences encountered`

## Learning Outcomes
- Understanding bitwise operations and LSB manipulation.
- Working with binary files in C.
- Understanding pointers, structures and modular programming.
- Implementing file encoding and decoding algorithms.

## Author
Developed as a C programming project.
