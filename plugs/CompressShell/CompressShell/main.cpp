
//// Huffman Coding in C
//
//#include <stdio.h>
//#include <stdlib.h>
//
//#define MAX_TREE_HT 1024
//
//struct MinHNode {
//	unsigned char item;
//	unsigned freq;
//	struct MinHNode *left, *right;
//};
//
//struct MinHeap {
//	unsigned size;
//	unsigned capacity;
//	struct MinHNode **array;
//};
//
//// Create nodes
//struct MinHNode *newNode(char item, unsigned freq) {
//	struct MinHNode *temp = (struct MinHNode *)malloc(sizeof(struct MinHNode));
//
//	temp->left = temp->right = NULL;
//	temp->item = item;
//	temp->freq = freq;
//
//	return temp;
//}
//
//// Create min heap
//struct MinHeap *createMinH(unsigned capacity) {
//	struct MinHeap *minHeap = (struct MinHeap *)malloc(sizeof(struct MinHeap));
//
//	minHeap->size = 0;
//
//	minHeap->capacity = capacity;
//
//	minHeap->array = (struct MinHNode **)malloc(minHeap->capacity * sizeof(struct MinHNode *));
//	return minHeap;
//}
//
//// Function to swap
//void swapMinHNode(struct MinHNode **a, struct MinHNode **b) {
//	struct MinHNode *t = *a;
//	*a = *b;
//	*b = t;
//}
//
//// Heapify
//void minHeapify(struct MinHeap *minHeap, int idx) {
//	int smallest = idx;
//	int left = 2 * idx + 1;
//	int right = 2 * idx + 2;
//
//	if (left < minHeap->size && minHeap->array[left]->freq < minHeap->array[smallest]->freq)
//		smallest = left;
//
//	if (right < minHeap->size && minHeap->array[right]->freq < minHeap->array[smallest]->freq)
//		smallest = right;
//
//	if (smallest != idx) {
//		swapMinHNode(&minHeap->array[smallest], &minHeap->array[idx]);
//		minHeapify(minHeap, smallest);
//	}
//}
//
//// Check if size if 1
//int checkSizeOne(struct MinHeap *minHeap) {
//	return (minHeap->size == 1);
//}
//
//// Extract min
//struct MinHNode *extractMin(struct MinHeap *minHeap) {
//	struct MinHNode *temp = minHeap->array[0];
//	minHeap->array[0] = minHeap->array[minHeap->size - 1];
//
//	--minHeap->size;
//	minHeapify(minHeap, 0);
//
//	return temp;
//}
//
//// Insertion function
//void insertMinHeap(struct MinHeap *minHeap, struct MinHNode *minHeapNode) {
//	++minHeap->size;
//	int i = minHeap->size - 1;
//
//	while (i && minHeapNode->freq < minHeap->array[(i - 1) / 2]->freq) {
//		minHeap->array[i] = minHeap->array[(i - 1) / 2];
//		i = (i - 1) / 2;
//	}
//	minHeap->array[i] = minHeapNode;
//}
//
//void buildMinHeap(struct MinHeap *minHeap) {
//	int n = minHeap->size - 1;
//	int i;
//
//	for (i = (n - 1) / 2; i >= 0; --i)
//		minHeapify(minHeap, i);
//}
//
//int isLeaf(struct MinHNode *root) {
//	return !(root->left) && !(root->right);
//}
//
//struct MinHeap *createAndBuildMinHeap(char item[], int freq[], int size) {
//	struct MinHeap *minHeap = createMinH(size);
//
//	for (int i = 0; i < size; ++i)
//		minHeap->array[i] = newNode(item[i], freq[i]);
//
//	minHeap->size = size;
//	buildMinHeap(minHeap);
//
//	return minHeap;
//}
//
////从概率数组生成一棵霍夫曼树
//struct MinHNode *buildHuffmanTree(char item[], int freq[], int size) {
//	struct MinHNode *left, *right, *top;
//	struct MinHeap *minHeap = createAndBuildMinHeap(item, freq, size);
//
//	//利用最小堆,取出概率最小的两棵子树,合成一颗新的树,然后加入到最小堆中.
//	while (!checkSizeOne(minHeap)) {
//		left = extractMin(minHeap);
//		right = extractMin(minHeap);
//
//		top = newNode('$', left->freq + right->freq);
//
//		top->left = left;
//		top->right = right;
//
//		insertMinHeap(minHeap, top);
//	}
//	return extractMin(minHeap);
//}
//
//// Print the array,输出01序列.
////16 * 8 == 128
//
//struct Byte
//{
//	unsigned int	enc_len;		//编码后的位数
//	unsigned char	enc_bits[16];  
//};
//
//#include <string.h>
//
//Byte BytesTable[256];
//
//
//void printHCodes(struct MinHNode *root, int arr[], int top) {
//	if (root->left) {
//		arr[top] = 0;
//		printHCodes(root->left, arr, top + 1);
//	}
//	if (root->right) {
//		arr[top] = 1;
//		printHCodes(root->right, arr, top + 1);
//	}
//	if (isLeaf(root)) {
//		
//		BytesTable[root->item].enc_len = top;			//编码后的位数.
//		memset(BytesTable[root->item].enc_bits, 0, 16);	//
//		for (int i = 0; i < top; i++){
//			BytesTable[root->item].enc_bits[i/8] |= (arr[i] << (i%8));
//		}
//	}
//}
//
//// Wrapper function
//void HuffmanCodes(char item[], int freq[], int size) {
//	struct MinHNode *root = buildHuffmanTree(item, freq, size);
//
//	int arr[MAX_TREE_HT], top = 0;
//	printHCodes(root, arr, top);
//}
//struct Compress{
//	unsigned char*buffer;
//	unsigned long long bits;
//	unsigned int bytes;
//};
//
//void compress(Byte*table, unsigned char*data, unsigned int len, Compress*result){
//	unsigned long long compressbits = 0;						//生成的bits
//	unsigned int bufsize = 0x1000;
//	unsigned char *compressdata = (unsigned char*)calloc(1,0x1000);
//	unsigned int compresslen = 0;
//
//	for (int i = 0; i < len; i++){
//		unsigned int needsize = (compressbits + table[data[i]].enc_len + 7) & 0xfffffff8;
//		//扩充buffer
//		if (needsize >= bufsize){
//			unsigned char*newbuf = (unsigned char*)calloc(1, bufsize * 2);
//			memcpy(newbuf, compressdata, bufsize);
//			free(compressdata);
//
//			compressdata = newbuf;
//			bufsize *= 2;
//		}
//		unsigned int left = table[data[i]].enc_len;
//		unsigned int enc_len = left;
//		unsigned char*enc_bits = table[data[i]].enc_bits;
//
//		while (left > 0){
//			unsigned int pad = 8 - (compressbits % 8);				//差多少够一个字节
//			unsigned int compress = pad < left ? pad : left;
//			
//			unsigned int pos = compressbits / 8;
//
//			unsigned char or = enc_bits[(enc_len - left) / 8] >> ((enc_len-left) % 8);
//			if (compress > (8 - ((enc_len - left) % 8))){
//				//读取下一个字节的bits.
//				or |= enc_bits[(enc_len - left) / 8 + 1] << (8 - ((enc_len - left) % 8));
//			}
//
//			compressdata[pos] |= (unsigned char)(or << (8 - pad));
//			left -= compress;
//			compressbits += compress;
//		}
//	}
//
//	result->buffer = compressdata;
//	result->bits = compressbits;
//	result->bytes = ((compressbits + 7) & 0xfffffff8) / 8;		//向上取整对齐.
//}
//
//int comparebits(unsigned char*s, unsigned char*t, unsigned int off1, unsigned int off2, unsigned int len){
//	while (len > 0){
//		unsigned char byte1 = s[off1 / 8];
//		unsigned char byte2 = t[off2 / 8];
//		byte1 >>= off1 % 8;
//		byte2 >>= off2 % 8;
//		if ((byte1 & 0x1) != (byte2 & 0x1)){
//			return -1;
//		}
//		len--;
//		off1++, off2++;
//	}
//	return 0;
//}
//int decompress(Compress*compressdata,unsigned char*&out_buff,unsigned int&out_len){
//	unsigned char* buff = (unsigned char*)malloc(0x1000);
//	unsigned int buffsize = 0x1000;
//	unsigned int decompresslen = 0;
//
//	unsigned long long bits = compressdata->bits;
//	unsigned char*data = compressdata->buffer;
//	unsigned long long pos = 0;
//	while (pos < bits){
//		unsigned int i = 0;
//		//不需要全部遍历,可以用树来加快寻找速度
//		// 0左,1右...
//		for (; i < 256; i++){
//			if (((bits - pos) >= BytesTable[i].enc_len) &&
//				!comparebits(data, BytesTable[i].enc_bits, 
//				pos, 0, BytesTable[i].enc_len)){
//
//				if (decompresslen >= buffsize){
//					buff = (unsigned char*)realloc(buff, buffsize * 2);
//					buffsize *= 2;
//				}
//
//				buff[decompresslen++] = (unsigned char)i;
//				pos += BytesTable[i].enc_len;
//				break;
//			}
//		}
//		if (i == 256){
//			return -1;
//		}
//	}
//	out_buff = buff;
//	out_len = decompresslen;
//	return 0;
//}
//

///*
//	最坏的情况是高度为多少??? (256 个字节)
//*/
#include "rle_compress.h"
#include <stdio.h>

int main() {
	//FILE*fp = fopen("D:\\RAT\\client_exe\\Client\\Release\\client.exe","rb");
	//if (fp){
	//	unsigned char bytes[256] = { 0 };
	//	int count[256] = { 0 };
	//	fseek(fp, 0, SEEK_END);
	//	int len = ftell(fp);
	//	unsigned char*buffer = new unsigned char[len];
	//	fseek(fp, 0, SEEK_SET);
	//	int nRead = fread(buffer, 1, len, fp);

	//	//////rel压缩.
	//	byte* encode_data = 0;
	//	size_t encode_len = 0;
	//	rle_encode(buffer, len, encode_data, encode_len);

	//	rle_decode(encode_data, encode_len, encode_data, encode_len);
	//	//fclose(fp);
	//	//fp = fopen("rat.exe", "wb");
	//	//if (fp){
	//	//	fwrite(encode_data, 1, encode_len, fp);
	//	//	fclose(fp);
	//	//}
	//	//decompress(&result, module, datalen);
	//	/*DWORD dwStart = GetTickCount();
	//	for (int i = 0; i < 1000; i++){
	//		unsigned char*data = NULL;
	//		size_t out_len = 0;
	//		rel_encode(buffer, len, data, out_len);
	//		rel_decode(data, out_len, data, out_len);
	//	}
	//	printf("%d\n", GetTickCount() - dwStart);*/
	//	fclose(fp);
	//}

	return 0;
}
#include "rle_compress.h"
void MyEntry(){
	byte* data;
	size_t len;
	byte s[] = { 12, 3, 2, 4,234 , 5, 6, 7, 3, 2, 23, 3, 45,42 , 6, 7, 8, 9, 45, 45, 46, 4, 2, 34, 3,};
	rle_encode(s, sizeof(s), data, len);
}
