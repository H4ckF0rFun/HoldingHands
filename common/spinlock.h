#ifndef _SPIN_LOCK
#define _SPIN_LOCK
#include<Windows.h>

#define __spin_lock_initialize(lock) InterlockedExchange(&lock,0)
#define __spin_lock(lock)  while (InterlockedExchange(&lock,1)){__asm{pause};};
#define __spin_unlock(lock) InterlockedExchange(&lock,0)


#define RW_LOCK_BIAS		 0x01000000

typedef volatile unsigned long rw_spinlock_t;
typedef volatile unsigned long spinlock_t;

#define _rw_spinlock_init(x)   do{ *x = RW_LOCK_BIAS; } while(0);

//copied from linux-2.6.11;

__declspec(naked) static int _read_lock(rw_spinlock_t *rw_lock)
{
	__asm
	{
		mov eax, [esp + 0x4];
		lock dec dword ptr[eax];
		jns _read_locked;
		call _read_lock_failed;

	_read_lock_failed:
		lock inc dword ptr[eax];			//release lock.
	_1:
		pause;
		cmp dword ptr[eax], 1;
		js _1;

		//[eax] >= 1 
		lock dec dword ptr[eax];
		js _read_lock_failed;
		ret;

	_read_locked:
		ret;
	}
}

__declspec(naked) static int _write_lock(rw_spinlock_t *rw_lock)
{
	__asm
	{
		mov eax, [esp + 0x4];
	_try_w_lock:
		lock sub dword ptr[eax], RW_LOCK_BIAS;
		jz _w_locked;
		pause;
		lock add dword ptr[eax], RW_LOCK_BIAS;
		jmp _try_w_lock
		_w_locked :
		ret
	}
}

__declspec(naked) static int _read_unlock(rw_spinlock_t *rw_lock)
{
	__asm
	{
		mov eax, [esp + 0x4];
		lock inc dword ptr[eax];
		ret;
	}
}


__declspec(naked) static int _write_unlock(rw_spinlock_t *rw_lock)
{
	__asm
	{
		mov eax, [esp + 0x4];
		lock add dword ptr[eax], RW_LOCK_BIAS;
		ret;
	}
}

#endif