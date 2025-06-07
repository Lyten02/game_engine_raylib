#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <raylib.h>
#include <nlohmann/json.hpp>

class CommandProcessor;

class Console {
private:
    std::vector<std::string> outputLines;
    std::string currentInput;
    size_t cursorPosition = 0;  // Position of cursor in currentInput
    std::vector<std::string> commandHistory;
    int historyIndex = -1;
    bool isVisible = false;
    Font consoleFont;
    int maxLines = 20;
    float consoleHeight = 300.0f;
    
    // Visual settings
    int fontSize = 16;
    int lineSpacing = 20;
    Color backgroundColor = {0, 0, 0, 200};
    Color textColor = WHITE;
    Color inputColor = GREEN;
    
    // Store colored output
    struct ColoredLine {
        std::string text;
        Color color;
    };
    std::vector<ColoredLine> coloredOutput;
    
    CommandProcessor* commandProcessor = nullptr;
    
    // Scrolling
    int scrollOffset = 0;
    int getMaxScroll() const;
    
    // Autocompletion
    std::vector<std::string> autocompleteSuggestions;
    int autocompleteIndex = -1;
    std::string autocompleteBase;
    std::string currentSuggestion;  // Current inline suggestion
    bool showSuggestionDropdown = false;
    int dropdownSelectedIndex = 0;
    const int maxDropdownItems = 8;  // Max items to show in dropdown
    void updateAutocompleteSuggestions();
    std::string getCommonPrefix(const std::vector<std::string>& suggestions) const;
    void updateInlineSuggestion();
    void showDropdown();
    void hideDropdown();
    std::string getCurrentCommandHint() const;
    void updateParameterSuggestions();
    std::vector<std::string> getParameterSuggestions(const std::string& command, int paramIndex) const;
    
    // Key repeat for backspace
    float backspaceTimer = 0.0f;
    const float backspaceDelay = 0.5f;  // Initial delay before repeat
    const float backspaceRepeat = 0.03f; // Repeat rate
    
    // FPS display
    bool showFPS = true;
    
    // Text selection
    bool isSelecting = false;
    Vector2 selectionStart = {0, 0};
    Vector2 selectionEnd = {0, 0};
    int selectionStartLine = -1;
    int selectionEndLine = -1;
    std::string selectedText;
    
    // Helper methods
    int getLineAtPosition(float y) const;
    std::string getSelectedText() const;
    void copyToClipboard(const std::string& text);
    
    // Capture mode
    bool captureMode = false;
    std::stringstream captureBuffer;
    
    // Command data for CLI mode
    nlohmann::json commandData;

public:
    Console() = default;
    ~Console() = default;
    
    void initialize();
    void shutdown();
    void toggle();
    void show();
    void hide();
    void update(float deltaTime);
    void render();
    void addLine(const std::string& text, Color color = WHITE);
    void executeCommand(const std::string& command);
    bool isOpen() const;
    void clear();
    void setCommandProcessor(CommandProcessor* processor) { commandProcessor = processor; }
    void setShowFPS(bool show) { showFPS = show; }
    bool isShowingFPS() const { return showFPS; }
    
    // Output capture mode
    void enableCapture();
    std::string disableCapture();
    bool isCaptureMode() const { return captureMode; }
    
    // Command data for CLI mode
    void setCommandData(const nlohmann::json& data);
    nlohmann::json getCommandData() const;
    void clearCommandData();
};