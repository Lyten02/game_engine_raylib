-- Script to rebuild MyGame project
print("Rebuilding MyGame project...")

-- Open the project
console_execute("project.open MyGame")

-- Load existing scene
console_execute("scene.load Level1")

-- Build the project
console_execute("project.build")

print("Rebuild script completed!")