# Logger Usage Report for `projectM`

This document explains how a third‑party developer can hook into **projectM**'s internal logging system, receive error and diagnostic messages, and control the verbosity of the output.

---

## 1. Overview of the Logging API

`projectM` provides a lightweight, header‑only logging facility in `src/libprojectM/Logging.hpp`. The key concepts are:

| Component                                              | Purpose                                                                                                                               |
| ------------------------------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------- |
| **`Logging::UserCallback`**                            | Holds a function pointer (`callbackFunction`) and an opaque `userData` pointer that will be passed to the callback on each log event. |
| **`Logging::SetGlobalCallback`**                       | Registers a callback that is used by all threads unless a thread‑local callback overrides it.                                         |
| **`Logging::SetThreadCallback`**                       | Registers a callback for the current thread only (useful when you have multiple rendering threads with separate UI sinks).            |
| **`Logging::SetGlobalLogLevel` / `SetThreadLogLevel`** | Controls which severity levels are emitted.                                                                                           |
| **`Logging::LogLevel` enum**                           | Severity levels (Trace, Debug, Information, Warning, Error, Fatal).                                                                   |
| **`LOG_ERROR`, `LOG_WARN`, … macros**                  | Convenience wrappers that only emit a message when the current log level permits it.                                                  |

All log macros eventually call `Logging::Log(message, severity)` which forwards the message to the active callback (global or thread‑local).

---

## 2. Installing a Callback (C++ Example)

```cpp
#include <projectM-4/projectM.h>
#include "Logging.hpp"

using namespace libprojectM;

// Signature required by the logger
void MyLogCallback(const char* message, int severity, void* userData) {
    // Convert the numeric severity to the enum for readability (optional)
    auto level = static_cast<Logging::LogLevel>(severity);

    // Example: forward to std::cout, a GUI console, or a telemetry system
    std::cout << "[projectM][" << static_cast<int>(level) << "] " << message << '\n';
    // If you have a UI object you can use userData to forward the message there.
}

int main() {
    // Register the callback globally – all threads will use it unless they set a thread‑local one.
    Logging::UserCallback cb{};
    cb.callbackFunction = MyLogCallback;
    cb.userData         = nullptr; // optional custom data
    Logging::SetGlobalCallback(cb);

    // Optionally set the log level you care about.
    Logging::SetGlobalLogLevel(Logging::LogLevel::Info);

    // … create a projectM instance and start using the library …
    return 0;
}
```

### C‑API Equivalent

If you are compiling against the C API (`projectm.h`), the same functionality is exposed via a helper function:

```c
void my_log_cb(const char* msg, int severity, void* user) {
    // forward to your preferred sink, e.g., a JavaScript console in WASM
}

// After `projectm_create` you can register it like this:
projectm_set_logging_callback(my_log_cb, NULL);
```

---

## 3. Choosing a Log Level

The logger respects the current severity threshold. Only messages whose severity is **less than or equal to** the configured level are delivered.

```cpp
// Show only warnings, errors and fatal messages:
Logging::SetGlobalLogLevel(Logging::LogLevel::Warning);
```

If you never call `SetGlobalLogLevel`, the default is `Logging::LogLevel::Information` (the _Info_ macro and higher).

---

## 4. Typical Use Cases

### a) Desktop Application (Qt / SDL)

- Register a callback that writes into a _QPlainTextEdit_ or an SDL overlay.
- Use `SetThreadCallback` for the render thread if you want to keep UI logging separate from the main thread.

### b) Web / Web‑Assembly Front‑end

```cpp
extern "C" void wasm_log_bridge(const char* msg, int severity, void* /*unused*/) {
    // Forward to JavaScript console via Emscripten
    emscripten::val::global("console").call<void>(
        severity >= static_cast<int>(Logging::LogLevel::Error) ? "error" : "log",
        std::string(msg)
    );
}

int main() {
    Logging::UserCallback cb{};
    cb.callbackFunction = wasm_log_bridge;
    Logging::SetGlobalCallback(cb);
    Logging::SetGlobalLogLevel(Logging::LogLevel::Info);
    // … rest of initialization …
}
```

Now the browser console shows lines such as:

```
[projectM][4] C-API Exception: std::bad_alloc
```

### c) Embedded / Headless Service

- Direct the callback to a file writer or a syslog service.
- Keep the log level at `Warning` or `Error` to avoid excessive output.

---

## 5. What Happens When an Exception Is Logged?

All `LOG_*` macros are used throughout the library (including the exception‑catch blocks we added in Phase 1 and Phase 2). When an exception occurs inside a C‑API wrapper, the macro logs a message like:

```
[projectM][4] C-API Exception: std::bad_alloc
```

The severity will be **`Error`** for most runtime failures, guaranteeing that the callback receives it if the configured level permits it.

---

## 6. Checklist for Consumers

1. **Include** `Logging.hpp` (or the C header `projectM.h`).
2. **Implement** a function matching `void(const char*, int, void*)`.
3. **Register** it via `Logging::SetGlobalCallback` (or the C counterpart).
4. **Set** a log level that matches your diagnostic needs.
5. **Test** by forcing a failure (e.g., request a non‑existent preset) and confirm the callback receives the message.

---

## 7. Where the Report Lives

The markdown file is saved to the repository workspace at:

```
/home/berto/projectm/logger_usage_report.md
```

You can open it directly in the IDE or view it on GitHub.

---

_Prepared by the Antigravity AI coding assistant._
