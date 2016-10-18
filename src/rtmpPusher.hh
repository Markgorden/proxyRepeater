#ifndef RTMPPUSHER_HH_
#define RTMPPUSHER_HH_

#include <math.h>
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "srs_librtmp.h"

#define CHECK_ALIVE_TASK_TIMER_INTERVAL 5*1000*1000

#define RECONNECT_WAIT_DELAY(n) if (n >= 3) { usleep(CHECK_ALIVE_TASK_TIMER_INTERVAL); n = 0; }

Boolean isSPS(u_int8_t nut) { return nut == 7; } //Sequence parameter set
Boolean isPPS(u_int8_t nut) { return nut == 8; } //Picture parameter set
Boolean isIDR(u_int8_t nut) { return nut == 5; } //Coded slice of an IDR picture
Boolean isNonIDR(u_int8_t nut) { return nut == 1; } //Coded slice of a non-IDR picture
//Boolean isSEI(u_int8_t nut) { return nut == 6; } //Supplemental enhancement information
//Boolean isAUD(u_int8_t nut) { return nut == 9; } //Access unit delimiter

class StreamClientState {
public:
	StreamClientState();
	virtual ~StreamClientState();
public:
	MediaSession* session;
	MediaSubsessionIterator* iter;
	MediaSubsession* subsession;
	TaskToken streamTimerTask;
	TaskToken checkAliveTimerTask;
	double duration;
	struct timeval lastGettingFrameTime;
};

class ourRTMPClient: public Medium {
public:
	static ourRTMPClient* createNew(UsageEnvironment& env,
			RTSPClient* rtspClient);
protected:
	ourRTMPClient(UsageEnvironment& env, RTSPClient* rtspClient);
	virtual ~ourRTMPClient();
	Boolean isConnected();
public:
	Boolean sendH264FramePacket(u_int8_t* data, unsigned size, double pts);
	Boolean sendAACFramePacket(u_int8_t* data, unsigned size, double pts);
private:
	srs_rtmp_t rtmp;
	RTSPClient* fSource;
};

class ourRTSPClient: public RTSPClient {
public:
	static ourRTSPClient* createNew(UsageEnvironment& env, unsigned channelId);
protected:
	ourRTSPClient(UsageEnvironment& env, unsigned channelId);
	virtual ~ourRTSPClient();
public:
	StreamClientState scs;
	ourRTMPClient* publisher;
	unsigned id() const { return fChannelId; };
private:
	unsigned fChannelId;
};

class DummyRTPSink: public MediaSink {
public:
	static DummyRTPSink* createNew(UsageEnvironment& env,
			MediaSubsession& subsession, char const* streamId = NULL);
protected:
	DummyRTPSink(UsageEnvironment& env, MediaSubsession& subsession,
			char const* streamId);
	virtual ~DummyRTPSink();

	static void afterGettingFrame(void* clientData, unsigned frameSize,
			unsigned numTruncatedBytes, struct timeval presentationTime,
			unsigned durationInMicroseconds);

	void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
			struct timeval presentationTime, unsigned durationInMicroseconds);

	// redefined virtual functions:
	virtual Boolean continuePlaying();
public:
	void setBufferSize(unsigned size) { fBufferSize = size; \
										delete[] fReceiveBuffer; \
										fReceiveBuffer = new u_int8_t[size]; }

	//unsigned getBufferSize() const { return fBufferSize; }

	Boolean sendRawPacket(u_int8_t* data, unsigned size, double pts, Boolean isVideo = True) {
		if (pts < 0)
			return True;
		else {
			ourRTMPClient* rtmpClient = (ourRTMPClient*)((ourRTSPClient*) fSubsession.miscPtr)->publisher;
			if(isVideo)
				return rtmpClient->sendH264FramePacket(data, size, pts);
			else
				return rtmpClient->sendAACFramePacket(data, size, pts);
		}
	}

	Boolean sendSpsPacket(u_int8_t* data, unsigned size, double pts = -1.0) {
		if (fSps != NULL) {
			delete[] fSps; fSps = NULL;
		}
		fSpsSize = size + 4;
		fSps = new u_int8_t[fSpsSize];

		fSps[0] = 0;	fSps[1] = 0;
		fSps[2] = 0;	fSps[3] = 1;
		memmove(fSps + 4, data, size);
#ifdef DEBUG
		envir() << "["<< timestamp << "] ";
		for(unsigned i = 0; i < fSpsSize; i++) {
			envir() <<  fSps[i] << " ";
		}
		envir() << "\n";
#endif
		return sendRawPacket(fSps, fSpsSize, pts);
	}

	Boolean sendPpsPacket(u_int8_t* data, unsigned size, double pts = -1.0) {
		if (fPps != NULL) {
			delete[] fPps; fPps = NULL;
		}
		fPpsSize = size + 4;
		fPps = new u_int8_t[fPpsSize];

		fPps[0] = 0;	fPps[1] = 0;
		fPps[2] = 0;	fPps[3] = 1;
		memmove(fPps + 4, data, size);
#ifdef DEBUG
		envir() << "["<< timestamp << "] ";
		for(unsigned i = 0; i < fPpsSize; i++) {
			envir() <<  fPps[i] << " ";
		}
		envir() << "\n";
#endif
		return sendRawPacket(fPps, fPpsSize, pts);
	}
private:
	u_int8_t* fSps;
	u_int8_t* fPps;
	unsigned fSpsSize;
	unsigned fPpsSize;
	u_int8_t* fReceiveBuffer;
	unsigned fBufferSize;
	char* fStreamId;
	MediaSubsession& fSubsession;
	Boolean fHaveWrittenFirstFrame;
};

class DummyFileSink: public MediaSink {
public:
	static DummyFileSink* createNew(UsageEnvironment& env, char const* streamId = NULL);
protected:
	DummyFileSink(UsageEnvironment& env, char const* streamId);
	// called only by createNew()
	virtual ~DummyFileSink();
protected:
	// redefined virtual functions:
	virtual Boolean continuePlaying();

	static void afterGettingFrame(void* clientData, unsigned frameSize,
			unsigned numTruncatedBytes, struct timeval presentationTime,
			unsigned durationInMicroseconds);

	virtual void afterGettingFrame(unsigned frameSize,
			unsigned numTruncatedBytes, struct timeval presentationTime);
private:
	u_int8_t* fBuffer;
	char* fStreamId;
};
#endif /* RTMPPUSHER_HH_ */

typedef unsigned int UINT;
typedef unsigned char BYTE;
typedef unsigned long DWORD;

int h264_decode_sps(BYTE * buf, unsigned int nLen, unsigned &width, unsigned &height, unsigned &fps);

UINT Ue(BYTE *pBuff, UINT nLen, UINT &nStartBit) {
	UINT nZeroNum = 0;
	while (nStartBit < nLen * 8) {
		if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8)))
			break;
		nZeroNum++;
		nStartBit++;
	}
	nStartBit++;

	DWORD dwRet = 0;
	for (UINT i = 0; i < nZeroNum; i++) {
		dwRet <<= 1;
		if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8))) {
			dwRet += 1;
		}
		nStartBit++;
	}
	return (1 << nZeroNum) - 1 + dwRet;
}

int Se(BYTE *pBuff, UINT nLen, UINT &nStartBit) {
	int UeVal = Ue(pBuff, nLen, nStartBit);
	double k = UeVal;
	int nValue = ceil(k / 2);
	if (UeVal % 2 == 0)
		nValue = -nValue;
	return nValue;
}

DWORD u(UINT BitCount, BYTE * buf, UINT &nStartBit) {
	DWORD dwRet = 0;
	for (UINT i = 0; i < BitCount; i++) {
		dwRet <<= 1;
		if (buf[nStartBit / 8] & (0x80 >> (nStartBit % 8)))
			dwRet += 1;
		nStartBit++;
	}
	return dwRet;
}

void de_emulation_prevention(BYTE* buf, unsigned int* buf_size) {
	size_t i = 0, j = 0;
	BYTE* tmp_ptr = NULL;
	unsigned int tmp_buf_size = 0;
	int val = 0;
	tmp_ptr = buf;
	tmp_buf_size = *buf_size;
	for (i = 0; i < (tmp_buf_size - 2); i++) {
		val = (tmp_ptr[i] ^ 0x00) + (tmp_ptr[i + 1] ^ 0x00)
				+ (tmp_ptr[i + 2] ^ 0x03);
		if (val == 0) {
			for (j = i + 2; j < tmp_buf_size - 1; j++)
				tmp_ptr[j] = tmp_ptr[j + 1];
			(*buf_size)--;
		}
	}
}
