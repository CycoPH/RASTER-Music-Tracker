#pragma once

/// <summary>
/// Helper class to record the 9 (mono) or 18 (stereo) Pokey registers
/// into a large memory buffer.
/// This happens during quick-play.
/// These values can then be exported by various paths: SAP-R, with LZSS etc
/// </summary>
class CPokeyStream
{
public:
	typedef enum
	{
		STOP = 0,
		RECORD = 1,
		WRITE = 2,
		START = 3,
	} STREAM_STATE;

	CPokeyStream();
	~CPokeyStream();

	void Clear();

	void StartRecording();

	inline unsigned char* GetStreamBuffer(void) { return m_StreamBuffer; }
	inline int GetFirstCountPoint(void) { return m_FirstCountPoint; }
	inline int GetSecondCountPoint(void) { return m_SecondCountPoint; }
	inline int GetThirdCountPoint(void) { return m_ThirdCountPoint; }

	inline bool IsRecording() { return m_recordState != STREAM_STATE::STOP; }
	inline bool IsWriting() { return m_recordState == STREAM_STATE::WRITE; }
	inline void SetState(STREAM_STATE newState) { m_recordState = newState; }
	inline int LoopCount() { return m_SongLoppedCounter; }

	int SwitchIntoRecording();
	int SwitchIntoStop();
	void CallFromPlay(int playerState, int trackLine, int songLine);
	bool TrackSongLine(int songLine);
	bool CallFromPlayBeat(int trackLine);

	void Record(void);
	void WriteToFile(std::ofstream& ou, int frames, int offset);
	void FinishedRecording();

private:
	STREAM_STATE m_recordState;		// What state is the recorder in?

	unsigned char* m_StreamBuffer;	// Ptr to the buffer to hold the Pokey values

	int m_FrameCounter;				// How many Pokey frames have been recorded?

	int m_SongLoppedCounter;		// How may times has the song been looped?

	int m_PlayCount[256];			// Keeping track of loop points

	int m_FirstCountPoint;			// How many frames until we hit the first loop point
	int m_SecondCountPoint;			// How many frames until we hit the second loop point
	int m_ThirdCountPoint;

	int m_BufferSize;				// What size if the m_StreamBuffer currently
};

