## 1. Respect SOLID Principles

All code written for this project must strictly respect **SOLID principles**:

- **S - Single Responsibility Principle (SRP):** Each module, hook, or component should have one, and only one, reason to change.
- **O - Open/Closed Principle (OCP):** Software entities should be open for extension but closed for modification.
- **L - Liskov Substitution Principle (LSP):** Subtypes must be substitutable for their base types.
- **I - Interface Segregation Principle (ISP):** Do not force components to depend on interfaces they do not use.
- **D - Dependency Inversion Principle (DIP):** High-level modules should not depend on low-level modules. Both should depend on abstractions.

## 2. Coding Standards & Best Practices

- **Dart Style:** Strictly adhere to standard Dart conventions: `snake_case` for files/folders, `PascalCase` for classes and types, and `camelCase` for variables and methods.
- **Const Constructors:** Always use `const` widgets and constructors where possible to prevent unnecessary UI rebuilds.
- **Stateless vs Stateful:** Favor `StatelessWidget` over `StatefulWidget`. Keep logic out of the UI tree.
- **State Management:** Avoid passing state deeply through the widget tree (prop drilling). Use a defined state management solution (e.g., Riverpod or Provider) for the global app state.
- **C++ & FFI Boundaries:** Keep the C++ interface small and simple. Only pass primitives or well-defined structs across the FFI boundary. Meticulously manage memory: ensure memory allocated in Dart is freed in Dart, and memory allocated in C++ is freed in C++.

## 3. Project Structure

We follow a **Feature-First** modular architecture. Avoid dumping all views or all controllers into single top-level folders. Group files by the feature they belong to:

```text
lib/
  ├── core/               # Shared utilities, FFI bindings, theming, routing, constants
  ├── features/           # Isolated feature modules
  │   ├── visualizer/     # The OpenGL rendering views and projectM logic
  │   ├── presets/        # Preset browsing, downloading, and liking/banning
  │   └── settings/       # Audio sources and app configuration
  └── main.dart           # Application entry point
```

## 4. Performance & Threading

- **60+ FPS Requirement:** The visualizer must maintain a smooth framerate. Never block the main UI thread with heavy synchronous operations.
- **Use Isolates:** Any heavy data parsing (like reading thousands of preset files) or complex audio analysis mapping must be offloaded to Dart Isolates.
- **Texture Rendering:** Ensure the OpenGL texture rendering pipeline from projectM bypasses heavy Flutter widget rebuilds, updating only the `Texture` widget.

## 5. Error Handling & Logging

- **Fail Gracefully:** Never allow an unhandled C++ exception to crash the Dart application. Catch errors at the FFI boundary and pass them back as error codes or custom Dart Exceptions.
- **Centralized Logging:** Do not use `print()`. Use a dedicated logging package (like `logger`) to record errors, especially for debugging native audio capture and OpenGL issues across different OS platforms.
- Use `flutter analyze` to check for compile errors

## 6. Reuse, Modularity & Component Independence

- **Prefer Reuse:** Reuse existing code and components whenever possible. This reduces duplication, speeds implementation, and keeps the UI consistent.
- **Compose, Don't Duplicate:** Favor composition over copying: wrap or compose smaller shared primitives (buttons, cards, typography, layout containers) to create feature-specific UI rather than creating new ad-hoc implementations.
- **Modular & Independent:** New components should be small, focused, and independent:
  - Accept well-typed props and avoid implicit dependencies on global state.
  - Encapsulate styling and behavior so the component can be reused in different contexts.
  - Provide clear, minimal public APIs and avoid leaking internal implementation details.
- **Testability:** Design components so they can be unit-tested in isolation (pure rendering logic, injected dependencies, or mocked contexts).
- **Discoverability & Documentation:** Add new shared components to the appropriate index/export file and document intended usage and props in a short README or JSDoc comment so other engineers and agents can discover and reuse them.
- **When to Create New Components:** Create new shared components only when there's a clear, repeatable use case; prefer feature-local components for one-off UI that is unlikely to be reused.

## 7. Effectively Using Your AI Agent abilities

Do not start mindlessly writing code! Develop a full-fledged plan first.
Put it in `docs/plans`, you can either choose to write a `.md` file or an `.html` file.

- **Precision & Exhaustiveness:** The plan should always describe all features and changes with precision.
- **Task Tracking:** If you have to do a lot of work, split it into smaller tasks (Divide et impera principle). Use Markdown checkboxes (`[ ]` and `[x]`) in your plan file to track progress. Update the file as you complete each step.
- **Tests in Plan:** Always include a sketch of the required unit tests as part of your implementation plan. They should be written, but can be done at the end of the coding phase.
- **Confirmation:** After writing a first version of the plan, always ask for confirmation from the human. If there are ambiguities, do not invent a solution by yourself, but ask the human.
- **Simplicity:** Always prefer elegant simple solutions that achieve 95% of the goal, rather than complex and hard to understand solutions that might achieve 100% of the goal.
- **Subagents for Research:** If a task requires extensive reading of the codebase or searching the web, delegate the research phase to a background subagent to keep your main context clean and focused.
- **Feedback:** If the human is making a mistake, tell him what he is doing wrong and why, and propose a better solution.
