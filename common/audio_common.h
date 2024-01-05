#ifndef ADUIO_COMMON_H
#define ADUIO_COMMON_H

#define AUDIO		('A'|'U'<<8|'D'<<16|'O'<<24)


enum AUDIO_EVENT
{
	AUDIO_ERROR,
	AUDIO_BEGIN,
	AUDIO_STOP,

	AUDIO_PLAY_BEGIN,
	AUDIO_PLAY_DATA,
	AUDIO_PLAY_STOP
};

#endif