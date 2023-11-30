#include "rle_compress.h"

bool __inline is_repeat(byte data[]){
	return (data[0] == data[1] && data[0] == data[2]);
}


void rle_decode(byte encode[], size_t len, byte*&out_data, size_t&out_len){
	size_t i = 0;

	HGLOBAL hBuf = GlobalAlloc(GHND, 0x1000);

	byte* buff = (byte*)GlobalLock(hBuf);

	size_t bufsize = 0x1000;
	size_t decompresslen = 0;

	while (i<len){
		size_t _len = (encode[i] & 0x7f);
		if ((decompresslen + _len) >= bufsize){
			//realloc memory
			GlobalUnlock(hBuf);
			hBuf = GlobalReAlloc(hBuf, bufsize * 2, GHND);

			buff = (byte*)GlobalLock(hBuf);
			bufsize *= 2;
		}
		if (encode[i] & 0x80){
			RtlCopyMemory(buff + decompresslen, encode + i + 1, _len);	//not repeat
			i += (1 + _len);
		}
		else{
			memset(buff + decompresslen, encode[i + 1], _len);			//set repeat byte
			i += 2;
		}
		decompresslen += _len;
	}
	out_data = buff;
	out_len = decompresslen;
}
void rle_encode(byte data[], size_t len, byte*&out_data, size_t&out_len){

	HGLOBAL hBuff = GlobalAlloc(GHND, 0x1000);
	byte* buff = (byte*)GlobalLock(hBuff);
	size_t bufsize = 0x1000;
	size_t compresslen = 0;

	size_t i = 0;
	byte* data_end = data + len;

	while (i < len){
		//计算重复长度
		if ((len - i) >= 3 && is_repeat(data + i)){
			size_t repeatlen = 3;
			byte*p = data + i + 3;
			while (p < data_end && repeatlen < 127 && data[i] == p[0]){
				repeatlen++, p++;
			}

			if (compresslen + 2 >= bufsize){
				GlobalUnlock(hBuff);
				hBuff = GlobalReAlloc(hBuff, bufsize * 2, GHND);
				buff = (byte*)GlobalLock(hBuff);
				bufsize *= 2;
			}
			//保存数据.
			buff[compresslen] = (byte)repeatlen;
			buff[compresslen + 1] = (byte)data[i];
			compresslen += 2;
			//调整i
			i += repeatlen;
		}
		else{
			size_t not_repeatlen = 1;
			byte*p = data + i + 1;

			while (not_repeatlen < 127 && p < data_end &&
				((p + 3 < data_end && !is_repeat(p)) || p + 3 >= data_end)){
				p++, not_repeatlen++;
			}
			if ((compresslen + 1 + not_repeatlen) >= bufsize){
				GlobalUnlock(hBuff);
				hBuff = GlobalReAlloc(hBuff, bufsize * 2, GHND);
				buff = (byte*)GlobalLock(hBuff);
				bufsize *= 2;
			}
			buff[compresslen] = 0x80 | not_repeatlen;
			memcpy(buff + compresslen + 1, data + i, not_repeatlen);
			compresslen += (1 + not_repeatlen);

			i += not_repeatlen;
		}
	}
	out_data = buff;
	out_len = compresslen;
}