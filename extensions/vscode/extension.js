const vscode = require('vscode');
const path = require('path');
const fs = require('fs');

async function activate(context) {
    const provider = {
        async provideDefinition(document, position, token) {
            const range = document.getWordRangeAtPosition(position);
            if (!range) return null;
            const word = document.getText(range);

            // Patterns we are looking for:
            // 1. Function definition: fn word
            // 2. Struct definition: struct word
            const defRegex = new RegExp(`^\\s*(fn|struct)\\s+${word}\\b`);

            // We will search the current document first (fastest)
            for (let lineNum = 0; lineNum < document.lineCount; lineNum++) {
                const lineText = document.lineAt(lineNum).text;
                if (defRegex.test(lineText)) {
                    return new vscode.Location(
                        document.uri,
                        new vscode.Position(lineNum, lineText.indexOf(word))
                    );
                }
            }

            // If not found in current file, search all .np files in the workspace
            const files = await vscode.workspace.findFiles('**/*.np');
            for (const fileUri of files) {
                // Skip the current document since we already searched it
                if (fileUri.toString() === document.uri.toString()) continue;

                try {
                    const content = await vscode.workspace.fs.readFile(fileUri);
                    const text = new TextDecoder('utf-8').decode(content);
                    const lines = text.split(/\r?\n/);
                    for (let lineNum = 0; lineNum < lines.length; lineNum++) {
                        if (defRegex.test(lines[lineNum])) {
                            return new vscode.Location(
                                fileUri,
                                new vscode.Position(lineNum, lines[lineNum].indexOf(word))
                            );
                        }
                    }
                } catch (e) {
                    // Ignore read errors
                }
            }

            // Also search inside .np_packages/ (which might be outside the active workspace folders but inside the root directory)
            const workspaceFolder = vscode.workspace.getWorkspaceFolder(document.uri);
            if (workspaceFolder) {
                const npPackagesPath = path.join(workspaceFolder.uri.fsPath, '.np_packages');
                if (fs.existsSync(npPackagesPath)) {
                    // Find all .np files in .np_packages recursively
                    const findNpFiles = (dir) => {
                        let results = [];
                        const list = fs.readdirSync(dir);
                        list.forEach((file) => {
                            const fullPath = path.join(dir, file);
                            const stat = fs.statSync(fullPath);
                            if (stat && stat.isDirectory()) {
                                results = results.concat(findNpFiles(fullPath));
                            } else if (file.endsWith('.np')) {
                                results.push(fullPath);
                            }
                        });
                        return results;
                    };

                    try {
                        const npFiles = findNpFiles(npPackagesPath);
                        for (const filePath of npFiles) {
                            const text = fs.readFileSync(filePath, 'utf-8');
                            const lines = text.split(/\r?\n/);
                            for (let lineNum = 0; lineNum < lines.length; lineNum++) {
                                if (defRegex.test(lines[lineNum])) {
                                    return new vscode.Location(
                                        vscode.Uri.file(filePath),
                                        new vscode.Position(lineNum, lines[lineNum].indexOf(word))
                                    );
                                }
                            }
                        }
                    } catch (e) {
                        // Ignore filesystem errors
                    }
                }
            }

            return null;
        }
    };

    context.subscriptions.push(vscode.languages.registerDefinitionProvider('np', provider));
    context.subscriptions.push(vscode.languages.registerDefinitionProvider('np-expected', provider));
}

function deactivate() {}

module.exports = {
    activate,
    deactivate
};
