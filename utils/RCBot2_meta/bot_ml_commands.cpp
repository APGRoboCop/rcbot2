// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
/*
 *    This file is part of RCBot.
 *
 *    Machine Learning commands for RCBot2
 *
 *    RCBot by Paul Murphy adapted from Botman's HPB Bot 2 template.
 *
 *    RCBot is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published by the
 *    Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *
 *    RCBot is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with RCBot; if not, write to the Free Software Foundation,
 *    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "bot_commands.h"
#include "bot_recorder.h"
#include "bot_globals.h"
#include "bot.h"

// ML Recording Commands

CBotCommandInline MLRecordStartCommand("ml_record_start", CMD_ACCESS_BOT | CMD_ACCESS_DEDICATED,
    [](const CClient* pClient, const BotCommandArgs& args)
    {
        edict_t* pEntity = nullptr;
        if (pClient)
            pEntity = pClient->getPlayer();

        CBotRecorder::GetInstance()->StartRecording();
        CBotGlobals::botMessage(pEntity, 0, "[ML] Recording started");
        return COMMAND_ACCESSED;
    });

CBotCommandInline MLRecordStopCommand("ml_record_stop", CMD_ACCESS_BOT | CMD_ACCESS_DEDICATED,
    [](const CClient* pClient, const BotCommandArgs& args)
    {
        edict_t* pEntity = nullptr;
        if (pClient)
            pEntity = pClient->getPlayer();

        CBotRecorder::GetInstance()->StopRecording();
        int frames = CBotRecorder::GetInstance()->GetFrameCount();
        float duration = CBotRecorder::GetInstance()->GetRecordingDuration();

        CBotGlobals::botMessage(pEntity, 0, "[ML] Recording stopped. Frames: %d, Duration: %.1fs",
            frames, duration);
        return COMMAND_ACCESSED;
    });

CBotCommandInline MLRecordPauseCommand("ml_record_pause", CMD_ACCESS_BOT | CMD_ACCESS_DEDICATED,
    [](const CClient* pClient, const BotCommandArgs& args)
    {
        edict_t* pEntity = nullptr;
        if (pClient)
            pEntity = pClient->getPlayer();

        CBotRecorder::GetInstance()->PauseRecording();
        CBotGlobals::botMessage(pEntity, 0, "[ML] Recording paused");
        return COMMAND_ACCESSED;
    });

CBotCommandInline MLRecordResumeCommand("ml_record_resume", CMD_ACCESS_BOT | CMD_ACCESS_DEDICATED,
    [](const CClient* pClient, const BotCommandArgs& args)
    {
        edict_t* pEntity = nullptr;
        if (pClient)
            pEntity = pClient->getPlayer();

        CBotRecorder::GetInstance()->ResumeRecording();
        CBotGlobals::botMessage(pEntity, 0, "[ML] Recording resumed");
        return COMMAND_ACCESSED;
    });

CBotCommandInline MLRecordSaveCommand("ml_record_save", CMD_ACCESS_BOT | CMD_ACCESS_DEDICATED,
    [](const CClient* pClient, const BotCommandArgs& args)
    {
        edict_t* pEntity = nullptr;
        if (pClient)
            pEntity = pClient->getPlayer();

        if (!args[0] || !*args[0]) {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Usage: rcbot ml_record_save <filename>");
            return COMMAND_ERROR;
        }

        char filepath[512];
        snprintf(filepath, sizeof(filepath), "rcbot2/data/ml/%s.rcbr", args[0]);

        if (CBotRecorder::GetInstance()->SaveRecording(filepath)) {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Recording saved to %s", filepath);
            return COMMAND_ACCESSED;
        } else {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Failed to save recording");
            return COMMAND_ERROR;
        }
    });

CBotCommandInline MLRecordExportJSONCommand("ml_record_export_json", CMD_ACCESS_BOT | CMD_ACCESS_DEDICATED,
    [](const CClient* pClient, const BotCommandArgs& args)
    {
        edict_t* pEntity = nullptr;
        if (pClient)
            pEntity = pClient->getPlayer();

        if (!args[0] || !*args[0]) {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Usage: rcbot ml_record_export_json <filename>");
            return COMMAND_ERROR;
        }

        char filepath[512];
        snprintf(filepath, sizeof(filepath), "rcbot2/data/ml/%s.json", args[0]);

        if (CBotRecorder::GetInstance()->ExportToJSON(filepath)) {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Recording exported to JSON: %s", filepath);
            return COMMAND_ACCESSED;
        } else {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Failed to export recording");
            return COMMAND_ERROR;
        }
    });

CBotCommandInline MLRecordExportCSVCommand("ml_record_export_csv", CMD_ACCESS_BOT | CMD_ACCESS_DEDICATED,
    [](const CClient* pClient, const BotCommandArgs& args)
    {
        edict_t* pEntity = nullptr;
        if (pClient)
            pEntity = pClient->getPlayer();

        if (!args[0] || !*args[0]) {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Usage: rcbot ml_record_export_csv <filename>");
            return COMMAND_ERROR;
        }

        char filepath[512];
        snprintf(filepath, sizeof(filepath), "rcbot2/data/ml/%s.csv", args[0]);

        if (CBotRecorder::GetInstance()->ExportToCSV(filepath)) {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Recording exported to CSV: %s", filepath);
            return COMMAND_ACCESSED;
        } else {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Failed to export recording");
            return COMMAND_ERROR;
        }
    });

CBotCommandInline MLRecordStatusCommand("ml_record_status", CMD_ACCESS_BOT | CMD_ACCESS_DEDICATED,
    [](const CClient* pClient, const BotCommandArgs& args)
    {
        edict_t* pEntity = nullptr;
        if (pClient)
            pEntity = pClient->getPlayer();

        CBotRecorder* recorder = CBotRecorder::GetInstance();

        if (recorder->IsRecording()) {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Recording: YES (frames: %d, duration: %.1fs)",
                recorder->GetFrameCount(),
                recorder->GetRecordingDuration());
        } else if (recorder->IsPaused()) {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Recording: PAUSED (frames: %d)",
                recorder->GetFrameCount());
        } else {
            CBotGlobals::botMessage(pEntity, 0, "[ML] Recording: NO (frames in buffer: %d)",
                recorder->GetFrameCount());
        }

        return COMMAND_ACCESSED;
    });

CBotCommandInline MLRecordClearCommand("ml_record_clear", CMD_ACCESS_BOT | CMD_ACCESS_DEDICATED,
    [](const CClient* pClient, const BotCommandArgs& args)
    {
        edict_t* pEntity = nullptr;
        if (pClient)
            pEntity = pClient->getPlayer();

        CBotRecorder::GetInstance()->ClearRecording();
        CBotGlobals::botMessage(pEntity, 0, "[ML] Recording buffer cleared");
        return COMMAND_ACCESSED;
    });
