#include "console.h"
#include "command_processor.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <sstream>

void Console::initialize() {
    consoleFont = GetFontDefault();
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
    }
    
    // Handle backspace with repeat
    if (IsKeyDown(KEY_BACKSPACE) && !currentInput.empty()) {
        if (IsKeyPressed(KEY_BACKSPACE)) {
            // Initial press
            currentInput.pop_back();
            backspaceTimer = 0.0f;
        } else {
            // Key is held down
            backspaceTimer += deltaTime;
            if (backspaceTimer > backspaceDelay) {
                // Start repeating
                float repeatTime = backspaceTimer - backspaceDelay;
                int repeats = static_cast<int>(repeatTime / backspaceRepeat);
                if (repeats > 0) {
                    currentInput.pop_back();
                    backspaceTimer = backspaceDelay + (repeatTime - repeats * backspaceRepeat);
                }
            }
        }
    } else {
        backspaceTimer = 0.0f;
    }
    
    // Handle history navigation
    if (IsKeyPressed(KEY_UP) && !commandHistory.empty()) {
        if (historyIndex == -1) {
            historyIndex = commandHistory.size() - 1;
        } else if (historyIndex > 0) {
            historyIndex--;
        }
        currentInput = commandHistory[historyIndex];
    }
    
    if (IsKeyPressed(KEY_DOWN) && historyIndex != -1) {
        if (historyIndex < static_cast<int>(commandHistory.size()) - 1) {
            historyIndex++;
            currentInput = commandHistory[historyIndex];
        } else {
            historyIndex = -1;
            currentInput.clear();
        }
    }
    
    // Handle text input
    int key = GetCharPressed();
    while (key > 0) {
        if (key >= 32 && key <= 126) {
            currentInput += static_cast<char>(key);
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
        int cursorX = 10 + MeasureText(inputLine.c_str(), fontSize);
        DrawRectangle(cursorX, consoleHeight - 30, 2, fontSize, inputColor);
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