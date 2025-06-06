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
    
    // Handle dropdown navigation
    if (showSuggestionDropdown && !autocompleteSuggestions.empty()) {
        bool dropdownHandled = false;
        
        if (IsKeyPressed(KEY_DOWN)) {
            dropdownSelectedIndex = (dropdownSelectedIndex + 1) % autocompleteSuggestions.size();
            currentSuggestion = autocompleteSuggestions[dropdownSelectedIndex];
            dropdownHandled = true;
        } else if (IsKeyPressed(KEY_UP)) {
            dropdownSelectedIndex = (dropdownSelectedIndex - 1 + autocompleteSuggestions.size()) % autocompleteSuggestions.size();
            currentSuggestion = autocompleteSuggestions[dropdownSelectedIndex];
            dropdownHandled = true;
        } else if (IsKeyPressed(KEY_TAB) || (IsKeyPressed(KEY_ENTER) && dropdownSelectedIndex >= 0)) {
            // Accept selected suggestion
            std::vector<std::string> tokens = commandProcessor->parseCommand(currentInput);
            bool endsWithSpace = !currentInput.empty() && currentInput.back() == ' ';
            
            if (tokens.size() > 1 || (tokens.size() == 1 && endsWithSpace)) {
                // We're completing a parameter
                if (endsWithSpace) {
                    currentInput += autocompleteSuggestions[dropdownSelectedIndex];
                } else {
                    // Replace the last token
                    tokens[tokens.size() - 1] = autocompleteSuggestions[dropdownSelectedIndex];
                    currentInput = tokens[0];
                    for (size_t i = 1; i < tokens.size(); i++) {
                        currentInput += " " + tokens[i];
                    }
                }
            } else {
                // Completing a command
                currentInput = autocompleteSuggestions[dropdownSelectedIndex];
            }
            cursorPosition = currentInput.length();
            hideDropdown();
            updateInlineSuggestion();
            if (IsKeyPressed(KEY_ENTER)) {
                // If Enter was pressed, also execute the command
                goto handle_enter;  // Jump to enter handling
            }
            dropdownHandled = true;
        } else if (IsKeyPressed(KEY_ESCAPE)) {
            hideDropdown();
            dropdownHandled = true;
        }
        
        // If dropdown handled the input, skip normal input processing
        if (dropdownHandled) {
            return;
        }
    } else {
        // Handle Tab for autocompletion when dropdown is not shown
        if (IsKeyPressed(KEY_TAB)) {
            if (!currentSuggestion.empty()) {
                // Accept current inline suggestion
                currentInput = currentSuggestion;
                cursorPosition = currentInput.length();
                currentSuggestion.clear();
                updateInlineSuggestion();
            } else {
                // Show dropdown
                updateParameterSuggestions();
                if (autocompleteSuggestions.empty()) {
                    // No parameter suggestions, try command suggestions
                    updateAutocompleteSuggestions();
                }
                if (!autocompleteSuggestions.empty()) {
                    showDropdown();
                }
            }
        }
    }
    
    // Handle input
    // Handle ENTER key normally when dropdown is visible but not selecting
    if (IsKeyPressed(KEY_ENTER) && !currentInput.empty() && showSuggestionDropdown && !IsKeyDown(KEY_LEFT_SHIFT)) {
        // Just hide dropdown and execute command
        hideDropdown();
        currentSuggestion.clear();
        autocompleteSuggestions.clear();
        goto handle_enter;
    }
    
handle_enter:
    if (IsKeyPressed(KEY_ENTER) && !currentInput.empty()) {
        // Clear autocompletion and suggestion
        hideDropdown();
        currentSuggestion.clear();
        autocompleteSuggestions.clear();
        
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
            updateParameterSuggestions();
            updateInlineSuggestion();
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
                    updateParameterSuggestions();
                    updateInlineSuggestion();
                }
            }
        }
    } else {
        backspaceTimer = 0.0f;
    }
    
    // Handle Delete key
    if (IsKeyPressed(KEY_DELETE) && cursorPosition < currentInput.length()) {
        currentInput.erase(cursorPosition, 1);
        updateParameterSuggestions();
        updateInlineSuggestion();
    }
    
    // Handle history navigation
    if (IsKeyPressed(KEY_UP) && !commandHistory.empty() && !showSuggestionDropdown) {
        if (historyIndex == -1) {
            historyIndex = commandHistory.size() - 1;
        } else if (historyIndex > 0) {
            historyIndex--;
        }
        currentInput = commandHistory[historyIndex];
        cursorPosition = currentInput.length();
        currentSuggestion.clear();
        hideDropdown();
        updateInlineSuggestion();
    }
    
    if (IsKeyPressed(KEY_DOWN) && historyIndex != -1 && !showSuggestionDropdown) {
        if (historyIndex < static_cast<int>(commandHistory.size()) - 1) {
            historyIndex++;
            currentInput = commandHistory[historyIndex];
        } else {
            historyIndex = -1;
            currentInput.clear();
            cursorPosition = 0;
        }
        currentSuggestion.clear();
        hideDropdown();
        updateInlineSuggestion();
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
            // Update suggestions and maybe show dropdown
            updateParameterSuggestions();
            updateInlineSuggestion();
        }
        key = GetCharPressed();
    }
    
    // Accept suggestion with Right arrow at end of input
    if (IsKeyPressed(KEY_RIGHT) && cursorPosition == currentInput.length() && !currentSuggestion.empty() && !showSuggestionDropdown) {
        currentInput = currentSuggestion;
        cursorPosition = currentInput.length();
        currentSuggestion.clear();
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
    
    // Draw inline suggestion
    if (!currentSuggestion.empty() && cursorPosition == currentInput.length()) {
        std::string suggestionPart = currentSuggestion.substr(currentInput.length());
        std::string beforeSuggestion = "> " + currentInput;
        int suggestionX = 10 + MeasureText(beforeSuggestion.c_str(), fontSize);
        DrawTextEx(consoleFont, suggestionPart.c_str(),
                   {static_cast<float>(suggestionX), consoleHeight - 30}, 
                   fontSize, 1, {128, 128, 128, 128}); // Gray transparent
    }
    
    // Draw autocomplete hint
    if (currentInput.empty()) {
        DrawText("Press TAB to see suggestions", screenWidth - 220, consoleHeight - 25, 12, DARKGRAY);
    } else if (!currentSuggestion.empty() && !showSuggestionDropdown) {
        DrawText("TAB/→ to accept", screenWidth - 150, consoleHeight - 25, 12, DARKGRAY);
    } else if (showSuggestionDropdown) {
        DrawText("↑↓ navigate, TAB/ENTER accept", screenWidth - 250, consoleHeight - 25, 12, DARKGRAY);
    }
    
    // Draw suggestion dropdown
    if (showSuggestionDropdown && !autocompleteSuggestions.empty()) {
        static bool lastDropdownState = false;
        if (!lastDropdownState) {
            spdlog::debug("Rendering dropdown with {} items", autocompleteSuggestions.size());
            lastDropdownState = true;
        }
        
        int dropdownY = consoleHeight - 50;
        int itemHeight = 20;
        int visibleItems = std::min(static_cast<int>(autocompleteSuggestions.size()), maxDropdownItems);
        int dropdownHeight = visibleItems * itemHeight + 10;
        
        // Move dropdown up to fit on screen
        dropdownY -= dropdownHeight;
        
        // Calculate dropdown width based on longest suggestion
        int maxWidth = 200;
        for (const auto& suggestion : autocompleteSuggestions) {
            int textWidth = MeasureText(suggestion.c_str(), fontSize) + 20;
            maxWidth = std::max(maxWidth, textWidth);
        }
        
        // Draw dropdown background
        DrawRectangle(10, dropdownY, maxWidth, dropdownHeight, {30, 30, 30, 240});
        DrawRectangleLines(10, dropdownY, maxWidth, dropdownHeight, DARKGRAY);
        
        // Draw items
        int startIdx = 0;
        if (dropdownSelectedIndex >= maxDropdownItems) {
            startIdx = dropdownSelectedIndex - maxDropdownItems + 1;
        }
        
        for (int i = 0; i < visibleItems; i++) {
            int idx = startIdx + i;
            if (idx >= static_cast<int>(autocompleteSuggestions.size())) break;
            
            int itemY = dropdownY + 5 + i * itemHeight;
            
            // Highlight selected item
            if (idx == dropdownSelectedIndex) {
                DrawRectangle(12, itemY, maxWidth - 4, itemHeight - 2, {70, 70, 200, 200});
            }
            
            // Draw text
            Color textColor = (idx == dropdownSelectedIndex) ? WHITE : LIGHTGRAY;
            DrawText(autocompleteSuggestions[idx].c_str(), 15, itemY + 2, fontSize, textColor);
        }
        
        // Draw scroll indicators if needed
        if (startIdx > 0) {
            DrawText("▲", maxWidth - 15, dropdownY + 5, 12, GRAY);
        }
        if (startIdx + visibleItems < static_cast<int>(autocompleteSuggestions.size())) {
            DrawText("▼", maxWidth - 15, dropdownY + dropdownHeight - 15, 12, GRAY);
        }
    } else {
        // Reset dropdown state tracking
        static bool* lastDropdownState = nullptr;
        if (!lastDropdownState) {
            lastDropdownState = new bool(false);
        }
        *lastDropdownState = false;
    }
    
    // Draw cursor
    if (static_cast<int>(GetTime() * 2) % 2 == 0) {
        std::string beforeCursor = "> " + currentInput.substr(0, cursorPosition);
        int cursorX = 10 + MeasureText(beforeCursor.c_str(), fontSize);
        DrawRectangle(cursorX, consoleHeight - 30, 2, fontSize, inputColor);
    }
    
    // Draw command parameter hints
    std::string hint = getCurrentCommandHint();
    if (!hint.empty()) {
        // Draw hint above input line
        DrawText(hint.c_str(), 10, consoleHeight - 55, fontSize - 2, DARKGRAY);
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

void Console::updateAutocompleteSuggestions() {
    autocompleteSuggestions.clear();
    autocompleteBase = currentInput;
    
    if (!commandProcessor || currentInput.empty()) {
        spdlog::debug("updateAutocompleteSuggestions: No processor or empty input");
        return;
    }
    
    // Get all command names
    std::vector<std::string> allCommands = commandProcessor->getCommandNames();
    spdlog::debug("updateAutocompleteSuggestions: Got {} total commands", allCommands.size());
    
    // Filter commands that start with current input
    for (const auto& command : allCommands) {
        if (command.find(currentInput) == 0) {
            autocompleteSuggestions.push_back(command);
        }
    }
    
    // Sort suggestions
    std::sort(autocompleteSuggestions.begin(), autocompleteSuggestions.end());
    
    spdlog::debug("updateAutocompleteSuggestions: Found {} suggestions for '{}'", 
                  autocompleteSuggestions.size(), currentInput);
    
    if (!autocompleteSuggestions.empty()) {
        for (const auto& s : autocompleteSuggestions) {
            spdlog::debug("  - {}", s);
        }
    }
}

std::string Console::getCommonPrefix(const std::vector<std::string>& suggestions) const {
    if (suggestions.empty()) return "";
    if (suggestions.size() == 1) return suggestions[0];
    
    std::string prefix = suggestions[0];
    for (size_t i = 1; i < suggestions.size(); i++) {
        size_t j = 0;
        while (j < prefix.length() && j < suggestions[i].length() && 
               prefix[j] == suggestions[i][j]) {
            j++;
        }
        prefix = prefix.substr(0, j);
    }
    
    return prefix;
}

void Console::updateInlineSuggestion() {
    currentSuggestion.clear();
    
    if (!commandProcessor || currentInput.empty() || cursorPosition != currentInput.length()) {
        return;
    }
    
    // Don't show inline suggestion if dropdown is open
    if (showSuggestionDropdown) {
        return;
    }
    
    // Get all command names
    std::vector<std::string> allCommands = commandProcessor->getCommandNames();
    
    // Find best match
    std::string bestMatch;
    for (const auto& command : allCommands) {
        if (command.find(currentInput) == 0 && command != currentInput) {
            if (bestMatch.empty() || command.length() < bestMatch.length()) {
                bestMatch = command;
            }
        }
    }
    
    if (!bestMatch.empty()) {
        currentSuggestion = bestMatch;
    }
}

void Console::showDropdown() {
    if (!autocompleteSuggestions.empty()) {
        if (!showSuggestionDropdown) {
            spdlog::debug("Showing dropdown with {} suggestions", autocompleteSuggestions.size());
        }
        showSuggestionDropdown = true;
        dropdownSelectedIndex = 0;
        currentSuggestion = autocompleteSuggestions[0];
    }
}

void Console::hideDropdown() {
    if (showSuggestionDropdown) {
        spdlog::debug("Hiding dropdown");
    }
    showSuggestionDropdown = false;
    dropdownSelectedIndex = 0;
    // Don't clear suggestions here - let updateParameterSuggestions handle it
}

std::string Console::getCurrentCommandHint() const {
    if (!commandProcessor || currentInput.empty() || showSuggestionDropdown) {
        return "";
    }
    
    // Parse current input to get command name
    std::vector<std::string> tokens = commandProcessor->parseCommand(currentInput);
    if (tokens.empty()) {
        return "";
    }
    
    std::string commandName = tokens[0];
    
    // Check if this is a complete command
    auto commandNames = commandProcessor->getCommandNames();
    bool isValidCommand = std::find(commandNames.begin(), commandNames.end(), commandName) != commandNames.end();
    
    if (!isValidCommand) {
        return "";
    }
    
    // Get command info
    CommandInfo info = commandProcessor->getCommandInfo(commandName);
    
    // Build hint
    std::string hint;
    if (!info.syntax.empty()) {
        hint = info.syntax;
    } else if (!info.parameters.empty()) {
        hint = commandName;
        for (const auto& param : info.parameters) {
            hint += " ";
            if (param.required) {
                hint += "<" + param.name + ">";
            } else {
                hint += "[" + param.name + "]";
            }
        }
    }
    
    return hint;
}

void Console::updateParameterSuggestions() {
    static std::string lastLoggedInput;
    static int lastSuggestionCount = -1;
    
    autocompleteSuggestions.clear();
    
    if (!commandProcessor || currentInput.empty()) {
        return;
    }
    
    // Parse current input
    std::vector<std::string> tokens = commandProcessor->parseCommand(currentInput);
    if (tokens.empty()) {
        return;
    }
    
    std::string commandName = tokens[0];
    
    // Log input changes (avoid spam)
    if (currentInput != lastLoggedInput) {
        spdlog::debug("Console input: '{}', tokens: {}", currentInput, tokens.size());
        for (size_t i = 0; i < tokens.size(); i++) {
            spdlog::debug("  Token[{}]: '{}'", i, tokens[i]);
        }
        lastLoggedInput = currentInput;
    }
    
    // Check if we're typing the command name
    bool endsWithSpace = !currentInput.empty() && currentInput.back() == ' ';
    if (tokens.size() == 1 && !endsWithSpace) {
        // Get command suggestions
        spdlog::debug("Getting command suggestions for: '{}'", commandName);
        updateAutocompleteSuggestions();
        
        // Show dropdown if we have suggestions
        if (!autocompleteSuggestions.empty()) {
            spdlog::debug("Showing dropdown for command suggestions");
            showDropdown();
        }
        return;
    }
    
    // Check if this is a valid command
    auto commandNames = commandProcessor->getCommandNames();
    bool isValidCommand = std::find(commandNames.begin(), commandNames.end(), commandName) != commandNames.end();
    
    if (!isValidCommand) {
        spdlog::debug("Not a valid command: '{}'", commandName);
        return;
    }
    
    // We're typing a parameter
    int paramIndex = tokens.size() - 2;  // -1 for command, -1 for current typing
    if (endsWithSpace) {
        paramIndex++;  // We're starting a new parameter
    }
    
    spdlog::debug("Getting parameter suggestions for command '{}', param index: {}", commandName, paramIndex);
    
    // Get suggestions for this parameter
    auto suggestions = commandProcessor->getParameterSuggestions(commandName, paramIndex);
    
    if (!suggestions.empty()) {
        spdlog::debug("Got {} parameter suggestions", suggestions.size());
        
        // Filter suggestions based on what's already typed
        std::string currentParam;
        if (paramIndex < static_cast<int>(tokens.size()) - 1) {
            currentParam = tokens[paramIndex + 1];
        }
        
        spdlog::debug("Filtering with current param: '{}'", currentParam);
        
        for (const auto& suggestion : suggestions) {
            if (currentParam.empty() || suggestion.find(currentParam) == 0) {
                autocompleteSuggestions.push_back(suggestion);
            }
        }
        
        // Log suggestions only if count changed
        if (static_cast<int>(autocompleteSuggestions.size()) != lastSuggestionCount) {
            spdlog::debug("Filtered to {} suggestions:", autocompleteSuggestions.size());
            for (const auto& s : autocompleteSuggestions) {
                spdlog::debug("  - {}", s);
            }
            lastSuggestionCount = autocompleteSuggestions.size();
        }
        
        if (!autocompleteSuggestions.empty()) {
            showDropdown();
        } else {
            hideDropdown();
        }
    } else {
        // No parameter suggestions
        if (lastSuggestionCount != 0) {
            spdlog::debug("No parameter suggestions available");
            lastSuggestionCount = 0;
        }
        hideDropdown();
    }
}

std::vector<std::string> Console::getParameterSuggestions(const std::string& command, int paramIndex) const {
    if (!commandProcessor) {
        return {};
    }
    return commandProcessor->getParameterSuggestions(command, paramIndex);
}