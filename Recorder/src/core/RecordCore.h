#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <opencv2/opencv.hpp>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>

class ConsoleLogger {
public:
    void logInfo(const std::string& message) {
        std::cout << "[INFO] " << message << std::endl;
    }
};

class ConfigManager {
private:
    int fps;
    int buffer_size_sec;
    std::string save_path;
    int hotkey_vk;

public:
    ConfigManager() : fps(30), buffer_size_sec(10), save_path("./"), hotkey_vk(VK_F10) {}
    void setFps(int newFps) { fps = (newFps > 0) ? newFps : 30; }
    void setBufferSizeSec(int newSec) { buffer_size_sec = (newSec > 0) ? newSec : 10; }
    void setSavePath(const std::string& path) { save_path = path.empty() ? "./" : path; }
    void setHotkeyVk(int newVk) { hotkey_vk = (newVk > 0) ? newVk : VK_F10; }
    int getFps() const { return fps; }
    int getBufferSizeSec() const { return buffer_size_sec; }
    std::string getSavePath() const { return save_path; }
    int getHotkeyVk() const { return hotkey_vk; }
};

struct FramePacket {
    cv::Mat frame;
    std::chrono::steady_clock::time_point timestamp;

    FramePacket clone() const {
        return { frame.clone(), timestamp };
    }
};

template <typename T>
class RingBuffer {
private:
    std::vector<T> buffer;
    size_t capacity;
    size_t head = 0;
    size_t count = 0;
    mutable std::mutex mtx;

public:
    explicit RingBuffer(size_t cap) : capacity(cap > 0 ? cap : 1) {
        buffer.reserve(capacity);
    }

    void push(const T& item) {
        std::lock_guard<std::mutex> lock(mtx);
        if (capacity == 0) return;

        if (buffer.size() < capacity) {
            buffer.push_back(item);
            head = buffer.size() % capacity;
            count = buffer.size();
        }
        else {
            buffer[head] = item;
            head = (head + 1) % capacity;
        }
    }

    std::vector<T> snapshot() const {
        std::lock_guard<std::mutex> lock(mtx);
        std::vector<T> result;
        if (count == 0) return result;

        result.reserve(count);
        if (count < capacity) {
            for (size_t i = 0; i < count; ++i) {
                result.push_back(buffer[i].clone());
            }
        }
        else {
            for (size_t i = 0; i < capacity; ++i) {
                size_t idx = (head + i) % capacity;
                result.push_back(buffer[idx].clone());
            }
        }
        return result;
    }

    void resize(size_t new_capacity) {
        std::lock_guard<std::mutex> lock(mtx);
        buffer.clear();
        capacity = (new_capacity > 0) ? new_capacity : 1;
        buffer.reserve(capacity);
        head = 0;
        count = 0;
    }
};

class Application {
private:
    std::shared_ptr<ConsoleLogger> logger;
    ConfigManager config;
    std::unique_ptr<RingBuffer<FramePacket>> ring_buffer;
    std::thread worker_thread;
    std::atomic<bool> is_recording{ false };
    std::mutex save_mtx;

    cv::Mat captureScreen() {
        int width = GetSystemMetrics(SM_CXSCREEN);
        int height = GetSystemMetrics(SM_CYSCREEN);

        HDC hScreenDC = GetDC(nullptr);
        HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
        HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
        HGDIOBJ oldObj = SelectObject(hMemoryDC, hBitmap);

        BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY);

        BITMAPINFOHEADER bi{};
        bi.biSize = sizeof(BITMAPINFOHEADER);
        bi.biWidth = width;
        bi.biHeight = -height;
        bi.biPlanes = 1;
        bi.biBitCount = 32;
        bi.biCompression = BI_RGB;

        cv::Mat bgra(height, width, CV_8UC4);
        GetDIBits(hMemoryDC, hBitmap, 0, height, bgra.data, reinterpret_cast<BITMAPINFO*>(&bi), DIB_RGB_COLORS);

        SelectObject(hMemoryDC, oldObj);
        DeleteObject(hBitmap);
        DeleteDC(hMemoryDC);
        ReleaseDC(nullptr, hScreenDC);

        cv::Mat bgr;
        cv::cvtColor(bgra, bgr, cv::COLOR_BGRA2BGR);
        return bgr;
    }

    void captureLoop() {
        bool hotkeyWasPressed = false;

        while (is_recording.load()) {
            auto frameStart = std::chrono::steady_clock::now();
            cv::Mat frame = captureScreen();
            if (!frame.empty()) {
                ring_buffer->push({ frame, frameStart });
            }
            int currentHotkey = config.getHotkeyVk();
            bool keyDown = (GetAsyncKeyState(currentHotkey) & 0x8000) != 0;
            if (keyDown && !hotkeyWasPressed) {
                saveHighlight("Hotkey");
            }
            hotkeyWasPressed = keyDown;
            int targetDelayMs = 1000 / config.getFps();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - frameStart
            ).count();
            if (elapsed < targetDelayMs) {
                std::this_thread::sleep_for(std::chrono::milliseconds(targetDelayMs - elapsed));
            }
        }
    }

public:
    explicit Application(std::shared_ptr<ConsoleLogger> log)
        : logger(std::move(log)) {
        size_t initialCap = static_cast<size_t>(config.getFps() * config.getBufferSizeSec());
        ring_buffer = std::make_unique<RingBuffer<FramePacket>>(initialCap);
    }

    ~Application() {
        stopRecording();
    }

    void initialize() {
        if (logger) {
            logger->logInfo("Recorder initialized");
        }
    }

    void startRecording() {
        if (is_recording.load()) return;
        is_recording.store(true);
        worker_thread = std::thread(&Application::captureLoop, this);
    }

    void stopRecording() {
        if (!is_recording.load()) return;
        is_recording.store(false);
        if (worker_thread.joinable()) {
            worker_thread.join();
        }
    }

    void updateConfig(int newFps, int newBufferSec, const std::string& newSavePath, int newHotkeyVk = VK_F10) {
        bool wasRecording = is_recording.load();
        if (wasRecording) {
            stopRecording();
        }
        config.setFps(newFps);
        config.setBufferSizeSec(newBufferSec);
        config.setSavePath(newSavePath);
        config.setHotkeyVk(newHotkeyVk);
        size_t newCap = static_cast<size_t>(config.getFps() * config.getBufferSizeSec());
        ring_buffer->resize(newCap);
        if (logger) {
            logger->logInfo("Config updated: " + std::to_string(newFps) + " FPS, " +
                std::to_string(newBufferSec) + "s, path: " + config.getSavePath());
        }
        if (wasRecording) {
            startRecording();
        }
    }

    void saveHighlight(const std::string& triggerSource) {
        std::lock_guard<std::mutex> lock(save_mtx);

        std::vector<FramePacket> allFrames = ring_buffer->snapshot();
        if (allFrames.empty()) {
            if (logger) logger->logInfo("Save skipped: buffer is empty");
            return;
        }

        auto newestTime = allFrames.back().timestamp;
        double maxDurationSec = static_cast<double>(config.getBufferSizeSec());
        std::vector<FramePacket> frames;
        frames.reserve(allFrames.size());

        for (const auto& pkt : allFrames) {
            double ageSec = std::chrono::duration<double>(newestTime - pkt.timestamp).count();
            if (ageSec <= maxDurationSec) {
                frames.push_back(pkt);
            }
        }

        if (frames.empty()) return;

       
        double realFps = static_cast<double>(config.getFps());
        if (frames.size() > 1) {
            double actualDurationSec = std::chrono::duration<double>(
                frames.back().timestamp - frames.front().timestamp
            ).count();
            if (actualDurationSec > 0.1) {
                realFps = static_cast<double>(frames.size() - 1) / actualDurationSec;
            }
        }

        std::filesystem::path dirPath(config.getSavePath());
        std::error_code ec;
        std::filesystem::create_directories(dirPath, ec);

        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm_buf{};
        localtime_s(&tm_buf, &t);

        std::ostringstream nameStream;
        nameStream << "clip_" << std::put_time(&tm_buf, "%Y%m%d_%H%M%S") << ".mp4";
        std::filesystem::path fullPath = dirPath / nameStream.str();

        cv::Size frameSize = frames[0].frame.size();
        int fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
        cv::VideoWriter writer(fullPath.string(), fourcc, realFps, frameSize);

        if (!writer.isOpened()) {
            if (logger) logger->logInfo("Error: Could not open VideoWriter for " + fullPath.string());
            return;
        }

        for (const auto& pkt : frames) {
            writer.write(pkt.frame);
        }
        writer.release();

        if (logger) {
            logger->logInfo("Saved clip (" + triggerSource + "): " + fullPath.string());
        }
    }
};