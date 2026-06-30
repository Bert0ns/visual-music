# ProjectM 4 Exception Handling Patch Report

## The Bug
In the ProjectM 4 codebase, several custom exception classes inherit from `std::exception` but fail to override the standard `what()` method. Instead, they define a custom method `const std::string& message() const`.

Because `what()` is not overridden, if these exceptions are caught by reference to the base class (`catch (const std::exception& e)`), calling `e.what()` simply returns the default string `"std::exception"`, completely erasing the underlying error message. 

Furthermore, the public C-API wrapper (`ProjectMCWrapper.cpp`) wraps the instance creation in a blanket `catch (...)` block, which silently swallows the error and returns a null pointer without any logging.

## Proposed Pull Request Changes

To fix this, the custom exception classes should override `what()`, and the C-wrapper should at least catch `std::exception` and log the error to `stderr` before returning null.

### 1. Fix `ShaderException`
**File:** `src/libprojectM/Renderer/Shader.hpp`

```diff
 class ShaderException : public std::exception
 {
 public:
     inline ShaderException(std::string message)
         : m_message(std::move(message))
     {
     }
 
     virtual ~ShaderException() = default;
 
     const std::string& message() const
     {
         return m_message;
     }
 
+    const char* what() const noexcept override
+    {
+        return m_message.c_str();
+    }
+
 private:
     std::string m_message;
 };
```

### 2. Fix `PresetFactoryException`
**File:** `src/libprojectM/PresetFactoryManager.hpp`

```diff
 class PresetFactoryException : public std::exception
 {
 public:
     inline PresetFactoryException(std::string message)
         : m_message(std::move(message))
     {
     }
 
     virtual ~PresetFactoryException() = default;
 
     const std::string& message() const
     {
         return m_message;
     }
 
+    const char* what() const noexcept override
+    {
+        return m_message.c_str();
+    }
+
 private:
     std::string m_message;
 };
```

*(Note: The same override should be applied to `MilkdropPresetLoadException` and `MilkdropCompileException` in `src/libprojectM/MilkdropPreset/MilkdropPresetExceptions.hpp`, as well as `PlaylistEmptyException` in `src/playlist/Playlist.hpp`)*.

### 3. Improve C-API Wrapper Logging
**File:** `src/libprojectM/ProjectMCWrapper.cpp`

```diff
+#include <iostream>
 #include <projectM-4/projectM.h>
 
 projectm_handle projectm_create()
 {
     try
     {
         auto projectMInstance = new libprojectM::projectMWrapper();
         return reinterpret_cast<projectm_handle>(projectMInstance);
     }
+    catch (const std::exception& e)
+    {
+        std::cerr << "projectm_create caught exception: " << e.what() << std::endl;
+        return nullptr;
+    }
     catch (...)
     {
+        std::cerr << "projectm_create caught unknown exception" << std::endl;
         return nullptr;
     }
 }
```
