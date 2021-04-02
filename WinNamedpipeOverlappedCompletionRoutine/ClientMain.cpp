#include "Common.h"

typedef struct
{
   OVERLAPPED oOverlap;
   HANDLE hPipeInst;
   HANDLE hUserCmd;
   char chReceived[MSGSIZE];
   std::mutex sendBufferLock;
   char chToSend[MSGSIZE];
   bool quit;
} PIPEINST, *LPPIPEINST;

typedef struct
{
   OVERLAPPED oOverlap;
   char buffer[MSGSIZE];
} PIPEWRITE;

PIPEINST ClientPipe;


static VOID WINAPI CompletedReadRoutine(DWORD, DWORD, LPOVERLAPPED);
static VOID WINAPI CompletedWriteRoutine(DWORD, DWORD, LPOVERLAPPED);
static void UIThreadProc();


int ClientMain()
{
   HANDLE hClientPipe = NULL;
   while (1)
   {
      hClientPipe = CreateFile(
         lpszPipename,   // pipe name 
         GENERIC_READ |  // read and write access 
         GENERIC_WRITE,
         0,              // no sharing 
         NULL,           // default security attributes
         OPEN_EXISTING,  // opens existing pipe 
         FILE_FLAG_OVERLAPPED,           // default attributes 
         NULL);          // no template file 

   // Break if the pipe handle is valid. 

      if (hClientPipe != INVALID_HANDLE_VALUE)
         break;

      // Exit if an error other than ERROR_PIPE_BUSY occurs. 

      if (GetLastError() != ERROR_PIPE_BUSY)
      {
         printf("Could not open pipe. GLE=%d\n", GetLastError());
         return -1;
      }

      // All pipe instances are busy, so wait for 1 seconds. 

      if (!WaitNamedPipe(lpszPipename, 1000))
      {
         printf("Could not open pipe: 1 second wait timed out.");
         return -1;
      }

      //Sleep(1000);
   }

   // The pipe connected; change to message-read mode. 
   DWORD dwMode = PIPE_READMODE_MESSAGE;
   BOOL fSuccess = SetNamedPipeHandleState(
      hClientPipe,    // pipe handle 
      &dwMode,  // new pipe mode 
      NULL,     // don't set maximum bytes 
      NULL);    // don't set maximum time 
   if (!fSuccess)
   {
      printf("SetNamedPipeHandleState failed. GLE=%d\n", GetLastError());
      return -1;
   }

   // DUMMY
   ClientPipe.hUserCmd = CreateEvent(
      NULL,    // default security attribute
      FALSE,    // manual reset event 
      FALSE,    // initial state = signaled 
      NULL);   // unnamed event object 
   if (ClientPipe.hUserCmd == NULL) {
      std::cout << "CreateEvent failed." << std::endl;
      return 0;
   }

   std::thread UIThread(UIThreadProc);

   ClientPipe.hPipeInst = hClientPipe;
   CompletedReadRoutine(0, 0, (LPOVERLAPPED)&ClientPipe);

   while (1) {
      if (ClientPipe.quit) {
         break;
      }
      DWORD dwWait = WaitForSingleObjectEx(
         ClientPipe.hUserCmd,  // event object to wait for 
         INFINITE,       // waits indefinitely 
         TRUE);          // alertable wait enabled 

      switch (dwWait)
      {
      case 0:
         if (ClientPipe.quit) {
            break;
         } else {
            PIPEWRITE * wovlp = new PIPEWRITE();
            {
               std::lock_guard<std::mutex> lock(ClientPipe.sendBufferLock);
               memcpy(wovlp->buffer, ClientPipe.chToSend, MSGSIZE);
            }
            BOOL fWrite = WriteFileEx(
               ClientPipe.hPipeInst,
               wovlp->buffer,
               MSGSIZE,
               (LPOVERLAPPED)wovlp,
               (LPOVERLAPPED_COMPLETION_ROUTINE)CompletedWriteRoutine);
            if (fWrite == 0) {
               printf("WriteFileEx failed. GLE=%d\n", GetLastError());
               ClientPipe.quit = true;
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
   CloseHandle(ClientPipe.hPipeInst);
   CloseHandle(ClientPipe.hUserCmd);
   return 0;
}


static VOID WINAPI CompletedReadRoutine(DWORD dwErr, DWORD cbBytesRead,
   LPOVERLAPPED lpOverLap)
{
   LPPIPEINST lpPipeInst;
   BOOL fRead = FALSE;

   // lpOverlap points to storage for this instance. 

   lpPipeInst = (LPPIPEINST)lpOverLap;

   // The read operation has finished, so write a response (if no 
   // error occurred). 

   if ((dwErr == 0))
   {
      // TODO. consume the message.
      if (cbBytesRead != 0) {
         std::cout << "Received a message: " << lpPipeInst->chReceived << std::endl;
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
         ClientPipe.quit = true;
      }
   } else {
      printf("dwErr=%d\n", dwErr);
      ClientPipe.quit = true;
   }
}

static VOID WINAPI CompletedWriteRoutine(DWORD dwErr, DWORD cbBytesRead,
   LPOVERLAPPED lpOverLap)
{
   PIPEWRITE * lpPipeInst = (PIPEWRITE *)lpOverLap;
   delete lpPipeInst;
}


void UIThreadProc()
{
   std::cout << "Let's play (C)." << std::endl;
   int i = 0;
   while (!ClientPipe.quit) {
      //std::string buffer;
      //std::cin >> buffer;
      //if (buffer == "q") {
      //   ClientPipe.quit = true;
      //} else {

      char buffer[1024];
      sprintf_s(buffer, "%d: %d", GetCurrentProcessId(), i++);

      {
         std::lock_guard<std::mutex> lock(ClientPipe.sendBufferLock);
         strncpy_s(ClientPipe.chToSend, buffer, MSGSIZE - 1);
      }

      SetEvent(ClientPipe.hUserCmd);
      Sleep(3000);
   }
}
