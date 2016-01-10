#pragma once

#include "Emu/Audio/AudioThread.h"
#include "OpenAL/include/alext.h"

class OpenALThread : public AudioThread
{
private:
	static const uint g_al_buffers_count = 16;

	ALuint m_source;
	ALuint m_buffers[g_al_buffers_count];
	ALsizei m_buffer_size;

public:
	OpenALThread();
	virtual ~OpenALThread() override;

	virtual void Play() override;
	virtual void Open(const void* src, int size) override;
	virtual void Close() override;
	virtual void Stop() override;
	virtual void AddData(const void* src, int size) override;
};
