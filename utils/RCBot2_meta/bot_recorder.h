/*
 *    This file is part of RCBot.
 */

#ifndef __BOT_RECORDER_H__
#define __BOT_RECORDER_H__

// Stub header - bot_recorder temporarily disabled

class CBot;

// Stub recorder class
class CBotRecorder
{
public:
    static CBotRecorder* GetInstance() {
        static CBotRecorder instance;
        return &instance;
    }

    void RecordBotFrame(CBot* pBot) { (void)pBot; }
    void RecordFrame(CBot* pBot) { (void)pBot; }
    bool IsRecording() const { return false; }
    void StartRecording() {}
    void StopRecording() {}
    int GetFrameCount() const { return 0; }
    float GetRecordingDuration() const { return 0.0f; }
    void PauseRecording() {}
    void ResumeRecording() {}
    bool SaveRecording(const char* filename) { (void)filename; return false; }
    bool ExportToJSON(const char* filename) { (void)filename; return false; }
    bool ExportToCSV(const char* filename) { (void)filename; return false; }
    bool IsPaused() const { return false; }
    void ClearRecording() {}
};

#endif
