# np Language Support for VS Code

This extension adds syntax highlighting and basic language configuration support for the `np` programming language.

## Features

- **Syntax Highlighting**: Supports keywords (`fn`, `if`, `elif`, `else`, `for`, `while`, `return`, etc.), logical operators (`and`, `or`, `not`), numbers, string literals, variables, and function definitions.
- **Auto-Closing Brackets**: Auto-closes `{ }`, `[ ]`, `( )`, and quotation marks.
- **Comments Toggle**: Use `Ctrl + /` (Windows/Linux) or `Cmd + /` (macOS) to toggle line comments (`#`).
- **Indentation Rules**: Automatically handles Python-style colon `:` indent increases and decreases.

## Installation

### Method 1: Local Installation (Fastest)

1. Copy the `vscode-np` folder directly into your VS Code extensions folder:
   - **Windows**: Copy to `%USERPROFILE%\.vscode\extensions\`
   - **macOS / Linux**: Copy to `~/.vscode/extensions/`
2. Restart or reload your VS Code editor.

### Method 2: Package and Install (VSIX)

1. Install VSCE (VS Code Extension Manager) globally:
   ```bash
   npm install -g @vscode/vsce
   ```
2. Navigate to the extension directory:
   ```bash
   cd vscode-np
   ```
3. Package the extension:
   ```bash
   vsce package
   ```
4. Install the generated `.vsix` file in VS Code (`Extensions View` -> `...` menu -> `Install from VSIX...`).
