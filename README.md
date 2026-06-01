# Stitch

Stitch is a minimalist, modal text editor written in C11. It is designed to provide a focused editing environment with a clean interface and a modular architecture.

## Features

- **Modal Editing**: Separate modes for navigation, insertion, and commands.
- **Component-Based Architecture**: Modular design for maintainability.
- **Interactive Tutor**: Built-in interactive 11-lesson tutorial buffer (`:help`) that safely stashes your work while you practice.
- **Visual Context**: Horizontal and vertical scroll margins to keep the cursor within a visible area.
- **Quality of Life**: Mouse support for cursor positioning, undo/redo engine, and real-time bracket matching.
- **Clean UI**: A functional status bar and message area with a minimal aesthetic.

## Installation

### Prerequisites

- A C11-compliant compiler (GCC or Clang).
- `ncursesw` library (including headers).
- A POSIX-compliant environment.

### Building

To build the project, use the provided Makefile:

```bash
make
```

The binary will be generated in the `build/` directory.

### Running

```bash
./build/stitch [filename]
```

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
