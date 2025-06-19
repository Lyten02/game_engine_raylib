# GameEngine Tests

## 🚀 Запуск всех тестов

### Способ 1: Из директории build
```bash
cd GameEngine/build
make test
```

### Способ 2: Напрямую через Python
```bash
cd GameEngine/build
python3 ../tests/run_all_tests.py
```

### Способ 3: Из корня проекта
```bash
cd GameEngine
python3 tests/run_all_tests.py
```

## 🧪 Типы тестов

### 1. Python тесты (test_*.py)
- `test_cli_basic.py` - базовая функциональность CLI
- `test_projects.py` - управление проектами

### 2. Скриптовые тесты (*.txt)
- `basic_cli_test.txt` - основные команды
- `test_entities.txt` - работа с сущностями
- `test_scene_management.txt` - управление сценами

### 3. Командные тесты
Встроены в `run_all_tests.py` - тестируют отдельные команды

### 4. C++ тесты для ResourceManager
- `test_resource_manager_safety.cpp` - тестирование безопасности указателей
- `test_resource_manager_threading.cpp` - многопоточная безопасность
- `test_resource_manager_headless.cpp` - работа в headless режиме

## 📝 Запуск отдельных тестов

### Python тест
```bash
cd build
python3 ../tests/test_cli_basic.py
```

### Скриптовый тест
```bash
cd build
./game --json --script ../tests/basic_cli_test.txt
```

### Одиночная команда
```bash
cd build
./game --json --command "help"
```

### C++ тесты ResourceManager
```bash
cd tests
./compile_and_run_tests.sh  # Компилирует, запускает и очищает
```

Или вручную:
```bash
g++ -std=c++20 test_resource_manager_safety.cpp ../src/resources/resource_manager.cpp \
    -I../src -I../build/_deps/raylib-src/src -I../build/_deps/spdlog-src/include \
    -I../build/_deps/entt-src/src -I../build/_deps/glm-src \
    -L../build -L../build/_deps/raylib-build/raylib -lraylib \
    -framework OpenGL -framework Cocoa -framework IOKit \
    -framework CoreVideo -framework CoreFoundation \
    -o test_resource_manager_safety
./test_resource_manager_safety
```

## 🧹 Очистка тестовых данных

```bash
python3 tests/clean_test_data.py          # Очистка тестовых проектов
./tests/clean_test_executables.sh  # Очистка скомпилированных тестов
```

- `clean_test_data.py` - удаляет все тестовые проекты (начинающиеся с `test_`, `cli_test`, etc.)
- `clean_test_executables.sh` - удаляет скомпилированные исполняемые файлы тестов

## 📊 Результаты тестов и логирование

### Выходные файлы

После запуска тестов создаются следующие файлы:

1. **`test_log_YYYYMMDD_HHMMSS.log`** - подробный лог выполнения всех тестов
   - Полная информация о каждом тесте (старт, финиш, статус)
   - Детальный вывод stdout/stderr для упавших тестов
   - Трассировка стека для исключений
   - Таблица времени выполнения всех тестов

2. **`test_results.json`** - структурированные результаты тестов
   - Количество пройденных/проваленных тестов
   - Время выполнения каждого теста
   - Сообщения об ошибках

### Опции логирования

```bash
# Стандартный запуск с логированием
python3 tests/run_all_tests.py

# Verbose режим - показывает ошибки в реальном времени
python3 tests/run_all_tests.py --verbose

# Быстрый режим (пропускает полные сборки)
python3 tests/run_all_tests.py --skip-full-build
```

### Пример лог-файла

```
GameEngine Test Suite Execution Log
================================================================================
Start Time: 2025-06-11 22:13:59
Python Version: 3.13.1
Platform: darwin
Working Directory: /path/to/build
Skip Full Build: False
Verbose Mode: False
================================================================================

[2025-06-11 22:13:59.058] [INFO    ] ============================================================
[2025-06-11 22:13:59.058] [INFO    ] TEST START: test_categories.py (1/43)
[2025-06-11 22:13:59.058] [INFO    ] Type: Python Test
[2025-06-11 22:13:59.058] [INFO    ] File: ../tests/test_categories.py
[2025-06-11 22:13:59.058] [INFO    ] ============================================================
[2025-06-11 22:13:59.083] [SUCCESS ] TEST PASSED: test_categories.py
[2025-06-11 22:13:59.083] [INFO    ] Duration: 0.02 seconds
```

### Анализ логов

```bash
# Найти все упавшие тесты
grep "TEST FAILED" test_log_*.log

# Посмотреть полную ошибку конкретного теста
grep -A 50 "TEST FAILED: test_name.py" test_log_*.log

# Найти самые медленные тесты
grep -E "Duration: [0-9.]+ seconds" test_log_*.log | sort -k3 -n -r | head -10

# Посмотреть все таймауты
grep "TEST TIMEOUT" test_log_*.log
```

## ✍️ Создание новых тестов

### Python тест
1. Создайте файл `test_feature.py` в директории `tests/`
2. Используйте функцию `run_command()` для выполнения команд
3. Используйте `assert` для проверок
4. Возвращайте код 0 при успехе, 1 при ошибке

### Скриптовый тест
1. Создайте файл `test_feature.txt` в директории `tests/`
2. Пишите команды построчно
3. Используйте `#` для комментариев
4. Тест считается успешным если все команды выполнились

## 🔧 Интеграция с CI/CD

Тесты возвращают правильные exit коды:
- 0 - все тесты прошли
- 1 - есть проваленные тесты

Пример для GitHub Actions:
```yaml
- name: Build
  run: |
    mkdir build && cd build
    cmake ..
    make

- name: Run tests
  run: |
    cd build
    make test
```

## 📈 Покрытие тестами

Текущее покрытие:
- ✅ CLI аргументы и опции
- ✅ Управление проектами
- ✅ Создание сущностей
- ✅ Управление сценами
- ✅ Batch выполнение
- ✅ Скриптовое выполнение
- ⚠️ Сериализация (TODO)
- ⚠️ Система сборки (TODO)
- ⚠️ Play mode (TODO)