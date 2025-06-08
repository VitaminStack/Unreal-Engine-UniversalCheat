// Logger.h
#pragma once
#include <fstream>
#include <mutex>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <cstdarg>
#include <thread>
#include <string>
#include <iostream>
#include <chrono>
#include <vector>
#include <windows.h> // Für WideCharToMultiByte

#define LOGGER_CONSOLE_OUTPUT // aktivieren für zusätzliches Log in die Konsole

namespace Logger
{
    enum class Level
    {
        Debug,
        Info,
        Warn,
        Error
    };

    inline std::string ws2s(const std::wstring& wstr)
    {
        if (wstr.empty()) return {};
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
        std::string strTo(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &strTo[0], size_needed, nullptr, nullptr);
        return strTo;
    }

    class Logger
    {
    public:
        static Logger& Instance()
        {
            static Logger instance;
            return instance;
        }

        void SetLogFile(const std::string& path)
        {
            std::lock_guard<std::mutex> lock(logMutex);
            if (logfile.is_open())
                logfile.close();
            logfile.open(path, std::ios::app);
            logFilePath = path;
        }

        std::string GetLogFilePath()
        {
            std::lock_guard<std::mutex> lock(logMutex);
            return logFilePath;
        }

        void Log(Level level, const char* fmt, ...)
        {
            if (!logfile.is_open())
                logfile.open(logFilePath, std::ios::app);

            char msgBuf[2048];

            va_list args;
            va_start(args, fmt);
            vsnprintf(msgBuf, sizeof(msgBuf), fmt, args);
            va_end(args);

            std::string finalMsg = FormatEntry(level, msgBuf);

            {
                std::lock_guard<std::mutex> lock(logMutex);
                if (logfile.is_open())
                {
                    logfile << finalMsg << std::endl;
                    logfile.flush();
                }
#ifdef LOGGER_CONSOLE_OUTPUT
                std::cout << finalMsg << std::endl;
#endif
            }
        }

        // Overload für std::string, falls du z.B. JSON loggen willst
        void Log(Level level, const std::string& msg)
        {
            if (!logfile.is_open())
                logfile.open(logFilePath, std::ios::app);

            std::string finalMsg = FormatEntry(level, msg.c_str());
            std::lock_guard<std::mutex> lock(logMutex);
            if (logfile.is_open())
            {
                logfile << finalMsg << std::endl;
                logfile.flush();
            }
#ifdef LOGGER_CONSOLE_OUTPUT
            std::cout << finalMsg << std::endl;
#endif
        }

        // Komfort-Methoden
        void LogDebug(const char* fmt, ...) { va_list args; va_start(args, fmt); VLog(Level::Debug, fmt, args); va_end(args); }
        void LogInfo(const char* fmt, ...) { va_list args; va_start(args, fmt); VLog(Level::Info, fmt, args); va_end(args); }
        void LogWarn(const char* fmt, ...) { va_list args; va_start(args, fmt); VLog(Level::Warn, fmt, args); va_end(args); }
        void LogError(const char* fmt, ...) { va_list args; va_start(args, fmt); VLog(Level::Error, fmt, args); va_end(args); }

        void LogDebug(const std::string& msg) { Log(Level::Debug, msg); }
        void LogInfo(const std::string& msg) { Log(Level::Info, msg); }
        void LogWarn(const std::string& msg) { Log(Level::Warn, msg); }
        void LogError(const std::string& msg) { Log(Level::Error, msg); }

        // NEU: Universelle Log-Methode für beliebige Argumente!
        template<typename... Args>
        void LogAny(Level lvl, Args&&... args)
        {
            std::ostringstream oss;
            (oss << ... << ToLogString(std::forward<Args>(args)));
            Log(lvl, oss.str());
        }

        // --- ToLogString: universal für beliebige Typen ---
        // Für std::wstring
        static std::string ToLogString(const std::wstring& ws)
        {
            if (ws.empty()) return {};
            int size_needed = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), NULL, 0, NULL, NULL);
            std::string strTo(size_needed, 0);
            WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), &strTo[0], size_needed, NULL, NULL);
            return strTo;
        }
        // Für wchar_t*
        static std::string ToLogString(const wchar_t* ws)
        {
            if (!ws) return {};
            int size_needed = WideCharToMultiByte(CP_UTF8, 0, ws, (int)wcslen(ws), NULL, 0, NULL, NULL);
            std::string strTo(size_needed, 0);
            WideCharToMultiByte(CP_UTF8, 0, ws, (int)wcslen(ws), &strTo[0], size_needed, NULL, NULL);
            return strTo;
        }
        // Für char*
        static std::string ToLogString(const char* s)
        {
            return s ? std::string(s) : std::string();
        }
        // Für std::string
        static std::string ToLogString(const std::string& s)
        {
            return s;
        }
        // Für alle anderen Typen (int, double, enum, usw.)
        template<typename T>
        static std::string ToLogString(const T& val)
        {
            std::ostringstream oss;
            oss << val;
            return oss.str();
        }

    private:
        std::ofstream logfile;
        std::string logFilePath = "C:\\MeinProjektLog.txt";
        std::mutex logMutex;

        Logger()
        {
            logfile.open(logFilePath, std::ios::app);
        }
        ~Logger()
        {
            if (logfile.is_open()) logfile.close();
        }

        // Copy-Protect
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;

        std::string NowString()
        {
            std::ostringstream oss;
            auto now = std::chrono::system_clock::now();
            auto t_c = std::chrono::system_clock::to_time_t(now);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

            tm localTm;
#ifdef _WIN32
            localtime_s(&localTm, &t_c);
#else
            localtime_r(&t_c, &localTm);
#endif
            oss << "[" << std::put_time(&localTm, "%Y-%m-%d %H:%M:%S")
                << "." << std::setw(3) << std::setfill('0') << ms.count() << "]";
            return oss.str();
        }

        std::string LevelString(Level lvl)
        {
            switch (lvl)
            {
            case Level::Debug: return "[DEBUG]";
            case Level::Info:  return "[INFO ]";
            case Level::Warn:  return "[WARN ]";
            case Level::Error: return "[ERROR]";
            default:           return "[UNKWN]";
            }
        }

        std::string ThreadString()
        {
            std::ostringstream oss;
            oss << "[TID:" << std::this_thread::get_id() << "]";
            return oss.str();
        }

        std::string FormatEntry(Level lvl, const char* msg)
        {
            std::ostringstream oss;
            oss << NowString() << " " << LevelString(lvl) << " " << ThreadString() << " " << msg;
            return oss.str();
        }

        void VLog(Level lvl, const char* fmt, va_list args)
        {
            char buf[2048];
            vsnprintf(buf, sizeof(buf), fmt, args);
            Log(lvl, buf);
        }
    };

    // Convenience-Macros
#define LOG_DEBUG(...)    ::Logger::Logger::Instance().LogDebug(__VA_ARGS__)
#define LOG_INFO(...)     ::Logger::Logger::Instance().LogInfo(__VA_ARGS__)
#define LOG_WARN(...)     ::Logger::Logger::Instance().LogWarn(__VA_ARGS__)
#define LOG_ERROR(...)    ::Logger::Logger::Instance().LogError(__VA_ARGS__)

#define LOG_DEBUG_ANY(...) ::Logger::Logger::Instance().LogAny(::Logger::Level::Debug, __VA_ARGS__)
#define LOG_INFO_ANY(...)  ::Logger::Logger::Instance().LogAny(::Logger::Level::Info, __VA_ARGS__)
#define LOG_WARN_ANY(...)  ::Logger::Logger::Instance().LogAny(::Logger::Level::Warn, __VA_ARGS__)
#define LOG_ERROR_ANY(...) ::Logger::Logger::Instance().LogAny(::Logger::Level::Error, __VA_ARGS__)

#define LOG_SETFILE(filepath) ::Logger::Logger::Instance().SetLogFile(filepath)
#define LOG_GETFILE()         ::Logger::Logger::Instance().GetLogFilePath()

} // namespace Logger
