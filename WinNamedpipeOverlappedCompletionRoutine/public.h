#pragma once
#include <Windows.h>

constexpr int MSGSIZE = 10240;
constexpr int PIPE_TIMEOUT_MS = 5000;

constexpr TCHAR lpszPipename[] = TEXT("\\\\.\\pipe\\nestedbridgemon");

constexpr char CMD_PROCESS[] = "PROCESS";
constexpr char CMD_CONNS[] = "CONNS";
constexpr char CMD_SEP[] = "::";

constexpr char CMD_PROCESS_WITH_SEP[] = "PROCESS::";
constexpr char CMD_CONNS_WITH_SEP[] = "CONNS::";

constexpr const char * DISCONNECT_CMD = "Disconnect";           // Shutdown the feature, so all channels should be going to close.
constexpr const char * CONNECT_CMD = "Connect";                 // ReStart the feature.
//constexpr const char * CLOSECHANNEL_CMD = "CloseChannel";   // Close the requested channel.
//constexpr const char * OPENCHANNELS_CMD = "OpenChannels";     // Open the feature's channels.

constexpr const char * VALID_COMMANDS[] = {
   DISCONNECT_CMD,
   CONNECT_CMD,
   //CLOSECHANNEL_CMD,
   //OPENCHANNELS_CMD,
};
