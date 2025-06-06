#!/usr/bin/env python3
import os
import shutil

# List all test projects to remove
test_projects = [
    "projects/test",
    "projects/test_automation",
    "projects/test_entities",
    "projects/test_lifecycle",
    "projects/test_persist",
    "projects/test_proj1",
    "projects/test_proj2",
    "projects/test_proj3",
    "projects/test_scenes",
    "projects/cli_test_project",
    "projects/batch_test"
]

for project_path in test_projects:
    if os.path.exists(project_path):
        shutil.rmtree(project_path)
        print(f"Removed: {project_path}")

print("Cleanup complete!")