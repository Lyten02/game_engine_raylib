-- Test Lua script for the game engine
log_info("Hello from Lua script!")

-- Test function
function test_function()
    log_info("Test function called from Lua")
    return 42
end

-- Test Vector3
local pos = Vector3()
pos.x = 10
pos.y = 5
pos.z = 0
log_info("Created position: " .. pos.x .. ", " .. pos.y .. ", " .. pos.z)

-- Test Vector3 with constructor
local vel = Vector3(1.5, 2.5, 3.5)
log_info("Created velocity: " .. tostring(vel))

-- Test Vector3 addition
local sum = pos + vel
log_info("Position + Velocity = " .. tostring(sum))

-- Test Transform
local transform = Transform()
transform.position.x = 100
transform.position.y = 200
transform.scale.x = 2
transform.scale.y = 2
log_info("Transform position: " .. tostring(transform.position))
log_info("Transform scale: " .. tostring(transform.scale))

-- Test error handling
log_warn("This is a warning from Lua")

-- Store some data for later use
global_data = {
    score = 100,
    level = 1,
    player_name = "Test Player"
}

log_info("Lua script initialization complete!")