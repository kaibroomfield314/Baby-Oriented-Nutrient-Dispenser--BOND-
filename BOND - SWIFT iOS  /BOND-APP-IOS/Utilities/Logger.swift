import Foundation
import os

class Logger {
    static let shared = Logger()

    private let osLog = OSLog(subsystem: "com.bond.pills", category: "General")

    enum LogLevel: String {
        case debug = "🔍"
        case info = "ℹ️"
        case warning = "⚠️"
        case error = "❌"
    }

    func log(_ message: String, level: LogLevel = .info, file: String = #file, function: String = #function, line: Int = #line) {
        let filename = (file as NSString).lastPathComponent
        let timestamp = ISO8601DateFormatter().string(from: Date())
        let logMessage = "[\(timestamp)] [\(level.rawValue) \(level)] [\(filename):\(line)] \(function) - \(message)"

        // Print to console
        print(logMessage)

        // Log to system
        let osLogType: OSLogType
        switch level {
        case .debug: osLogType = .debug
        case .info: osLogType = .info
        case .warning: osLogType = .default
        case .error: osLogType = .error
        }
        os_log("%{public}@", log: osLog, type: osLogType, message)
    }
}
