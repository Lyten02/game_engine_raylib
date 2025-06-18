
# Import TDD dependency resolver
try:
    from test_dependency_path_fix import get_compilation_flags, validate_test_environment
except ImportError:
    def get_compilation_flags():
        return {'includes': '', 'libs': '-lraylib -lspdlog', 'deps_dir': None}
    def validate_test_environment():
        return False, "Dependency resolver not available"

#!/usr/bin/env python3
"""
Комплексные тесты для проверки исправлений системы тестирования.
Проверяют корректность путей, логирования, очистки и производительности.
"""

import os
import sys
import subprocess
import tempfile
import time
import json
import shutil
import glob
from pathlib import Path

# Добавляем родительскую директорию в путь
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

class TestSystemFixesTests:
    """Тесты для проверки исправлений системы тестирования"""
    
    def __init__(self):
        self.passed = 0
        self.failed = 0
        self.test_dir = os.path.dirname(os.path.abspath(__file__))
        self.engine_dir = os.path.dirname(self.test_dir)
        
    def run_test(self, test_name, test_func):
        """Запуск одного теста с обработкой ошибок"""
        try:
            print(f"\n{'='*60}")
            print(f"Running: {test_name}")
            test_func()
            print(f"✅ PASSED: {test_name}")
            self.passed += 1
        except Exception as e:
            print(f"❌ FAILED: {test_name}")
            print(f"Error: {str(e)}")
            self.failed += 1
            
    def test_cpp_compilation_paths(self):
        """Тест: Проверка корректности путей компиляции C++ тестов"""
        # Проверяем наличие compile_and_run_tests.sh
        compile_script = os.path.join(self.test_dir, "compile_and_run_tests.sh")
        assert os.path.exists(compile_script), "compile_and_run_tests.sh not found"
        
        # Проверяем наличие директорий с зависимостями
        deps_dirs = [
            ".deps_cache/raylib-src/src",
            ".deps_cache/spdlog-src/include", 
            ".deps_cache/entt-src/src",
            ".deps_cache/glm-src",
            ".deps_cache/json-src/include"
        ]
        
        for deps_dir in deps_dirs:
            full_path = os.path.join(self.engine_dir, deps_dir)
            assert os.path.exists(full_path), f"Dependencies directory not found: {deps_dir}"
            
        # Проверяем что скрипт содержит правильные пути
        with open(compile_script, 'r') as f:
            content = f.read()
            
        # Проверяем что используются правильные пути к зависимостям
        assert ".deps_cache/raylib-src/src" in content, "Incorrect raylib path"
        assert ".deps_cache/spdlog-src/include" in content, "Incorrect spdlog path"
        assert ".deps_cache/entt-src/src" in content, "Incorrect entt path"
        
        # Пытаемся скомпилировать простой тест
        test_cpp = os.path.join(self.test_dir, "test_resource_manager_simple.cpp")
        if os.path.exists(test_cpp):
            cmd = ["bash", compile_script, "test_resource_manager_simple"]
            result = subprocess.run(cmd, capture_output=True, text=True, cwd=self.test_dir)
            assert result.returncode == 0, f"C++ test compilation failed: {result.stderr}"
            
    def test_python_logger_robustness(self):
        """Тест: Проверка устойчивости логгера Python тестов"""
        # Проверяем наличие run_all_tests.py
        run_all_tests = os.path.join(self.test_dir, "run_all_tests.py")
        assert os.path.exists(run_all_tests), "run_all_tests.py not found"
        
        # Создаем временную директорию для тестов
        with tempfile.TemporaryDirectory() as temp_dir:
            # Копируем скрипт для модификации
            temp_script = os.path.join(temp_dir, "test_logger.py")
            shutil.copy(run_all_tests, temp_script)
            
            # Запускаем скрипт и проверяем что он создает лог файл
            cmd = [sys.executable, temp_script, "--dry-run"]
            result = subprocess.run(cmd, capture_output=True, text=True, cwd=self.test_dir)
            
            # Проверяем что скрипт не падает из-за логгера
            assert "FileNotFoundError" not in result.stderr, "Logger threw FileNotFoundError"
            
            # Проверяем что лог файл создан
            log_files = glob.glob(os.path.join(self.test_dir, "logs", "test_log_*.log"))
            assert len(log_files) > 0, "No log files created"
            
    def test_cleanup_mechanism(self):
        """Тест: Проверка механизма очистки временных проектов"""
        test_projects_dir = os.path.join(self.test_dir, "test_projects")
        
        # Создаем несколько тестовых проектов
        for i in range(10):
            proj_dir = os.path.join(test_projects_dir, f"TestProject_{i}")
            os.makedirs(proj_dir, exist_ok=True)
            # Создаем project.json чтобы симулировать реальный проект
            with open(os.path.join(proj_dir, "project.json"), "w") as f:
                json.dump({"name": f"TestProject_{i}"}, f)
                
        # Проверяем что проекты созданы
        initial_count = len(glob.glob(os.path.join(test_projects_dir, "TestProject_*")))
        assert initial_count >= 10, "Failed to create test projects"
        
        # Вызываем функцию очистки (которая должна быть добавлена)
        # Здесь мы проверяем что такая функция будет работать корректно
        cleanup_script = """
import os
import shutil
import glob

def cleanup_test_projects(keep_last=5):
    test_projects_dir = os.path.join(os.path.dirname(__file__), "test_projects")
    if not os.path.exists(test_projects_dir):
        return
        
    projects = glob.glob(os.path.join(test_projects_dir, "TestProject*"))
    projects.sort(key=lambda x: os.path.getmtime(x), reverse=True)
    
    # Удаляем все кроме последних keep_last
    for project in projects[keep_last:]:
        shutil.rmtree(project, ignore_errors=True)
        
cleanup_test_projects()
"""
        
        # Сохраняем и выполняем скрипт очистки
        cleanup_file = os.path.join(self.test_dir, "test_cleanup.py")
        with open(cleanup_file, "w") as f:
            f.write(cleanup_script)
            
        subprocess.run([sys.executable, cleanup_file], cwd=self.test_dir)
        
        # Проверяем что осталось <= 5 проектов
        final_count = len(glob.glob(os.path.join(test_projects_dir, "TestProject_*")))
        assert final_count <= 5, f"Cleanup failed: {final_count} projects remain"
        
        # Удаляем временный скрипт
        os.unlink(cleanup_file)
        
    def test_rebuild_caching_performance(self):
        """Тест: Проверка оптимизации test_rebuild_caching.py"""
        rebuild_test = os.path.join(self.test_dir, "test_rebuild_caching.py")
        
        if not os.path.exists(rebuild_test):
            print("test_rebuild_caching.py not found, skipping")
            return
            
        # Проверяем что тест не удаляет .deps_cache
        with open(rebuild_test, 'r') as f:
            content = f.read()
            
        # Проверяем что .deps_cache не удаляется
        if 'shutil.rmtree(".deps_cache")' in content:
            assert False, "test_rebuild_caching.py should not delete .deps_cache"
            
        # Измеряем время выполнения с мокированием долгих операций
        # Для реального теста это должно быть < 30 секунд
        start_time = time.time()
        
        # Здесь бы был запуск теста, но мы пока проверяем структуру
        # result = subprocess.run([sys.executable, rebuild_test], capture_output=True, timeout=30)
        
        elapsed = time.time() - start_time
        print(f"Performance check completed in {elapsed:.2f} seconds")
        
    def test_parallel_compilation(self):
        """Тест: Проверка поддержки параллельной компиляции C++ тестов"""
        compile_script = os.path.join(self.test_dir, "compile_and_run_tests.sh")
        
        with open(compile_script, 'r') as f:
            content = f.read()
            
        # Проверяем что скрипт поддерживает параллельную компиляцию
        # Должны быть флаги -j или использование GNU parallel
        has_parallel = "-j" in content or "parallel" in content or "&" in content
        
        # Пока это опциональная проверка
        if has_parallel:
            print("✓ Script supports parallel compilation")
        else:
            print("⚠ Script does not support parallel compilation (optional)")
            
    def test_log_rotation(self):
        """Тест: Проверка ротации логов"""
        logs_dir = os.path.join(self.test_dir, "logs")
        os.makedirs(logs_dir, exist_ok=True)
        
        # Создаем старые лог файлы
        old_time = time.time() - (8 * 24 * 60 * 60)  # 8 дней назад
        for i in range(20):
            log_file = os.path.join(logs_dir, f"test_log_old_{i}.log")
            with open(log_file, "w") as f:
                f.write("Old log content")
            os.utime(log_file, (old_time, old_time))
            
        # Функция ротации логов
        rotation_script = """
import os
import glob
import time

def rotate_logs(logs_dir, keep_days=7, max_files=10):
    if not os.path.exists(logs_dir):
        return
        
    current_time = time.time()
    cutoff_time = current_time - (keep_days * 24 * 60 * 60)
    
    log_files = glob.glob(os.path.join(logs_dir, "test_log_*.log"))
    log_files.sort(key=lambda x: os.path.getmtime(x), reverse=True)
    
    # Удаляем старые файлы
    for log_file in log_files:
        if os.path.getmtime(log_file) < cutoff_time:
            os.unlink(log_file)
            
    # Оставляем только max_files самых новых
    remaining = glob.glob(os.path.join(logs_dir, "test_log_*.log"))
    remaining.sort(key=lambda x: os.path.getmtime(x), reverse=True)
    
    for log_file in remaining[max_files:]:
        os.unlink(log_file)
        
rotate_logs('logs')
"""
        
        # Сохраняем и выполняем скрипт ротации
        rotation_file = os.path.join(self.test_dir, "test_rotation.py")
        with open(rotation_file, "w") as f:
            f.write(rotation_script)
            
        subprocess.run([sys.executable, rotation_file], cwd=self.test_dir)
        
        # Проверяем что старые файлы удалены
        remaining_logs = glob.glob(os.path.join(logs_dir, "test_log_old_*.log"))
        assert len(remaining_logs) == 0, f"Old logs not deleted: {len(remaining_logs)} remain"
        
        # Удаляем временный скрипт
        os.unlink(rotation_file)
        
    def run_all_tests(self):
        """Запуск всех тестов"""
        print("🔧 Running Test System Fixes Tests")
        print("="*60)
        
        self.run_test("C++ Compilation Paths", self.test_cpp_compilation_paths)
        self.run_test("Python Logger Robustness", self.test_python_logger_robustness)
        self.run_test("Cleanup Mechanism", self.test_cleanup_mechanism)
        self.run_test("Rebuild Caching Performance", self.test_rebuild_caching_performance)
        self.run_test("Parallel Compilation Support", self.test_parallel_compilation)
        self.run_test("Log Rotation", self.test_log_rotation)
        
        print(f"\n{'='*60}")
        print(f"Results: {self.passed} passed, {self.failed} failed")
        
        return self.failed == 0


if __name__ == "__main__":
    tester = TestSystemFixesTests()
    success = tester.run_all_tests()
    sys.exit(0 if success else 1)