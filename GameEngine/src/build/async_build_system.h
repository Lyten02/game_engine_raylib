#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <queue>
#include <mutex>

namespace GameEngine {

class Project;
class BuildSystem;

class AsyncBuildSystem {
public:
    enum class BuildStatus {
        Idle,
        InProgress,
        Success,
        Failed
    };
    
    struct BuildProgress {
        std::atomic<float> progress{0.0f};
        std::atomic<BuildStatus> status{BuildStatus::Idle};
        std::string currentStep;
        std::queue<std::string> messages;
        std::mutex messageMutex;
        std::string errorMessage;
    };
    
    using ProgressCallback = std::function<void(const std::string& message, float progress)>;
    
private:
    std::unique_ptr<std::thread> buildThread;
    BuildProgress buildProgress;
    std::unique_ptr<BuildSystem> buildSystem;
    
public:
    AsyncBuildSystem();
    ~AsyncBuildSystem();
    
    bool startBuild(Project* project, const std::string& buildConfig = "Release");
    bool startFastBuild(Project* project, const std::string& buildConfig = "Release");
    void cancelBuild();
    
    BuildStatus getStatus() const { return buildProgress.status.load(); }
    float getProgress() const { return buildProgress.progress.load(); }
    std::string getCurrentStep() const { return buildProgress.currentStep; }
    
    bool hasMessages();
    std::string getNextMessage();
    
    void waitForCompletion();
    
private:
    void buildThreadFunc(Project* project, const std::string& buildConfig);
    void fastBuildThreadFunc(Project* project, const std::string& buildConfig);
    void addMessage(const std::string& message);
    void addMessageWithLimit(const std::string& key, const std::string& message);
    void setProgress(float progress, const std::string& step);
};

} // namespace GameEngine