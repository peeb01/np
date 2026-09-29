#!/bin/bash

# Target extensions directory
TARGET_DIR="$HOME/.vscode/extensions/vscode-np"

echo "Installing np language support for VS Code..."

# Create target directory if it doesn't exist
mkdir -p "$TARGET_DIR"

# Get current script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

# Copy all files recursively
cp -R "$SCRIPT_DIR"/* "$TARGET_DIR/"

echo ""
echo "Success! np-lang support extension installed globally."
echo "Please restart or reload your VS Code window to apply changes."
