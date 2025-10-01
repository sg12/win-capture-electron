#include <napi.h>
#include <windows.h>
#include <TlHelp32.h>
#include <vector>
#include "audio-helpers.h"

// Включаем SeDebugPrivilege (обязательно для инжекции)
BOOL EnableDebugPrivilege() {
    HANDLE hToken;
    LUID luid;
    TOKEN_PRIVILEGES tp;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
        return FALSE;
    if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)) {
        CloseHandle(hToken);
        return FALSE;
    }
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
    CloseHandle(hToken);
    return GetLastError() == ERROR_SUCCESS;
}

// Простая функция запуска захвата (PID → инжекция)
Napi::Value StartCapture(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "PID must be a number").ThrowAsJavaScriptException();
        return env.Null();
    }

    uint32_t pid = info[0].As<Napi::Number>().Uint32Value();

    // Включаем привилегию один раз
    static bool privEnabled = false;
    if (!privEnabled) {
        EnableDebugPrivilege();
        privEnabled = true;
    }

    // Запускаем инжекцию (адаптированная функция из inject-helper.c)
    // В реальном проекте: оберни InjectProcess в C++-совместимую функцию
    // Пока просто возвращаем true — логика инжекции будет в inject-helper.c
    InjectProcess(pid);

    return Napi::Boolean::New(env, true);
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set(Napi::String::New(env, "startCapture"),
                Napi::Function::New(env, StartCapture));
    return exports;
}

NODE_API_MODULE(win_capture_audio, Init)
