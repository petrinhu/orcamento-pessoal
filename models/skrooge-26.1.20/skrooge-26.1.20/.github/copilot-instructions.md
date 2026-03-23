# Copilot Coding Agent Onboarding Instructions for Skrooge

## High-Level Repository Overview

**Project:** Skrooge – Personal Finance Manager

**Summary:**
Skrooge is a cross-platform personal finance management application powered by KDE. It allows users to manage, analyze, and report on their finances, supporting a wide range of import/export formats and advanced features such as plugins, dashboards, and automation. The project is written in C++ and leverages Qt6 and KDE Frameworks 6, with support for Linux, BSD, macOS, and Windows.

**Repository Size & Structure:**
- Large, multi-directory C++/Qt/KDE project
- Main code in: `skrooge/`, `skgbankgui/`, `skgbankmodeler/`, `skgbasegui/`, `skgbasemodeler/`, `skgsqlcipher/`, `plugins/`
- Build system: CMake (see `CMakeLists.txt` at root)
- Scripts: `scripts/` (build, code checks, utilities)
- Tests: `tests/`
- Documentation: `doc/`, `README.md`, `CHANGELOG`, `Doxyfile`
- Packaging: `Dockerfile`, `appimage.yml`, `snapcraft.yaml`
- Linting/formatting: `.clang-format`

**Languages & Frameworks:**
- C++17 (see `CMakeLists.txt`)
- Qt6 (Widgets, WebEngine, DBus, etc.)
- KDE Frameworks 6 (multiple modules)
- CMake >= 3.16
- Optional Python for some scripts

## Project Layout & Key Files
- **Root:** CMakeLists.txt, README.md, CHANGELOG, Dockerfile, appimage.yml, snapcraft.yaml, .clang-format
- **Source:** skrooge/, skgbankgui/, skgbankmodeler/, skgbasegui/, skgbasemodeler/, skgsqlcipher/, plugins/
- **Build scripts:** scripts/
- **Tests:** tests/
- **Docs:** doc/
- **CI:** .gitlab-ci.yml, .kde-ci.yml (uses KDE and GitLab CI templates)

## Agent Guidance
- **Trust these instructions.** Only search for additional information if a step fails or is missing.
- **Always install dependencies and build as described above.**
- **Always run tests and code checks before submitting changes.**
- **Respect code style and formatting.**
- **For new features or bugfixes, prefer placing code in the appropriate subdirectory (see Project Layout).**
- **For packaging or deployment, refer to the provided YAML and Docker files.**
- **If in doubt, check the README.md and CMakeLists.txt first.**

---
