#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// 텍스트를 클립보드에 복사하는 함수
BOOL CopyTextToClipboard(const char* text) {
    if (!OpenClipboard(NULL)) {
        printf("Cannot open clipboard.\n");
        return FALSE;
    }

    if (!EmptyClipboard()) {
        printf("Cannot empty clipboard.\n");
        CloseClipboard();
        return FALSE;
    }

    size_t textLength = strlen(text) + 1;
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, textLength);
    if (!hMem) {
        printf("Cannot allocate memory.\n");
        CloseClipboard();
        return FALSE;
    }

    char* pMem = (char*)GlobalLock(hMem);
    if (!pMem) {
        printf("Cannot lock memory.\n");
        GlobalFree(hMem);
        CloseClipboard();
        return FALSE;
    }

    memcpy(pMem, text, textLength);
    GlobalUnlock(hMem);

    if (!SetClipboardData(CF_TEXT, hMem)) {
        printf("Cannot set clipboard data.\n");
        GlobalFree(hMem);
        CloseClipboard();
        return FALSE;
    }

    CloseClipboard();
    return TRUE;
}

// 클립보드에서 텍스트를 가져오는 함수
char* PasteTextFromClipboard() {
    static char* clipboardText = NULL;

    if (clipboardText) {
        free(clipboardText);
        clipboardText = NULL;
    }

    if (!OpenClipboard(NULL)) {
        printf("Cannot open clipboard.\n");
        return NULL;
    }

    HANDLE hData = GetClipboardData(CF_TEXT);
    if (!hData) {
        printf("Cannot get data from clipboard.\n");
        CloseClipboard();
        return NULL;
    }

    char* pData = (char*)GlobalLock(hData);
    if (!pData) {
        printf("Cannot access clipboard data.\n");
        CloseClipboard();
        return NULL;
    }

    clipboardText = _strdup(pData);

    GlobalUnlock(hData);
    CloseClipboard();

    return clipboardText;
}

// 특정 창에 텍스트를 붙여넣는 함수
BOOL PasteTextToWindow(HWND hWnd, const char* text) {
    if (!CopyTextToClipboard(text)) {
        return FALSE;
    }

    // 창을 활성화
    if (!SetForegroundWindow(hWnd)) {
        printf("Cannot activate window.\n");
        return FALSE;
    }

    // 창에 포커스를 주고 텍스트 붙여넣기
    SetFocus(hWnd); // 창에 포커스 주기
    Sleep(100); // 잠시 대기
    keybd_event(VK_CONTROL, 0, 0, 0);
    keybd_event('V', 0, 0, 0);
    keybd_event('V', 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);

    return TRUE;
}

// 메모장 창에서 텍스트 복사
BOOL CopyTextFromNotepad(HWND hWnd) {
    if (!SetForegroundWindow(hWnd)) {
        printf("Cannot activate window.\n");
        return FALSE;
    }

    keybd_event(VK_CONTROL, 0, 0, 0);
    keybd_event('A', 0, 0, 0);
    keybd_event('A', 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);

    Sleep(100);

    keybd_event(VK_CONTROL, 0, 0, 0);
    keybd_event('C', 0, 0, 0);
    keybd_event('C', 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);

    Sleep(100);

    return TRUE;
}

// Notepad++ 창 핸들 찾기 (부분 일치)
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    char title[256];
    GetWindowTextA(hwnd, title, sizeof(title));
    if (strstr(title, "Notepad++")) {
        *((HWND*)lParam) = hwnd;
        return FALSE; // 찾았으니 중단
    }
    return TRUE; // 계속 탐색
}

// 메인 함수
int main() {
    // Notepad++ 실행
    ShellExecute(NULL, "open", "C:\\Program Files\\Notepad++\\notepad++.exe", NULL, NULL, SW_SHOWNORMAL);
    Sleep(1000); // 창이 뜰 시간

    // Notepad++ 창 찾기
    HWND hNotepad = NULL;
    EnumWindows(EnumWindowsProc, (LPARAM)&hNotepad);
    if (!hNotepad) {
        printf("Cannot find Notepad++ window. Please launch Notepad++.\n");
        return 1;
    }

    // 텍스트 복사 및 붙여넣기
    if (CopyTextFromNotepad(hNotepad)) {
        Sleep(200); // 복사 안정성 확보
    
        // 클립보드에서 복사된 텍스트 읽기
        char* copiedText = PasteTextFromClipboard(); 
        if (!copiedText) {
            printf("Failed to read from clipboard.\n");
            return 1;
        }
        
        // 새로운 텍스트 생성 - "paste completed!: " + 복사된 텍스트
        const char* prefix = "paste completed!: ";
        size_t newLength = strlen(prefix) + strlen(copiedText) + 1; // +1 for null terminator
        char* newText = (char*)malloc(newLength);
        
        if (!newText) {
            printf("Failed to allocate memory for new text.\n");
            return 1;
        }
        
        strcpy(newText, prefix);
        strcat(newText, copiedText);
        
        // 먼저 전체 텍스트를 선택하여 지우기
        if (SetForegroundWindow(hNotepad)) {
            Sleep(100);
            keybd_event(VK_CONTROL, 0, 0, 0);
            keybd_event('A', 0, 0, 0);
            keybd_event('A', 0, KEYEVENTF_KEYUP, 0);
            keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
            Sleep(100);
        }
    
        // 새로 만든 텍스트를 붙여넣기
        if (PasteTextToWindow(hNotepad, newText)) {
            printf("Text modified and pasted successfully.\n");
        } else {
            printf("Failed to paste modified text.\n");
        }
        
        // 메모리 해제
        free(newText);
    }

    return 0;
}