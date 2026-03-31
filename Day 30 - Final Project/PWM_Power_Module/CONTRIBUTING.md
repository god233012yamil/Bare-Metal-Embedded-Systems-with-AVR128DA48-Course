# Contributing to PWM-Controlled Power Module

Thank you for your interest in contributing! This document describes the process and coding standards expected for all contributions.

---

## Table of Contents

- [Getting Started](#getting-started)
- [How to Contribute](#how-to-contribute)
- [Coding Standards](#coding-standards)
- [Commit Message Format](#commit-message-format)
- [Pull Request Checklist](#pull-request-checklist)
- [Reporting Bugs](#reporting-bugs)
- [Requesting Features](#requesting-features)

---

## Getting Started

1. **Fork** the repository on GitHub.
2. **Clone** your fork locally:
   ```
   git clone https://github.com/your-username/PWM_Power_Module.git
   ```
3. Create a **feature branch**:
   ```
   git checkout -b feature/your-feature-name
   ```
4. Make your changes, following the standards below.
5. Open a **Pull Request** against the `main` branch of the upstream repository.

---

## How to Contribute

| Type | Branch name convention |
|------|----------------------|
| Bug fix | `fix/short-description` |
| New feature | `feature/short-description` |
| Documentation | `docs/short-description` |
| Refactoring | `refactor/short-description` |

All changes must be verified to **build without errors or warnings** in Atmel Studio 7 / Microchip Studio targeting `AVR128DA48` with DFP 2.4.286 before a PR is opened.

---

## Coding Standards

This project follows a strict set of embedded C conventions to ensure the code is readable, portable, and safe.

### Language & Toolchain

- **C99** (`-std=gnu99` as set by AVR-GCC).
- No C++ features, no `cplusplus` ifdefs.
- No dynamic memory allocation (`malloc`, `calloc`, `realloc` are forbidden).
- No `delay_ms()` or any blocking spin-wait.

### Naming Conventions

| Entity | Convention | Example |
|--------|-----------|---------|
| Functions | `MODULE_VerbNoun` | `ADC_GetResult()` |
| Types / structs | `PascalCase_t` | `Fifo_t`, `CtrlState_t` |
| Enums | `SCREAMING_SNAKE_CASE` | `CTRL_STATE_IDLE` |
| Macros / constants | `SCREAMING_SNAKE_CASE` | `TCA0_PERIOD` |
| Global variables | `g_camelCase` | `g_adcResult` |
| Local variables | `camelCase` | `newDuty` |
| ISR-shared variables | `volatile` + `g_` prefix | `volatile bool g_tickFlag` |

### File Structure

Every `.c` file must begin with a Doxygen file-level comment block:

```c
/**
 * @file    module.c
 * @brief   One-sentence summary.
 *
 * Longer description, design rationale, register-level notes.
 *
 * @author  Your Name
 * @date    YYYY-MM-DD
 * @version X.Y.Z
 */
```

### Function Documentation

All public functions (declared in a `.h` file) must have a Google-style Doxygen docstring:

```c
/**
 * @brief Short one-line summary.
 *
 * Optional longer description.
 *
 * @param[in]  paramA  Description of input parameter.
 * @param[out] paramB  Description of output parameter.
 * @return Description of return value.
 */
```

Private (static) functions should have at minimum a `@brief` line.

### Layer Rules

- **Application layer** (`main.c`): must not access any peripheral registers directly. All hardware interaction must go through a driver API function.
- **Driver layer** (`adc.c`, `tca.c`, etc.): owns its peripheral exclusively. No other module may write to registers owned by a driver.
- **Configuration** (`config.h`): all tuning constants, pin assignments, and feature flags must live here. No magic numbers in `.c` files.

### ISR Rules

- ISRs must be as short as possible — set a flag, store a value, clear the hardware flag, return.
- All variables shared between ISR and main context must be declared `volatile`.
- Multi-byte shared variables must be read inside a `cli() / sei()` critical section in main context.
- ISRs must not call driver functions that are not explicitly ISR-safe.

### Formatting

- Indent with **4 spaces** (no tabs).
- Opening braces on the **same line** as the statement for `if`, `for`, `while`; on a **new line** for function definitions.
- Line length: soft limit 100 characters, hard limit 120.
- Exactly one blank line between function definitions in a `.c` file.

---

## Commit Message Format

Use the [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/) format:

```
<type>(<scope>): <short summary>

[optional body]

[optional footer(s)]
```

**Types:**

| Type | When to use |
|------|-------------|
| `feat` | New feature or capability |
| `fix` | Bug fix |
| `docs` | Documentation only |
| `refactor` | Code restructure, no behaviour change |
| `test` | Adding or updating tests |
| `chore` | Build system, CI, dependency updates |

**Examples:**

```
fix(ac): replace AC_ACOUT_bm with AC_CMPSTATE_bm

AC_ACOUT_bm does not exist in the AVR-Dx DFP ioavr128da48.h.
The correct mask for the live comparator output state is AC_CMPSTATE_bm (0x10).
```

```
feat(usart): add TX FIFO with DRE interrupt

Replaces polled USART_SendByte() with an interrupt-driven transmit path,
eliminating CPU stalls during telemetry emission.

Closes #12
```

---

## Pull Request Checklist

Before opening a PR, confirm the following:

- [ ] Code builds with **0 errors, 0 warnings** in Atmel Studio 7 (Debug and Release configurations).
- [ ] All public functions have complete Doxygen docstrings.
- [ ] No magic numbers — all constants are in `config.h`.
- [ ] No `delay_ms()` or blocking spin-waits introduced.
- [ ] ISR-shared variables are declared `volatile`.
- [ ] Layer boundaries are respected (app layer uses driver API only).
- [ ] `CHANGELOG.md` has been updated under `[Unreleased]`.
- [ ] Commit messages follow the Conventional Commits format.
- [ ] PR description explains **what** changed and **why**.

---

## Reporting Bugs

Open a GitHub Issue and include:

1. A clear title summarising the problem.
2. **Observed behaviour** — what actually happens.
3. **Expected behaviour** — what should happen.
4. **Steps to reproduce** (minimal).
5. Hardware details: board revision, DFP version, Studio version, OS.
6. Any relevant USART output or debug session screenshots.

---

## Requesting Features

Open a GitHub Issue with the `enhancement` label and describe:

1. The problem you are trying to solve.
2. The proposed solution or behaviour.
3. Any peripheral or register constraints relevant to the AVR128DA48.

---

Thank you for helping improve this project!
