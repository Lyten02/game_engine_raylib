import os
import subprocess
import time
import json

# Import TDD dependency resolver
try:
    from test_dependency_path_fix import get_compilation_flags, validate_test_environment
except ImportError:
    def get_compilation_flags():
        return {'includes': '', 'libs': '-lraylib -lspdlog', 'deps_dir': None}
    def validate_test_environment():
        return False, "Dependency resolver not available"


def find_executable():
    """Find the game_engine executable in the build directory"""
    build_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'build'))
    
    # Try different possible locations
    possible_paths = [
        os.path.join(build_dir, 'game_engine'),
        os.path.join(build_dir, 'game_engine.exe'),
        os.path.join(build_dir, 'Debug', 'game_engine.exe'),
        os.path.join(build_dir, 'Release', 'game_engine.exe'),
    ]
    
    for path in possible_paths:
        if os.path.exists(path):
            return os.path.abspath(path)
    
    return None

def build_engine_if_needed():
    """Build the engine if the executable doesn't exist"""
    exe = find_executable()
    if exe and os.path.exists(exe):
        return True
    
    build_dir = os.path.join(os.path.dirname(__file__), '..', 'build')
    
    # Create build directory if it doesn't exist
    os.makedirs(build_dir, exist_ok=True)
    
    # Run cmake
    result = subprocess.run(['cmake', '..'], cwd=build_dir, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"CMAKE failed: {result.stderr}")
        return False
    
    # Build
    result = subprocess.run(['cmake', '--build', '.'], cwd=build_dir, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"Build failed: {result.stderr}")
        return False
    
    return True

def run_cli_command(args, timeout=5):
    """Run a CLI command and return the result"""
    exe = find_executable()
    if not exe:
        return {'success': False, 'output': '', 'error': 'Executable not found'}
    
    # Get the build directory to run commands from
    build_dir = os.path.dirname(exe)
    
    try:
        # Convert args list to a single command string
        command_str = ' '.join(args) if isinstance(args, list) else args
        result = subprocess.run(
            [exe, '--headless', '--command', command_str],
            capture_output=True,
            text=True,
            timeout=timeout,
            cwd=build_dir  # Run from build directory
        )
        
        return {
            'success': result.returncode == 0,
            'output': result.stdout,
            'error': result.stderr
        }
    except subprocess.TimeoutExpired:
        return {'success': False, 'output': '', 'error': 'Command timed out'}
    except Exception as e:
        return {'success': False, 'output': '', 'error': str(e)}

def run_cli_batch(commands, timeout=30):
    """Run multiple CLI commands in batch mode"""
    exe = find_executable()
    if not exe:
        return {'success': False, 'output': '', 'error': 'Executable not found'}
    
    # Get the build directory to run commands from
    build_dir = os.path.dirname(exe)
    
    try:
        # Build the batch command arguments
        batch_args = [exe, '--headless', '--json', '--batch']
        for cmd in commands:
            batch_args.append(cmd if isinstance(cmd, str) else ' '.join(cmd))
        
        result = subprocess.run(
            batch_args,
            capture_output=True,
            text=True,
            timeout=timeout,
            cwd=build_dir  # Run from build directory
        )
        
        # Try to parse JSON output for batch mode
        if result.returncode == 0 and result.stdout:
            try:
                return json.loads(result.stdout)
            except json.JSONDecodeError:
                return {
                    'success': result.returncode == 0,
                    'output': result.stdout,
                    'error': result.stderr
                }
        else:
            return {
                'success': result.returncode == 0,
                'output': result.stdout,
                'error': result.stderr
            }
    except subprocess.TimeoutExpired:
        return {'success': False, 'output': '', 'error': 'Commands timed out'}
    except Exception as e:
        return {'success': False, 'output': '', 'error': str(e)}

def wait_for_output(process, expected_text, timeout=5):
    """Wait for expected text in process output"""
    import select
    start_time = time.time()
    output = ""
    
    while time.time() - start_time < timeout:
        if process.poll() is not None:
            # Process has terminated
            remaining_output = process.stdout.read()
            output += remaining_output
            if expected_text in output:
                return True, output
            return False, output
        
        # Check if there's data to read
        ready, _, _ = select.select([process.stdout], [], [], 0.1)
        if ready:
            line = process.stdout.readline()
            output += line
            if expected_text in output:
                return True, output
    
    return False, output

def print_test_header(test_name):
    """Print a formatted test header"""
    print(f"\n{'='*60}")
    print(f"{test_name}")
    print(f"{'='*60}")

def print_test_result(test_name, success, message=""):
    """Print a formatted test result"""
    status = "PASS" if success else "FAIL"
    color = "\033[92m" if success else "\033[91m"  # Green for pass, red for fail
    reset = "\033[0m"
    
    result_line = f"{color}[{status}]{reset} {test_name}"
    if message:
        result_line += f" - {message}"
    print(result_line)

def cleanup_test_files(paths):
    """Clean up test files and directories"""
    import shutil
    for path in paths:
        if os.path.exists(path):
            if os.path.isdir(path):
                shutil.rmtree(path)
            else:
                os.remove(path)