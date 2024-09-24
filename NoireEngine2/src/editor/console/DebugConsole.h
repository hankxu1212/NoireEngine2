#pragma once

#include "imgui/imgui.h"

class DebugConsole
{
public:
    static DebugConsole& Get()
    {
        static DebugConsole INSTANCE;
        return INSTANCE;
    }

public:
    DebugConsole();
    ~DebugConsole();
    DebugConsole(const DebugConsole&) = delete;
    DebugConsole(DebugConsole&&) = delete;
    DebugConsole& operator=(const DebugConsole&) = delete;
    DebugConsole& operator=(DebugConsole&&) = delete;

public:
    void ClearLog();

    void Draw(const char* title, bool* p_open);

    void ExecCommand(const char* command_line);

    void AddLog(const char* fmt, ...);

private: // Portable helpers
    static int   Stricmp(const char* s1, const char* s2) { int d; while ((d = toupper(*s2) - toupper(*s1)) == 0 && *s1) { s1++; s2++; } return d; }
    static int   Strnicmp(const char* s1, const char* s2, int n) { int d = 0; while (n > 0 && (d = toupper(*s2) - toupper(*s1)) == 0 && *s1) { s1++; s2++; n--; } return d; }
    static char* Strdup(const char* s) { IM_ASSERT(s); size_t len = strlen(s) + 1; void* buf = malloc(len); IM_ASSERT(buf); return (char*)memcpy(buf, (const void*)s, len); }
    static void  Strtrim(char* s) { char* str_end = s + strlen(s); while (str_end > s && str_end[-1] == ' ') str_end--; *str_end = 0; }

private:
    char                  m_InputBuf[256];
    ImVector<char*>       m_Items;
    ImVector<const char*> m_Commands;
    ImVector<char*>       m_History;
    int                   m_HistoryPos;    // -1: new line, 0..m_History.Size-1 browsing history.
    ImGuiTextFilter       m_Filter;
    bool                  m_AutoScroll;
    bool                  m_ScrollToBottom;

private:
    static int TextEditCallbackStub(ImGuiInputTextCallbackData* data);
    int TextEditCallback(ImGuiInputTextCallbackData* data);
};