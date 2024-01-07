
#include <Windows.h>

void __stdcall StartKernel(CONST CHAR * szServerAddress, USHORT Port);

int main()
{
	StartKernel("127.0.0.1", 10086);
	return 0;
}