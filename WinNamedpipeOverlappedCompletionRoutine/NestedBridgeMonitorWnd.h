// ConsoleApplication3.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "Common.h"
#include <map>
#include <atlbase.h>
#include <atlwin.h>
#include <atlctl.h>

#include "resource.h"


#define WM_UPDATE_SHOWBOARD (WM_USER + 1000)
#define WM_USER_COMMAND (WM_USER + 1001)


class CCmdEditBox : public CWindowImpl<CCmdEditBox>
{
public:
   BEGIN_MSG_MAP(CCmdEditBox)
      MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
   END_MSG_MAP()

   LRESULT OnKeyDown(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};


class CMainDialog :
   public CDialogImpl<CMainDialog>
{
public:
   enum { IDD = IDD_DIALOG1 };

   BEGIN_MSG_MAP(CMainDialog)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      MESSAGE_HANDLER(WM_UPDATE_SHOWBOARD, OnShowBoard)
      MESSAGE_HANDLER(WM_USER_COMMAND, OnUserCommand)
      MESSAGE_HANDLER(WM_SIZE, OnSize)
      COMMAND_ID_HANDLER(IDCANCEL, OnCommand)
      COMMAND_ID_HANDLER(IDOK, OnCommand)
   END_MSG_MAP()

public:
   // CMainDialog

   // Window Message Handlers
   LRESULT OnInitDialog(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
   {
      m_cmdEditBox.SubclassWindow(GetDlgItem(IDC_COMMAND));
      m_boardEditBox.Attach(GetDlgItem(IDC_CONNECTIONS));

      RECT rcClient, rcBoard, rcCmd;
      GetWindowRect(&rcClient);
      m_cmdEditBox.GetWindowRect(&rcCmd);
      m_boardEditBox.GetWindowRect(&rcBoard);
      m_borderWidth = rcBoard.left - rcClient.left;
      m_boardBottom = rcClient.bottom - rcBoard.bottom;
      m_cmdBottom = rcClient.bottom - rcCmd.bottom;
      m_cmdHeight = rcCmd.bottom - rcCmd.top;

      ATLVERIFY(CenterWindow());
      return 0;
   }
   LRESULT OnCommand(UINT cmd, INT nIdentifier, HWND hWnd, BOOL& bHandled)
   {
      PostQuitMessage(0);
      return 0;
   }

   void UpdateConnectionInfo(int id, char * info);
   LRESULT OnShowBoard(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnUserCommand(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSize(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

private:
   std::string FormatConnectionString();

private:
   struct ConnectionGroup
   {
      std::string groupInfo;
      std::string connections;
   };

   CCmdEditBox m_cmdEditBox;
   CWindow m_boardEditBox;
   int m_borderWidth;
   int m_boardBottom;
   int m_cmdBottom;
   int m_cmdHeight;

   std::map<int, ConnectionGroup> m_connections;
};
