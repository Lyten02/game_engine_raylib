#include "console.h"
#include "command_processor.h"
#include "utils/config.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <sstream>

void Console::initialize() {
    consoleFont = GetFontDefault();
    
    // Load settings from config
    if (Config::isConfigLoaded()) {
        fontSize = Config::getInt("console.font_size", 14);
        maxLines = Config::getInt("console.max_lines", 20);
        float alpha = Config::getFloat("console.background_alpha", 0.8f);
        backgroundColor.a = static_cast<unsigned char>(alpha * 255);
        
        // Update console height based on max lines
        consoleHeight = (maxLines * lineSpacing) + 50;
    }
    
    spdlog::info("Console::initialize - Developer console initialized");
}

void Console::shutdown() {
    outputLines.clear();
    commandHistory.clear();
    coloredOutput.clear();
    spdlog::info("Console::shutdown - Developer console shut down");
}

void Console::toggle() {
    isVisible = !isVisible;
    if (isVisible) {
        spdlog::debug("Console opened");
    } else {
        spdlog::debug("Console closed");
        currentInput.clear();
        historyIndex = -1;
    }
}

void Console::show() {
    isVisible = true;
}

void Console::hide() {
    isVisible = false;
    currentInput.clear();
    cursorPosition = 0;
    historyIndex = -1;
    scrollOffset = getMaxScroll();  // Reset to bottom
}

void Console::update(float deltaTime) {
    // Handle console toggle (check even when not visible)
    if (IsKeyPressed(KEY_F1)) {
        toggle();
        return;
    }
    
    if (!isVisible) return;
    
    // Handle input
    if (IsKeyPressed(KEY_ENTER) && !currentInput.empty()) {
        // Add to history
        commandHistory.push_back(currentInput);
        historyIndex = -1;
        
        // Add input line to output
        addLine("> " + currentInput, inputColor);
        
        // Execute command
        executeCommand(currentInput);
        
        // Clear input
        currentInput.clear();
        cursorPosition = 0;
    }
    
    // Handle backspace with repeat
    if (IsKeyDown(KEY_BACKSPACE) && !currentInput.empty() && cursorPosition > 0) {
        if (IsKeyPressed(KEY_BACKSPACE)) {
            // Initial press
            currentInput.erase(cursorPosition - 1, 1);
            cursorPosition--;
            backspaceTimer = 0.0f;
        } else {
            // Key is held down
            backspaceTimer += deltaTime;
            if (backspaceTimer > backspaceDelay) {
                // Start repeating
                float repeatTime = backspaceTimer - backspaceDelay;
                int repeats = static_cast<int>(repeatTime / backspaceRepeat);
                if (repeats > 0 && cursorPosition > 0) {
                    currentInput.erase(cursorPosition - 1, 1);
                    cursorPosition--;
                    backspaceTimer = backspaceDelay + (repeatTime - repeats * backspaceRepeat);
                }
            }
        }
    } else {
        backspaceTimer = 0.0f;
    }
    
    // Handle Delete key
    if (IsKeyPressed(KEY_DELETE) && cursorPosition < currentInput.length()) {
        currentInput.erase(cursorPosition, 1);
    }
    
    // Handle history navigation
    if (IsKeyPressed(KEY_UP) && !commandHistory.empty()) {
        if (historyIndex == -1) {
            historyIndex = commandHistory.size() - 1;
        } else if (historyIndex > 0) {
            historyIndex--;
        }
        currentInput = commandHistory[historyIndex];
        cursorPosition = currentInput.length();
    }
    
    if (IsKeyPressed(KEY_DOWN) && historyIndex != -1) {
        if (historyIndex < static_cast<int>(commandHistory.size()) - 1) {
            historyIndex++;
            currentInput = commandHistory[historyIndex];
        } else {
            historyIndex = -1;
            currentInput.clear();
            cursorPosition = 0;
        }
    }
    
    // Handle cursor movement with arrow keys
    if (IsKeyPressed(KEY_LEFT) && cursorPosition > 0) {
        cursorPosition--;
    }
    
    if (IsKeyPressed(KEY_RIGHT) && cursorPosition < currentInput.length()) {
        cursorPosition++;
    }
    
    // Handle HOME and END keys for cursor
    if (IsKeyPressed(KEY_HOME)) {
        cursorPosition = 0;
    }
    
    if (IsKeyPressed(KEY_END)) {
        cursorPosition = currentInput.length();
    }
    
    // Handle text input
    int key = GetCharPressed();
    while (key > 0) {
        if (key >= 32 && key <= 126) {
            currentInput.insert(cursorPosition, 1, static_cast<char>(key));
            cursorPosition++;
        }
        key = GetCharPressed();
    }
    
    // Handle scrolling
    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        scrollOffset -= static_cast<int>(wheel * 3);  // 3 lines per wheel notch
        scrollOffset = std::max(0, std::min(scrollOffset, getMaxScroll()));
    }
    
    // Handle Page Up/Down
    if (IsKeyPressed(KEY_PAGE_UP)) {
        scrollOffset -= 10;
        scrollOffset = std::max(0, scrollOffset);
    }
    
    if (IsKeyPressed(KEY_PAGE_DOWN)) {
        scrollOffset += 10;
        scrollOffset = std::min(scrollOffset, getMaxScroll());
    }
    
    // Handle Home/End
    if (IsKeyPressed(KEY_HOME)) {
        scrollOffset = 0;
    }
    
    if (IsKeyPressed(KEY_END)) {
        scrollOffset = getMaxScroll();
    }
    
    // Handle text selection
    Vector2 mousePos = GetMousePosition();
    
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && mousePos.y < consoleHeight - 40) {
        // Start selection
        isSelecting = true;
        selectionStart = mousePos;
        selectionEnd = mousePos;
        selectionStartLine = getLineAtPosition(mousePos.y);
        selectionEndLine = selectionStartLine;
    }
    
    if (isSelecting && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        // Update selection
        selectionEnd = mousePos;
        selectionEndLine = getLineAtPosition(mousePos.y);
    }
    
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && isSelecting) {
        // End selection
        isSelecting = false;
        selectedText = getSelectedText();
    }
    
    // Copy selection with Ctrl+C / Cmd+C
    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL) || 
         IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER)) && 
        IsKeyPressed(KEY_C) && !selectedText.empty()) {
        copyToClipboard(selectedText);
        addLine("Copied to clipboard", GRAY);
    }
}

void Console::render() {
    if (!isVisible) return;
    
    int screenWidth = GetScreenWidth();
    
    // Draw background
    DrawRectangle(0, 0, screenWidth, consoleHeight, backgroundColor);
    
    // Draw border
    DrawRectangle(0, consoleHeight - 2, screenWidth, 2, GRAY);
    
    // Calculate visible lines
    int visibleLines = (consoleHeight - 40) / lineSpacing;
    
    // Calculate start and end lines based on scroll
    int totalLines = static_cast<int>(coloredOutput.size());
    int startLine = scrollOffset;
    int endLine = std::min(startLine + visibleLines, totalLines);
    
    // Draw selection highlight
    if (!selectedText.empty() || isSelecting) {
        int minLine = std::min(selectionStartLine, selectionEndLine);
        int maxLine = std::max(selectionStartLine, selectionEndLine);
        
        for (int i = startLine; i < endLine; i++) {
            if (i >= minLine && i <= maxLine) {
                int lineY = 10 + (i - startLine) * lineSpacing;
                DrawRectangle(10, lineY, screenWidth - 40, lineSpacing, {100, 100, 255, 50});
            }
        }
    }
    
    // Draw output lines
    int y = 10;
    for (int i = startLine; i < endLine; i++) {
        DrawTextEx(consoleFont, coloredOutput[i].text.c_str(), 
                   {10, static_cast<float>(y)}, fontSize, 1, coloredOutput[i].color);
        y += lineSpacing;
    }
    
    // Draw scroll indicator if there are more lines than visible
    if (totalLines > visibleLines) {
        // Draw scrollbar background
        int scrollbarX = screenWidth - 20;
        int scrollbarHeight = consoleHeight - 50;
        DrawRectangle(scrollbarX, 10, 10, scrollbarHeight, {100, 100, 100, 100});
        
        // Calculate scrollbar position and size
        float scrollPercentage = static_cast<float>(scrollOffset) / std::max(1, getMaxScroll());
        int thumbHeight = std::max(20, scrollbarHeight * visibleLines / std::max(visibleLines, totalLines));
        int thumbY = 10 + static_cast<int>((scrollbarHeight - thumbHeight) * scrollPercentage);
        
        // Draw scrollbar thumb
        DrawRectangle(scrollbarX, thumbY, 10, thumbHeight, {200, 200, 200, 200});
        
        // Draw line count
        std::string lineInfo = std::to_string(startLine + 1) + "-" + std::to_string(endLine) + "/" + std::to_string(totalLines);
        DrawText(lineInfo.c_str(), screenWidth - 100, consoleHeight - 25, 12, GRAY);
    }
    
    // Draw input line
    std::string inputLine = "> " + currentInput;
    DrawTextEx(consoleFont, inputLine.c_str(), 
               {10, consoleHeight - 30}, fontSize, 1, inputColor);
    
    // Draw cursor
    if (static_cast<int>(GetTime() * 2) % 2 == 0) {
        std::string beforeCursor = "> " + currentInput.substr(0, cursorPosition);
        int cursorX = 10 + MeasureText(beforeCursor.c_str(), fontSize);
        DrawRectangle(cursorX, consoleHeight - 30, 2, fontSize, inputColor);
    }
    
    // Draw FPS in console header if enabled
    if (showFPS) {
        std::string fpsText = "FPS: " + std::to_string(GetFPS());
        int textWidth = MeasureText(fpsText.c_str(), fontSize);
        DrawText(fpsText.c_str(), screenWidth - textWidth - 120, 10, fontSize, GREEN);
    }
}

void Console::addLine(const std::string& text, Color color) {
    // Split text by newlines
    std::stringstream ss(text);
    std::string line;
    
    while (std::getline(ss, line)) {
        outputLines.push_back(line);
        coloredOutput.push_back({line, color});
        
        // Limit output history
        if (outputLines.size() > 100) {
            outputLines.erase(outputLines.begin());
            coloredOutput.erase(coloredOutput.begin());
            // Adjust scroll offset if needed
            if (scrollOffset > 0) {
                scrollOffset--;
            }
        }
    }
    
    // Auto-scroll to bottom when new line is added
    scrollOffset = getMaxScroll();
}

void Console::executeCommand(const std::string& command) {
    if (commandProcessor) {
        commandProcessor->executeCommand(command);
    } else {
        addLine("Error: Command processor not initialized", RED);
        spdlog::error("Console::executeCommand - CommandProcessor is null");
    }
}

bool Console::isOpen() const {
    return isVisible;
}

void Console::clear() {
    outputLines.clear();
    coloredOutput.clear();
    scrollOffset = 0;
    addLine("Console cleared", GRAY);
}

int Console::getMaxScroll() const {
    int visibleLines = (consoleHeight - 40) / lineSpacing;
    int totalLines = static_cast<int>(coloredOutput.size());
    return std::max(0, totalLines - visibleLines);
}

int Console::getLineAtPosition(float y) const {
    if (y < 10) return scrollOffset;
    int line = scrollOffset + static_cast<int>((y - 10) / lineSpacing);
    return std::min(line, static_cast<int>(coloredOutput.size()) - 1);
}

std::string Console::getSelectedText() const {
    if (selectionStartLine < 0 || selectionEndLine < 0) return "";
    
    int minLine = std::min(selectionStartLine, selectionEndLine);
    int maxLine = std::max(selectionStartLine, selectionEndLine);
    
    std::stringstream ss;
    for (int i = minLine; i <= maxLine && i < static_cast<int>(coloredOutput.size()); i++) {
        ss << coloredOutput[i].text;
        if (i < maxLine) ss << "\n";
    }
    
    return ss.str();
}

void Console::copyToClipboard(const std::string& text) {
    SetClipboardText(text.c_str());
}