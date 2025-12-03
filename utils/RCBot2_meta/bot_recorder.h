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
    static CBotRecorder& GetInstance() {
        static CBotRecorder instance;
        return instance;
    }

    void RecordBotFrame(CBot* pBot) { (void)pBot; }
    bool IsRecording() const { return false; }
};

#endif
