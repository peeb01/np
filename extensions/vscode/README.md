# np Language Support for VS Code

This extension adds syntax highlighting and basic language configuration support for the `np` programming language.

## Features

- **Syntax Highlighting**: Supports keywords (`fn`, `if`, `elif`, `else`, `for`, `while`, `return`, etc.), logical operators (`and`, `or`, `not`), numbers, string literals, variables, and function definitions.
- **Auto-Closing Brackets**: Auto-closes `{ }`, `[ ]`, `( )`, and quotation marks.
- **Comments Toggle**: Use `Ctrl + /` (Windows/Linux) or `Cmd + /` (macOS) to toggle line comments (`#`).
- **Indentation Rules**: Automatically handles Python-style colon `:` indent increases and decreases.

## Installation

### Method 1: Auto-Setup Scripts (Recommended)

Run the script inside this directory based on your OS:
- **Windows**: Double-click `setup.bat` (or run it in Command Prompt/PowerShell)
- **macOS / Linux / WSL**: Run `bash setup.sh` in your terminal

### Method 2: Manual Local Installation

1. Copy the `extensions/vscode` folder directly into your VS Code extensions folder:
   - **Windows**: Copy to `%USERPROFILE%\.vscode\extensions\vscode-np`
   - **macOS / Linux**: Copy to `~/.vscode/extensions/vscode-np`
2. Restart or reload your VS Code editor.

### Method 3: Package and Install (VSIX)

1. Install VSCE (VS Code Extension Manager) globally:
   ```bash
   npm install -g @vscode/vsce
   ```
2. Navigate to the extension directory:
   ```bash
   cd extensions/vscode
   ```
3. Package the extension:
   ```bash
   vsce package
   ```
4. Install the generated `.vsix` file in VS Code (`Extensions View` -> `...` menu -> `Install from VSIX...`).
