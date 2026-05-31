# Stitch: Minimalist Modal Editor

## Core Philosophy (DO NOT DELETE)
The ultimate goal of Stitch is to be a **"gentle modal text editor"**. It is designed to be simple, minimal, yet powerful enough for most daily tasks. Inspired heavily by the approachability of GNU Nano, it provides a clean interface and straightforward workflow. Stitch explicitly avoids the goal of becoming a hyper-complex, omnipotent environment. It is not meant to be everything to everyone; rather, it provides exactly what is needed for fast, focused text editing without the bloat.

Stitch is a C11 modal text editor that combines a clean bottom-bar aesthetic with traditional modal efficiency. It follows a strictly modular, component-based architecture to ensure industry-standard maintainability and scalability.

## Architecture

The codebase is organized into four primary domains, following a **Handler-Component** pattern. Global state is strictly forbidden; all application data is encapsulated in a [StitchState](file:///data/data/com.termux/files/home/works/stitch/include/stitch/types.h#L118) context and injected via pointers.

### Domain Map & Component Breakdown

- **Core (`src/core/`)**: System-level terminal state, input normalization, and memory management.
  - [sys_memory.c](file:///data/data/com.termux/files/home/works/stitch/src/core/sys/sys_memory.c): Memory management wrappers `editorMalloc` and `editorRealloc` for centralized heap allocation error handling.
  - [sys_raw_mode.c](file:///data/data/com.termux/files/home/works/stitch/src/core/sys/sys_raw_mode.c): Handles entering/exiting POSIX terminal raw mode and initializing ncursesw with settings like escdelay (50ms). Sets up the "Organic Warmth" color palette (Sage, Terracotta, Ochre, Earth Background).
  - [terminal_handler.c](file:///data/data/com.termux/files/home/works/stitch/src/core/terminal_handler.c): Handles reading keys via `core_read_key`, checks terminal size changes, and tracks key bindings.

- **Buffer (`src/buffer/`)**: Text data management, file I/O, and transaction history.
  - [buffer_handler.c](file:///data/data/com.termux/files/home/works/stitch/src/buffer/buffer_handler.c): Text navigation helpers, line length calculations, tab stops, and converting between editor screen columns (x-coordinate) and actual byte indexes in the string.
  - [io_handler.c](file:///data/data/com.termux/files/home/works/stitch/src/buffer/io_handler.c): Deals with reading/writing buffer to files, mapping lines dynamically.
  - `operations/`: Atomic buffer transformations.
    - [buf_insert.c](file:///data/data/com.termux/files/home/works/stitch/src/buffer/operations/buf_insert.c): Handles character and newline insertions.
    - [buf_delete.c](file:///data/data/com.termux/files/home/works/stitch/src/buffer/operations/buf_delete.c): Handles character and line deletions.
    - [buf_line.c](file:///data/data/com.termux/files/home/works/stitch/src/buffer/operations/buf_line.c): Handles inserting whole new lines and copying/reallocating buffer arrays.
    - [undo_handler.c](file:///data/data/com.termux/files/home/works/stitch/src/buffer/operations/undo_handler.c): Implements transaction-grouped undo/redo operations using an transaction stack structure.

- **UI (`src/ui/`)**: Drawing widgets and managing screen updates.
  - [render_handler.c](file:///data/data/com.termux/files/home/works/stitch/src/ui/render_handler.c): The central drawing logic. Redraws the view, expands tab sequences, coordinates child UI layouts, and manages cursor scrolling thresholds.
  - [prompt.c](file:///data/data/com.termux/files/home/works/stitch/src/ui/prompt.c): Manages interactive inputs, such as asking for a save location, entering query patterns, and navigating through command-line command history.
  - `components/`: Modular widgets drawn onto the terminal screen.
    - [ui_status_bar.c](file:///data/data/com.termux/files/home/works/stitch/src/ui/components/ui_status_bar.c): Renders the mode bar at the bottom, drawing current Mode, filename, modification status, and cursor coordinates.
    - [ui_message_bar.c](file:///data/data/com.termux/files/home/works/stitch/src/ui/components/ui_message_bar.c): Displays prompt labels, error messages, and info notifications.
    - [ui_text_grid.c](file:///data/data/com.termux/files/home/works/stitch/src/ui/components/ui_text_grid.c): Renders buffer line characters, selection highlighting (Visual Mode), dynamic line numbers, and matching bracket pairing highlights.
    - [ui_help_overlay.c](file:///data/data/com.termux/files/home/works/stitch/src/ui/components/ui_help_overlay.c): Displays a list of quick keybindings in a popup menu block.

- **Editor (`src/editor/`)**: Action dispatchers, mode management, and built-in interactive commands.
  - [mode_handler.c](file:///data/data/com.termux/files/home/works/stitch/src/editor/mode_handler.c): Routes input characters to specific sub-handlers according to the current editor mode.
  - [command_handler.c](file:///data/data/com.termux/files/home/works/stitch/src/editor/command_handler.c): Parses command inputs (like `:nu`, `:w`, `:q`, `:e`), executing them and tracking history.
  - `modes/`: Individual keyboard mapping implementations.
    - [mode_common.c](file:///data/data/com.termux/files/home/works/stitch/src/editor/modes/mode_common.c): Shared navigation helpers like `editor_move_cursor` (incorporating scroll margins).
    - [mode_normal.c](file:///data/data/com.termux/files/home/works/stitch/src/editor/modes/mode_normal.c): Keyboard shortcuts for navigation, mode switching, editing, undo/redo trigger, and launching search.
    - [mode_insert.c](file:///data/data/com.termux/files/home/works/stitch/src/editor/modes/mode_insert.c): Insert mode keys (writing characters directly, Backspace/Del editing).
    - [mode_command_prompt.c](file:///data/data/com.termux/files/home/works/stitch/src/editor/modes/mode_command_prompt.c): Passes input straight to prompt commands.
    - [mode_visual.c](file:///data/data/com.termux/files/home/works/stitch/src/editor/modes/mode_visual.c): Ranges-selection, bulk deletes, and select-all actions.
  - `commands/`:
    - [cmd_quit.c](file:///data/data/com.termux/files/home/works/stitch/src/editor/commands/cmd_quit.c): Verifies clean quit conditions.
    - [cmd_save.c](file:///data/data/com.termux/files/home/works/stitch/src/editor/commands/cmd_save.c): Handles write triggers.
    - [cmd_search.c](file:///data/data/com.termux/files/home/works/stitch/src/editor/commands/cmd_search.c): Incremental pattern-matching searches forward/backward.
    - [cmd_shell.c](file:///data/data/com.termux/files/home/works/stitch/src/editor/commands/cmd_shell.c): Fork-exec wrapper for launching asynchronous external commands using `!` syntax.

## Data Structures & Layout

The editor's context structures are defined in [types.h](file:///data/data/com.termux/files/home/works/stitch/include/stitch/types.h):
- [Line](file:///data/data/com.termux/files/home/works/stitch/include/stitch/types.h#L40): Holds the raw string buffer, rendered representation, size, and allocated capacity.
- [UndoAction](file:///data/data/com.termux/files/home/works/stitch/include/stitch/types.h#L60) & [UndoStack](file:///data/data/com.termux/files/home/works/stitch/include/stitch/types.h#L72): Implements transaction nodes tracking inserted/deleted characters/lines.
- [StitchBuffer](file:///data/data/com.termux/files/home/works/stitch/include/stitch/types.h#L77): Manages line arrays, dirty flag, file path, and the undo/redo stack.
- [StitchView](file:///data/data/com.termux/files/home/works/stitch/include/stitch/types.h#L86): Renders window geometry, off-screen offsets (`row_off`/`col_off`), and cursor coordinate offsets (`cx`/`cy`).
- [StitchUI](file:///data/data/com.termux/files/home/works/stitch/include/stitch/types.h#L95): Stores line-number display states, overlay popup triggers, and the bottom message text.
- [StitchEditor](file:///data/data/com.termux/files/home/works/stitch/include/stitch/types.h#L101): Keeps active modes, visual anchors, recent search matches, and command history cache.
- [StitchCore](file:///data/data/com.termux/files/home/works/stitch/include/stitch/types.h#L111): Keeps terminal raw attributes (`orig_termios`) and spawned shell process PIDs.
- [StitchState](file:///data/data/com.termux/files/home/works/stitch/include/stitch/types.h#L118): The top-level combined state context passed around the app.

## Technical Standards

### Scale & Performance
- **Large File Support**: All coordinates, line counts, and buffer lengths use `size_t`, ensuring safety for files >2GB.
- **Amortized O(1) Editing**: Lines utilize exponential capacity growth (doubling on overflow) to minimize reallocations during active typing.
- **Memory Safety**: All allocations must use [sys_memory.c](file:///data/data/com.termux/files/home/works/stitch/src/core/sys/sys_memory.c) wrappers `editorMalloc` and `editorRealloc` for centralized error handling.

### Modularization
Every specific behavior is isolated to its own file. Do not introduce monolithic handlers. Maintain headers under `include/stitch/` corresponding to their domain.

### Quality of Life (QoL)
- **Smart Undo/Redo Engine**: Atomic operations are grouped intelligently. Contiguous typing of characters is grouped by words (breaking on space or tabs), and contiguous deletions form a single block. Bound to `u` and `U`.
- **Visual Mode**: Accessible via `v`. Allows selecting blocks of text to delete (`d`/`x`). Yanking (`y`) is currently disabled. Press `%` to instantly select the entire file. Supports multi-line selections and reverse-video highlighting.
- **Stabilization Audit (v0.1.1)**: Comprehensive hardening of memory management, UTF-8 safety, and atomic buffer operations.
- **Dynamic Line Numbers**: Toggleable via `:number` (or `:nu`) and `:nonumber` (or `:nonu`).
- **Help Overlay**: Accessible via `:h` or `:help`. Displays a clean, structured guide to core keybindings, dismissed with `Esc`.
- **Mouse Support**: Basic click-to-move support for intuitive cursor positioning within the text grid.
- **Scroll Margins (Scrolloff)**: Horizontal (5 chars) and vertical (3 lines) margins ensure the cursor always stays within a visible context area.
- **Tab Key Support**: Inserts a tab character with a configurable stop (standardized to 4 spaces).
- **Bracket Matching**: Real-time highlighting of matching `()`, `[]`, and `{}` pairs.
- **Mode Switching**: The `escdelay` is minimized to 50ms to ensure instantaneous transitions between Insert and Normal modes.
- **Command History**: Allows navigating through the last 10 commands using Arrow Up/Down inside the command prompt mode.

## Building and Running

### Build Requirements
- GCC or Clang with C11 support.
- POSIX-compliant environment.
- **ncursesw** library.

### Commands
- **Build**: `make` (Produces a zero-warning build in `build/stitch`).
- **Run**: `./build/stitch [filename]`
- **Clean**: `make clean`

## Development Conventions

### Standards
- **Language**: Strict C11 (`-std=c11`).
- **Portability**: Target POSIX.1-2008 and `_XOPEN_SOURCE_EXTENDED` for ncursesw.
- **Naming**: Strict domain-based prefixing (e.g., `ui_`, `buffer_`, `core_`, `cmd_`).

### UI & Aesthetics
- **Design Philosophy**: Ultra-minimalist "Monochrome-Plus."
- **Aesthetic**: Uses standard terminal transparency/backgrounds. Color is strictly reserved for functional state indicators (Mode and Position) using the "Organic Warmth" palette (Sage Green, Terracotta Orange, Ochre Yellow).
- **Clean Interface**: Sparse and functional, with no decorative backgrounds.
- **Modified Indicator**: `*` directly attached to the filename in the status bar.
