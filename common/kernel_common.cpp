#include "stdafx.h"
#include "kernel_common.h"
#include "..\Client\Kernel\Kernel.h"
#include <Windows.h>

/*
TYPE

type_value   | value
type_pointer | length | data
*/

Params * deserialize_kernel_params(BYTE * lpData, UINT Size)
{
	BYTE * end = lpData + Size;
	BYTE * p = lpData;

	Params * lpParams = NULL;
	UINT params_cnt = 0;
	LPVOID *params = NULL;
	ParamInfo * param_info = NULL;

scan_params:
	while (p < end)
	{
		BYTE type = *p++;

		if (type == type_value)
		{
			if ((p + 4) > end){
				goto failed;
			}

			if (lpParams)
			{
				lpParams->num++;
				param_info->type = type_value;
				param_info->flag = 0;

				memcpy(params, p, 4);

				++param_info;
				++params;
			}

			p += 4;
		}
		else if (type == type_pointer)
		{
			int data_length = 0;
			if ((p + 4) > end){
				goto failed;
			}
			memcpy(&data_length, p, 4);
			p += 4;

			if (p + data_length > end){
				goto failed;
			}

			if (lpParams)
			{
				lpParams->num++;
				param_info->type = type_pointer;
				param_info->flag = PARAM_FLAG_RELEASE;

				if (data_length)
				{
					*params = malloc(data_length);
					memcpy(*params, p, data_length);
				}
				else
				{
					*params = NULL;
				}
				
				++param_info;
				++params;
			}

			p += data_length;
		}
		else
		{
			goto failed;
		}

		params_cnt++;
	}

	if (lpParams == NULL)
	{
		p = lpData;

		lpParams = (Params*)calloc(1,
			sizeof(Params) +
			sizeof(void*) * params_cnt +
			sizeof(ParamInfo) * params_cnt);

		lpParams->num = 0;
		lpParams->release = (kernel_params_release);

		params = (LPVOID*)(lpParams + 1);
		param_info = (ParamInfo*)(params + params_cnt);

		goto scan_params;
	}

	return lpParams;

failed:

	if (lpParams)
	{
		params = (LPVOID*)(lpParams + 1);
		param_info = (ParamInfo*)(params + params_cnt);

		for (int i = 0; i < lpParams->num; i++)
		{
			if (param_info[i].type == type_pointer &&
				param_info[i].flag & PARAM_FLAG_RELEASE)
			{
				free(params[i]);
			}
		}

		free(lpParams);
		lpParams = NULL;
	}
	return NULL;
}


UINT32 serialize_kernel_params(BYTE ** lppOutData,int argc, ...)
{
	BYTE * params = NULL;
	BYTE * p = NULL;
	UINT32 param_size = 0;
	va_list ap;
	
scan_params:
	va_start(ap, argc);
	//calc param total size.
	for (int i = 0; i < argc; i++)
	{
		BYTE  type = va_arg(ap, BYTE);
		if (type == type_value){
			UINT32 value = va_arg(ap, UINT32);
			

			if (p){
				*p++ = type;
				memcpy(p, &value, sizeof(value));
				p += sizeof(value);
			}
			else{
				param_size += 5;
			}
		}
		else if (type == type_pointer){
			LPVOID lpData = va_arg(ap, LPVOID);
			UINT32 length = va_arg(ap, UINT32);

			if (p){
				*p++ = type;
				memcpy(p, &length, sizeof(length));
				p += sizeof(length);

				memcpy(p, lpData, length);
				p += length;
			}
			else{
				param_size += 5;
				param_size += length;
			}
			
		}
		else{
			return (~0);
		}
	}

	if (!params){
		//alloc param and scan again.
		params = (BYTE*)calloc(1,param_size);
		p = params;
		goto scan_params;
	}

	*lppOutData = params;
	return param_size;
}

void kernel_params_release(Params * lpParams)
{
	LPVOID * params = NULL;
	ParamInfo * param_info = NULL;

	if (lpParams)
	{
		params = (LPVOID*)(lpParams + 1);
		param_info = (ParamInfo*)(params + lpParams->num);

		for (int i = 0; i < lpParams->num; i++)
		{
			if (param_info[i].type == type_pointer &&
				param_info[i].flag & PARAM_FLAG_RELEASE)
			{
				free(params[i]);
			}
		}

		free(lpParams);
		lpParams = NULL;
	}

}

