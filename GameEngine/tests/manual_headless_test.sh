#!/bin/bash
echo "Manual headless test - should not crash"
cd ../build

echo "Test 1: Simple help command"
./game --headless --command "help"

echo "Test 2: Entity creation (uses ResourceManager)"  
./game --headless --batch "project.create TestHeadless" "project.open TestHeadless" "entity.create Player"

echo "If you see this message, headless mode is working!"