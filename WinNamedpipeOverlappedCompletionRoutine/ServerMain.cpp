// ConsoleApplication3.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "Common.h"
#include <vector>
#include <map>
#include <atomic>

#include "NestedBridgeMonitorWnd.h"

typedef struct
{
   OVERLAPPED oOverlap;
   HANDLE hPipeInst;
   int id;
   char chReceived[MSGSIZE];
} PIPEINST, *LPPIPEINST;

typedef struct
{
   OVERLAPPED oOverlap;
   char buffer[MSGSIZE];
} PIPEWRITE;

typedef struct
{
   int id;
   std::string cmd;
} SENDCMD;

int gNextPipeId = 0;
std::map<int, LPPIPEINST> gPipeConnections;

std::mutex gCmdMutex;
std::vector<SENDCMD> gCmdQueue;
HANDLE gCmdEvent;
std::atomic<bool> gQuit;

CMainDialog gDialog;

VOID DisconnectAndClose(LPPIPEINST);
BOOL CreateAndConnectInstance(LPOVERLAPPED);

static VOID WINAPI CompletedWriteRoutine(DWORD, DWORD, LPOVERLAPPED);
static VOID WINAPI CompletedReadRoutine(DWORD, DWORD, LPOVERLAPPED);
static void UIThreadProc();


HANDLE hServerPipe;
int ServerMain()
{
   gCmdEvent = CreateEvent(
      NULL,    // default security attribute
      FALSE,    // manual reset event 
      FALSE,    // initial state = signaled 
      NULL);   // unnamed 
   if (gCmdEvent == NULL) {
      std::cout << "CreateEvent failed." << std::endl;
      return 0;
   }

   HANDLE hConnectEvent = CreateEvent(
      NULL,    // default security attribute
      FALSE,    // manual reset event 
      FALSE,    // initial state = signaled 
      NULL);   // unnamed event object 
   if (hConnectEvent == NULL) {
      std::cout << "CreateEvent failed." << std::endl;
      return 0;
   }

   std::thread UIThread(UIThreadProc);

   HANDLE waitedSignals[2] = { hConnectEvent, gCmdEvent };

   OVERLAPPED oConnect;
   oConnect.hEvent = hConnectEvent;

   BOOL fPendingIO = CreateAndConnectInstance(&oConnect);
   while (!gQuit) {
      DWORD dwWait = WaitForMultipleObjectsEx(
         2,
         waitedSignals,  // event object to wait for 
         FALSE,
         INFINITE,       // waits indefinitely 
         TRUE);          // alertable wait enabled 

      switch (dwWait)
      {
      case 0: {
         std::cout << "Connected a instance." << std::endl;

         if (fPendingIO)
         {
            DWORD cbRet;
            BOOL fSuccess = GetOverlappedResult(
               hServerPipe,     // pipe handle 
               &oConnect, // OVERLAPPED structure 
               &cbRet,    // bytes transferred 
               FALSE);    // does not wait 
            if (!fSuccess)
            {
               printf("ConnectNamedPipe (%d)\n", GetLastError());
               return 0;
            }
         }

         LPPIPEINST lpPipeInst = new PIPEINST{};
         lpPipeInst->hPipeInst = hServerPipe;
         lpPipeInst->id = gNextPipeId++;
         gPipeConnections[lpPipeInst->id] = lpPipeInst;

         CompletedReadRoutine(0, 0, (LPOVERLAPPED)lpPipeInst);

         fPendingIO = CreateAndConnectInstance(&oConnect);
         break;
      }

      case 1: {
         if (gQuit) {
            break;
         }

         std::vector<SENDCMD> cmds;
         {
            std::lock_guard<std::mutex> lock(gCmdMutex);
            std::swap(cmds, gCmdQueue);
         }

         for (auto & cmd : cmds) {
            auto it = gPipeConnections.find(cmd.id);
            if (it != gPipeConnections.end()) {
               PIPEWRITE * wovlp = new PIPEWRITE();
               memcpy(wovlp->buffer, cmd.cmd.c_str(), cmd.cmd.size());
               BOOL fWrite = WriteFileEx(
                  it->second->hPipeInst,
                  wovlp->buffer,
                  MSGSIZE,
                  (LPOVERLAPPED)wovlp,
                  (LPOVERLAPPED_COMPLETION_ROUTINE)CompletedWriteRoutine);
               if (fWrite == 0) {
                  printf("WriteFileEx failed. GLE=%d\n", GetLastError());
                  DisconnectAndClose(it->second);
               }
            } else {
               printf("Failed to write the cmd (%s) for invalid pipe id (%d)\n", cmd.cmd.c_str(), cmd.id);
            }
         }

         break;
      }

      case WAIT_IO_COMPLETION:
         break;

      default:
         printf("WaitForSingleObjectEx (%d)\n", GetLastError());
         return 0;
      }
   }

   UIThread.join();
   CloseHandle(hConnectEvent);
   CloseHandle(gCmdEvent);
   return 0;
}


BOOL CreateAndConnectInstance(LPOVERLAPPED lpoOverlap)
{
   hServerPipe = CreateNamedPipe(
      lpszPipename,             // pipe name 
      PIPE_ACCESS_DUPLEX |      // read/write access 
      FILE_FLAG_OVERLAPPED,     // overlapped mode 
      PIPE_TYPE_MESSAGE |       // message-type pipe 
      PIPE_READMODE_MESSAGE |   // message read mode 
      PIPE_WAIT,                // blocking mode 
      PIPE_UNLIMITED_INSTANCES, // unlimited instances 
      MSGSIZE,    // output buffer size 
      MSGSIZE,    // input buffer size 
      PIPE_TIMEOUT_MS,             // client time-out 
      NULL);                    // default security attributes
   if (hServerPipe == INVALID_HANDLE_VALUE)
   {
      printf("CreateNamedPipe failed with %d.\n", GetLastError());
      return 0;
   }

   BOOL fConnected = ConnectNamedPipe(hServerPipe, lpoOverlap);
   if (fConnected)
   {
      printf("ConnectNamedPipe failed with %d.\n", GetLastError());
      return 0;
   }

   BOOL fPendingIO = FALSE;
   switch (GetLastError())
   {
      // The overlapped connection in progress. 
   case ERROR_IO_PENDING:
      fPendingIO = TRUE;
      break;

      // Client is already connected, so signal an event. 
   case ERROR_PIPE_CONNECTED:
      if (SetEvent(lpoOverlap->hEvent))
         break;

      // If an error occurs during the connect operation... 
   default:
      printf("ConnectNamedPipe failed with %d.\n", GetLastError());
      return 0;
   }
   return fPendingIO;
}


static VOID WINAPI CompletedReadRoutine(DWORD dwErr, DWORD cbBytesRead,
   LPOVERLAPPED lpOverLap)
{
   printf("CompletedReadRoutine(%p)\n", lpOverLap);

   LPPIPEINST lpPipeInst;
   BOOL fRead = FALSE;

   // lpOverlap points to storage for this instance. 

   lpPipeInst = (LPPIPEINST)lpOverLap;

   // The read operation has finished, so write a response (if no 
   // error occurred). 

   if (dwErr == 0)
   {
      if (cbBytesRead != 0) {
         std::cout << "Received a message: " << lpPipeInst->chReceived << std::endl;
         gDialog.UpdateConnectionInfo(lpPipeInst->id, lpPipeInst->chReceived);
      }

      fRead = ReadFileEx(
         lpPipeInst->hPipeInst,
         lpPipeInst->chReceived,
         MSGSIZE,
         (LPOVERLAPPED)lpPipeInst,
         (LPOVERLAPPED_COMPLETION_ROUTINE)CompletedReadRoutine);

      // Disconnect if an error occurred. 
      if (!fRead) {
         printf("ReadFileEx failed. GLE=%d\n", GetLastError());
         DisconnectAndClose(lpPipeInst);
      }
   } else {
      printf("dwErr=%d\n", dwErr);
      DisconnectAndClose(lpPipeInst);
   }
}


VOID DisconnectAndClose(LPPIPEINST lpPipeInst)
{
   // Disconnect the pipe instance. 

   std::cout << "DISCONNECT & CLOSE" << std::endl << std::endl;

   gDialog.UpdateConnectionInfo(lpPipeInst->id, nullptr);

   if (!DisconnectNamedPipe(lpPipeInst->hPipeInst))
   {
      printf("DisconnectNamedPipe failed with %d.\n", GetLastError());
   }

   // Close the handle to the pipe instance. 

   printf("CloseHandle(%p)\n", lpPipeInst->hPipeInst);
   CloseHandle(lpPipeInst->hPipeInst);

   // Release the storage for the pipe instance. 

   printf("free(%p)\n", lpPipeInst);
   if (lpPipeInst != NULL) {
      gPipeConnections.erase(lpPipeInst->id);
      delete lpPipeInst;
   }
}


static VOID WINAPI CompletedWriteRoutine(DWORD dwErr, DWORD cbBytesRead,
   LPOVERLAPPED lpOverLap)
{
   PIPEWRITE * lpPipeInst = (PIPEWRITE *)lpOverLap;
   delete lpPipeInst;
}


static void UIThreadProc()
{
   std::cout << "Let's play (S)." << std::endl;

   gDialog.Create(NULL);
   gDialog.ShowWindow(SW_SHOW);

   MSG uMsg;
   while (GetMessage(&uMsg, NULL, 0, 0))
   {
      TranslateMessage(&uMsg);
      DispatchMessage(&uMsg);
   }

   gDialog.DestroyWindow();
   gQuit = true;
   SetEvent(gCmdEvent);
}


void
SendCmdToCient(int clientId, const char * cmd)
{
   printf("--->id: %d; cmd: %s\n", clientId, cmd);
   std::lock_guard<std::mutex> lock(gCmdMutex);
   gCmdQueue.push_back({clientId, std::string(cmd)});
   SetEvent(gCmdEvent);
}
