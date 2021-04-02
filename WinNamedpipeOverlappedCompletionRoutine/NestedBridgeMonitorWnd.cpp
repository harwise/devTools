// ConsoleApplication3.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "NestedBridgeMonitorWnd.h"
#include <vector>
#include <cctype>
#include <string>

void CMainDialog::UpdateConnectionInfo(int id, char * info)
{
   if (IsWindow()) {
      char * content = nullptr;
      if (info != nullptr) {
         content = new char[strlen(info) + 1];
         memcpy(content, info, strlen(info) + 1);
      }
      PostMessage(WM_UPDATE_SHOWBOARD, (WPARAM)id, (LPARAM)content);
   }
}


LRESULT CMainDialog::OnShowBoard(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
   int id = (int)wParam;
   char * content = (char *)lParam;

   if (content != nullptr) {
      auto it = m_connections.emplace(id, ConnectionGroup{});
      if (0 == strncmp(content, CMD_PROCESS_WITH_SEP, sizeof(CMD_PROCESS_WITH_SEP) - 1)) {
         it.first->second.groupInfo = content + sizeof(CMD_PROCESS_WITH_SEP) - 1;
      } else if (0 == strncmp(content, CMD_CONNS_WITH_SEP, sizeof(CMD_CONNS_WITH_SEP) - 1)) {
         it.first->second.connections = content + sizeof(CMD_CONNS_WITH_SEP) - 1;
      } else {
         printf("ERROR: Unknown msg: %s\n", content);
      }
      delete[] content;
   } else {
      m_connections.erase(id);
   }

   std::string strToShow = FormatConnectionString();

   size_t charCount = strToShow.size() + 1;
   wchar_t * editorText = new wchar_t[charCount];
   memset(editorText, 0x00, sizeof(wchar_t) * charCount);
   ::MultiByteToWideChar(CP_ACP, 0, strToShow.c_str(), (int)strToShow.size(), editorText, (int)charCount);

   auto showBoard = GetDlgItem(IDC_CONNECTIONS);
   showBoard.SetWindowText(editorText);

   delete[] editorText;
   return 0;
}


std::string CMainDialog::FormatConnectionString()
{
   std::string ret;
   for (auto & c : m_connections) {
      char header[MAX_PATH * 2] = "";
      sprintf_s(header, "[id=%d] %s", c.first, c.second.groupInfo.c_str());
      std::string headerSep(strlen(header), '_');
      ret.append(headerSep); ret.append("\r\n");
      ret.append(header); ret.append("\r\n");
      ret.append(headerSep); ret.append("\r\n");

      std::vector<std::string> lines;
      size_t lineLen = 0;

      std::string & s = c.second.connections;
      size_t s0 = 0;
      size_t s1 = s.find("\r\n");
      while (s1 != s.npos) {
         if (s1 != s0) {
            lines.push_back(s.substr(s0, s1 - s0));
            if (s1 - s0 > lineLen) {
               lineLen = s1 - s0;
            }
         }
         s0 = s1 + 2;
         s1 = s.find("\r\n", s0);
      }

      std::string tableHeader;
      tableHeader.append("Agent ");
      tableHeader.append(std::string(lineLen, ' '));
      tableHeader.append("Client");
      ret.append(tableHeader); ret.append("\r\n");

      lineLen += 12;
      std::string tableHeaderSep(lineLen, '-');

      for (auto & line : lines) {
         size_t sep = line.find("\t");
         std::string conn = line.substr(0, sep);
         size_t fillCount = lineLen - line.size() + 1;  // '\t'
         conn.append(std::string(fillCount, '-'));
         conn.append(line.substr(sep + 1));
         ret.append(conn); ret.append("\r\n");
      }

      ret.append("\r\n");
   }

   return ret;
}


LRESULT CMainDialog::OnSize(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
   int w = LOWORD(lParam);
   int h = HIWORD(lParam);

   RECT rcClient;
   GetClientRect(&rcClient);

   RECT rc;
   rc.left = m_borderWidth;
   rc.top = m_borderWidth;
   rc.right = rcClient.right - m_borderWidth;
   rc.bottom = rcClient.bottom - m_boardBottom;
   m_boardEditBox.SetWindowPos(NULL, &rc, 0);

   rc.left = m_borderWidth;
   rc.right = rcClient.right - m_borderWidth;
   rc.bottom = rcClient.bottom - m_cmdBottom;
   rc.top = rc.bottom - m_cmdHeight;
   m_cmdEditBox.SetWindowPos(NULL, &rc, 0);

   return 0;
}


LRESULT
CCmdEditBox::OnKeyDown(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
   if (wParam == VK_RETURN)
   {
      wchar_t cmdW[1024] = L"";
      GetWindowText(cmdW, 1024);
      if (cmdW[0] != L'\0') {
         char cmd[2048] = "";
         ::WideCharToMultiByte(CP_ACP, 0, cmdW, 1024, cmd, 2048, NULL, NULL);
         printf("--->the command is %s\n", cmd);
         SetWindowText(L"");

         GetParent().SendMessage(WM_USER_COMMAND, (WPARAM)cmd);
      }
      bHandled = TRUE;
      return 0;
   }
   bHandled = FALSE;
   return 0; // all other cases still need default processing
}


LRESULT
CMainDialog::OnUserCommand(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
   /*
    * Command clientid [param0, ... ]
    */

   std::string cmd = (const char *)wParam;
   size_t pos = std::string::npos;
   int clientid = -1;
   for (auto & validCmd : VALID_COMMANDS) {
      if (cmd.find(validCmd) == 0) {
         pos = strlen(validCmd);
         break;
      }
   }

   if (pos == std::string::npos) {
      printf("--->the command is not supported\n");
      return 0;
   }

   if (!std::isspace(cmd[pos])) {
      printf("--->the command is not complete\n");
      return 0;
   }

   size_t clientIdStart = pos;
   while (clientIdStart < cmd.size()) {
      if (!std::isspace(cmd[clientIdStart])) {
         break;
      }
      clientIdStart++;
   }

   if (!std::isdigit(cmd[clientIdStart])) {
      printf("--->can not find the clientid head\n");
      return 0;
   }

   size_t clientIdFinish = clientIdStart;
   while (clientIdFinish < cmd.size()) {
      if (!std::isdigit(cmd[clientIdFinish])) {
         break;
      }
      clientIdFinish++;
   }

   if (clientIdFinish != cmd.size() && !std::isspace(cmd[clientIdFinish])) {
      printf("--->can not find the clientid tail\n");
      return 0;
   }

   int clientId = std::stoi(&cmd[clientIdStart]);
   cmd.erase(pos, clientIdFinish - pos);
   SendCmdToCient(clientId, cmd.c_str());

   return 0;
}
