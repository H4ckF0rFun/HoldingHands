#include <Windows.h>

typedef unsigned char byte;

void rle_decode(byte encode[], size_t len, byte*&out_data, size_t&out_len);

void rle_encode(byte data[], size_t len, byte*&out_data, size_t&out_len);
