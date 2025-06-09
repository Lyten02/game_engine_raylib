#!/bin/bash

# full_test_smart.sh - Умный тестовый скрипт с проверкой существования game
# Использование: ./full_test_smart.sh [--force-rebuild]

# Определить абсолютный путь к проекту в начале
PROJECT_ROOT=$(pwd)
if [[ ! -f "CMakeLists.txt" ]]; then
    if [[ -f "../CMakeLists.txt" ]]; then
        PROJECT_ROOT=$(cd .. && pwd)
    else
        echo "❌ ERROR: Cannot find CMakeLists.txt"
        exit 1
    fi
fi

echo "🚀 Проект найден в: $PROJECT_ROOT"

# Создать директорию для логов в корне проекта
cd "$PROJECT_ROOT"
mkdir -p logs

# Имя файла лога с АБСОЛЮТНЫМ путем
TIMESTAMP=$(date '+%Y%m%d_%H%M%S')
LOG_FILE="$PROJECT_ROOT/logs/full_test_${TIMESTAMP}.log"

# Создать лог файл
echo "🚀 Starting smart test cycle at $(date)" > "$LOG_FILE"
echo "✅ Log file created: $LOG_FILE"

# Функция для логирования
log() {
    local message="[$(date '+%Y-%m-%d %H:%M:%S')] $1"
    echo "$message"
    echo "$message" >> "$LOG_FILE"
}

# Функция для запуска команд
run_command() {
    local cmd="$1"
    local description="$2"
    
    log "========================================"
    log "Starting: $description"
    log "Command: $cmd"
    log "========================================"
    
    if eval "$cmd" 2>&1 | tee -a "$LOG_FILE"; then
        log "✅ SUCCESS: $description completed"
        return 0
    else
        log "❌ FAILED: $description failed"
        return 1
    fi
}

# Функция проверки состояния сборки
check_build_status() {
    log "🔍 Checking build status..."
    
    # Проверить наличие build директории
    if [ ! -d "$PROJECT_ROOT/build" ]; then
        log "❌ Build directory not found"
        return 1
    fi
    
    cd "$PROJECT_ROOT/build"
    
    # Проверить наличие исполняемого файла game
    if [ -f "game" ] && [ -x "game" ]; then
        log "✅ Executable 'game' found and is executable"
        
        # Проверить когда был собран game
        local game_time=$(stat -f "%m" "game" 2>/dev/null || stat -c "%Y" "game" 2>/dev/null || echo "0")
        local cmake_time=0
        local src_time=0
        
        # Проверить время изменения CMakeLists.txt
        if [ -f "../CMakeLists.txt" ]; then
            cmake_time=$(stat -f "%m" "../CMakeLists.txt" 2>/dev/null || stat -c "%Y" "../CMakeLists.txt" 2>/dev/null || echo "0")
        fi
        
        # Проверить время изменения исходников (найти самый новый .cpp/.h файл)
        if command -v find >/dev/null 2>&1; then
            local newest_src=$(find ../src -name "*.cpp" -o -name "*.h" 2>/dev/null | head -1)
            if [ -n "$newest_src" ]; then
                src_time=$(stat -f "%m" "$newest_src" 2>/dev/null || stat -c "%Y" "$newest_src" 2>/dev/null || echo "0")
            fi
        fi
        
        # Если game новее чем исходники и CMakeLists.txt, то пересборка не нужна
        if [ "$game_time" -gt "$cmake_time" ] && [ "$game_time" -gt "$src_time" ]; then
            log "✅ Executable 'game' is up-to-date"
            return 0
        else
            log "⚠️  Executable 'game' exists but may be outdated"
            log "   game time: $(date -r $game_time 2>/dev/null || echo 'unknown')"
            log "   cmake time: $(date -r $cmake_time 2>/dev/null || echo 'unknown')"
            log "   source time: $(date -r $src_time 2>/dev/null || echo 'unknown')"
            return 2  # Нужна пересборка
        fi
    else
        log "❌ Executable 'game' not found or not executable"
        return 1
    fi
}

# Главная функция
main() {
    log "🚀 Starting smart test cycle"
    log "Project root: $PROJECT_ROOT"
    log "Git branch: $(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo 'unknown')"
    log "Command line args: $*"
    
    # Проверить аргументы командной строки
    local force_rebuild=false
    if [[ "$1" == "--force-rebuild" || "$1" == "-f" ]]; then
        force_rebuild=true
        log "🔨 Force rebuild requested"
    fi
    
    # Перейти в build директорию (создать если не существует)
    if [ ! -d "$PROJECT_ROOT/build" ]; then
        log "Creating build directory..."
        mkdir -p "$PROJECT_ROOT/build"
    fi
    
    cd "$PROJECT_ROOT/build"
    log "✅ Changed to build directory: $(pwd)"
    
    local total_tests=3
    local passed_tests=0
    local failed_tests=0
    local skip_build=false
    
    # 1. Проверка необходимости сборки
    log "🔍 [1/$total_tests] Checking build requirements..."
    
    if [ "$force_rebuild" = true ]; then
        log "🔨 Force rebuild mode - skipping build check"
    else
        local build_status
        check_build_status
        build_status=$?
        
        case $build_status in
            0)
                log "✅ Build is up-to-date, skipping rebuild"
                skip_build=true
                ((passed_tests++))
                ;;
            2)
                log "⚠️  Build exists but outdated, rebuilding..."
                ;;
            *)
                log "❌ Build required"
                ;;
        esac
    fi
    
    # Выполнить сборку если нужно
    if [ "$skip_build" = false ]; then
        log "🔨 Building project..."
        if run_command "cmake .. && make -j8" "CMake configure and build"; then
            ((passed_tests++))
            log "✅ Build completed successfully"
        else
            ((failed_tests++))
            log "❌ Build failed, but continuing with tests..."
        fi
    fi
    
    # Проверить что game действительно создался/существует
    if [ ! -f "game" ] || [ ! -x "game" ]; then
        log "❌ ERROR: Executable 'game' still not found after build!"
        log "Build directory contents:"
        ls -la >> "$LOG_FILE"
        ((failed_tests++))
    else
        log "✅ Executable 'game' confirmed present and executable"
        log "   Size: $(ls -lh game | awk '{print $5}')"
        log "   Modified: $(ls -l game | awk '{print $6, $7, $8}')"
    fi
    
    # 2. Python tests
    log "🐍 [2/$total_tests] Running Python tests..."
    if make help 2>/dev/null | grep -q "test "; then
        if run_command "make test" "Python tests"; then
            ((passed_tests++))
        else
            ((failed_tests++))
        fi
    else
        log "❌ 'make test' target not available"
        # Попробовать запустить Python тесты напрямую
        if [ -f "../tests/run_all_tests.py" ]; then
            log "🔄 Trying to run Python tests directly..."
            if run_command "python3 ../tests/run_all_tests.py" "Direct Python tests"; then
                ((passed_tests++))
            else
                ((failed_tests++))
            fi
        else
            ((failed_tests++))
        fi
    fi
    
    # 3. C++ tests
    log "⚙️ [3/$total_tests] Running C++ tests..."
    if make help 2>/dev/null | grep -q "test-cpp"; then
        if run_command "make test-cpp" "C++ tests"; then
            ((passed_tests++))
        else
            ((failed_tests++))
        fi
    else
        log "❌ 'make test-cpp' target not available"
        # Попробовать найти и запустить C++ тесты напрямую
        if [ -f "../tests/compile_and_run_tests.sh" ]; then
            log "🔄 Trying to run C++ tests directly..."
            if run_command "cd ../tests && ./compile_and_run_tests.sh" "Direct C++ tests"; then
                ((passed_tests++))
            else
                ((failed_tests++))
            fi
        else
            ((failed_tests++))
        fi
    fi
    
    # Результаты
    log "========================================"
    log "📊 FINAL RESULTS:"
    log "✅ Passed: $passed_tests/$total_tests"
    log "❌ Failed: $failed_tests/$total_tests"
    
    if [ "$skip_build" = true ]; then
        log "⚡ Build was skipped (executable up-to-date)"
    fi
    
    # Показать информацию о game
    if [ -f "game" ]; then
        log "🎮 Game executable info:"
        log "   Path: $(pwd)/game"
        log "   Size: $(ls -lh game | awk '{print $5}')"
        log "   Last modified: $(ls -l game | awk '{print $6, $7, $8}')"
        log "   Permissions: $(ls -l game | awk '{print $1}')"
    fi
    
    echo ""
    echo "📊 SUMMARY:"
    echo "=================="
    echo "✅ Passed: $passed_tests/$total_tests"
    echo "❌ Failed: $failed_tests/$total_tests"
    if [ "$skip_build" = true ]; then
        echo "⚡ Build skipped (up-to-date)"
    fi
    echo "📄 Full log: $LOG_FILE"
    echo "🕒 Finished: $(date '+%Y-%m-%d %H:%M:%S')"
    
    if [ -f "game" ]; then
        echo "🎮 Game ready: $(pwd)/game"
        echo ""
        echo "💡 Quick commands:"
        echo "   ./game                    # Run the engine"
        echo "   ./game --help            # Show help"
        echo "   ./game --json -c help    # JSON output"
    fi
    
    return $failed_tests
}

# Показать справку
if [[ "$1" == "--help" || "$1" == "-h" ]]; then
    echo "Usage: $0 [options]"
    echo ""
    echo "Smart test runner that skips rebuild if 'game' executable is up-to-date."
    echo ""
    echo "This script:"
    echo "1. Checks if build/game exists and is newer than sources"
    echo "2. Skips rebuild if game is up-to-date (saves time!)"
    echo "3. Runs Python tests (make test or direct)"
    echo "4. Runs C++ tests (make test-cpp or direct)"
    echo ""
    echo "Options:"
    echo "  --help, -h           Show this help message"
    echo "  --force-rebuild, -f  Force rebuild even if game exists"
    echo ""
    echo "Smart features:"
    echo "  ⚡ Skips rebuild if build/game is newer than sources"
    echo "  🔍 Checks file timestamps to determine if rebuild needed"
    echo "  📋 Falls back to direct test execution if make targets missing"
    echo "  📄 Detailed logging with absolute paths"
    echo ""
    echo "Examples:"
    echo "  $0                    # Smart test (skip rebuild if not needed)"
    echo "  $0 --force-rebuild   # Force full rebuild and test"
    echo ""
    exit 0
fi

# Запуск
echo "🚀 Initializing smart test runner..."
echo "💡 Use --help for options, --force-rebuild to force rebuild"
echo ""

main "$@"
exit_code=$?

# Финальные проверки
if [ -f "$LOG_FILE" ]; then
    log_lines=$(wc -l < "$LOG_FILE" 2>/dev/null || echo "0")
    echo "📋 Log created: $log_lines lines in $LOG_FILE"
    
    if [ $exit_code -ne 0 ]; then
        echo ""
        echo "❌ Issues found. Quick debug:"
        echo "   🔍 Check errors: grep -A3 -B1 'FAILED\\|ERROR' \"$LOG_FILE\""
        echo "   📄 Full log: $LOG_FILE"
    fi
else
    echo "⚠️  Warning: Log file not created"
fi

exit $exit_code