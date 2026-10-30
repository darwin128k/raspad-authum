#!/usr/bin/env python3
"""Check that Authum knows nothing about the platform it runs on.

Authum is a thin facade over lh: the OS calls, the libc calls and the platform
types belong to lh (and, under `lh/os/system/`, to the backend).  A module of
Authum that opens a file, prints, sleeps or resolves a symbol on its own is a
brick in the wrong layer.

Two headers are exempt and the exemption is the point of them: `export.h` and
`symbol.h` sit on the ABI boundary with the host process (a DLL export macro
and a symbol lookup), so the platform is named there on purpose.
"""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
TOKENS = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')

# Platform headers: the OS, the C runtime and the sockets.
FORBIDDEN_HEADERS = [
    "windows.h", "winbase.h", "winnt.h", "winsock2.h", "ws2tcpip.h", "process.h",
    "io.h", "fcntl.h", "unistd.h", "dirent.h", "pthread.h", "signal.h", "time.h",
    "stdio.h", "stdlib.h", "string.h", "strings.h", "errno.h", "sys/socket.h",
    "sys/time.h", "sys/types.h", "netinet/in.h", "arpa/inet.h", "netdb.h",
]
# Calls that belong to lh, not to Authum.
FORBIDDEN_CALLS = [
    (r'(?<![\w>.])(?:printf|fprintf|sprintf|snprintf|vprintf|vsnprintf|puts|fputs)\s*\(',
     "print through the lh logger or the engine console"),
    (r'(?<![\w>.])(?:fopen|freopen|fdopen|fclose|fflush|setvbuf|fread|fwrite|fgets|fgetc|fputc|feof|ferror|remove|rename)\s*\(',
     "open and write files through lh/os/system/fs/file.h"),
    (r'(?<![\w>.])(?:getenv|setenv|putenv|system|popen|_popen)\s*\(',
     "read the environment through lh/os/env.h"),
    (r'(?<![\w>.])(?:strlen|strcmp|strncmp|strcpy|strncpy|strcat|strncat|strchr|strrchr|strstr|strtok|strspn|strcspn|strpbrk|strerror|strdup|strndup)\s*\(',
     "use the lh string kernels (lh/str/*)"),
    (r'(?<![\w>.])(?:strtol|strtoul|strtoll|strtoull|strtod|strtof|atoi|atol|atoll|atof|abs|labs|llabs)\s*\(',
     "use the lh numeric parsers (lh/str/parse/*)"),
    (r'(?<![\w>.])(?:time|clock|gettimeofday|localtime|gmtime|mktime|ctime|difftime|strftime)\s*\(',
     "use the lh clock (lh/os/tick.h, lh/os/timestamp.h)"),
    (r'(?<![\w>.])(?:sleep|usleep|nanosleep)\s*\(',
     "use the lh condition or timer"),
    (r'(?<![\w>.])(?:socket|bind|listen|accept|connect|send|recv|select|poll|setsockopt|getsockopt|getaddrinfo|freeaddrinfo|gethostbyname)\s*\(',
     "use the lh sockets (lh/os/net/socket.h)"),
    (r'(?<![\w>.])(?:getpid|getuid|getgid|getcwd|chdir|mkdir|rmdir|unlink|stat|fstat|lstat|opendir|readdir|closedir)\s*\(',
     "use the lh filesystem (lh/os/system/fs/file.h)"),
    (r'(?<![\w>.])LoadLibrary(?:A|W)?\s*\(', "load modules through lh/os/module.h"),
    (r'(?<![\w>.])GetProcAddress\s*\(', "resolve module symbols through lh/os/module.h"),
    (r'(?<![\w>.])GetModuleHandle(?:A|W)?\s*\(', "resolve module symbols through lh/os/module.h"),
    (r'(?<![\w>.])(?:CreateFile[AW]?|ReadFile|WriteFile|CloseHandle|GetLastError|SetLastError)\s*\(',
     "use the lh OS backend"),
    (r'(?<![\w>.])(?:VirtualAlloc|VirtualFree|VirtualProtect|HeapAlloc|HeapFree|HeapReAlloc)\s*\(',
     "allocate through the lh allocator"),
    (r'(?<![\w>.])(?:CreateThread|ExitThread|TerminateThread|WaitForSingleObject|WaitForMultipleObjects|CreateEvent|CreateMutex|CreateSemaphore|ReleaseMutex|SetEvent|ResetEvent|Sleep|SleepEx|TlsAlloc|TlsGetValue|TlsSetValue)\s*\(',
     "use the lh threads (lh/os/thread.h)"),
    (r'(?<![\w>.])(?:GetSystemTime|GetLocalTime|SystemTimeToFileTime|FileTimeToSystemTime|QueryPerformanceCounter|QueryPerformanceFrequency|GetTickCount)\s*\(',
     "use the lh clock (lh/os/tick.h)"),
]
# Platform macros: Authum code must not branch on the OS it runs on.
FORBIDDEN_MACROS = [
    (r'#\s*if(?:def)?\s+_?(?:WIN32|WIN64|_MSC_VER|__linux__|__unix__|__APPLE__)\b',
     "branch on the platform: the platform belongs to lh"),
    (r'#\s*if\s+defined\s*\(\s*_?(?:WIN32|WIN64)\s*\)',
     "branch on the platform: the platform belongs to lh"),
]
# The ABI boundary with the host process: the platform is named there on purpose.
EXEMPT = ("include/authum/export.h", "include/authum/symbol.h")


def mask(text, strings):
    def replace(match):
        token = match.group()
        if token.startswith(("//", "/*")) or strings:
            return "".join("\n" if char == "\n" else " " for char in token)
        return token
    return TOKENS.sub(replace, text)


def main():
    errors = []
    paths = [path for folder in ("src/authum", "include/authum", "examples")
             for path in (ROOT / folder).rglob("*") if path.suffix in (".c", ".cpp", ".h")]
    checked = 0
    for path in paths:
        relative = path.relative_to(ROOT).as_posix()
        if relative in EXEMPT:
            continue
        checked += 1
        raw = path.read_bytes()
        text = raw.decode("utf-8")
        uncommented, code = mask(text, False), mask(text, True)
        for header in FORBIDDEN_HEADERS:
            pattern = r'^\s*#\s*include\s*<\s*' + re.escape(header) + r'\s*>'
            for match in re.finditer(pattern, uncommented, re.MULTILINE):
                line = uncommented.count("\n", 0, match.start()) + 1
                errors.append(f"{relative}:{line}: include <{header}>: the platform belongs to lh")
        for pattern, reason in FORBIDDEN_CALLS:
            for match in re.finditer(pattern, code):
                line = code.count("\n", 0, match.start()) + 1
                errors.append(f"{relative}:{line}: {reason}")
        for pattern, reason in FORBIDDEN_MACROS:
            for match in re.finditer(pattern, uncommented, re.MULTILINE):
                line = uncommented.count("\n", 0, match.start()) + 1
                errors.append(f"{relative}:{line}: {reason}")
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1
    print(f"No native calls in {checked} Authum files (exempt: {', '.join(EXEMPT)}).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
