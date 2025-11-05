import Foundation
import UserNotifications
import Combine
import UIKit

enum ScheduleManagerError: LocalizedError {
    case saveFailed(String)
    case loadFailed(String)
    case notificationFailed(String)
    case invalidSchedule(String)

    var errorDescription: String? {
        switch self {
        case .saveFailed(let message):
            return "Failed to save schedules: \(message)"
        case .loadFailed(let message):
            return "Failed to load schedules: \(message)"
        case .notificationFailed(let message):
            return "Notification failed: \(message)"
        case .invalidSchedule(let message):
            return "Invalid schedule: \(message)"
        }
    }
}

class ScheduleManager: ObservableObject {
    @Published var schedules: [PillSchedule] = []
    @Published var doseRecords: [DoseRecord] = []
    @Published var statistics: [UUID: MedicationStatistics] = [:]
    @Published var lastError: Error?

    private let schedulesKey = "pill_schedules"
    private let doseRecordsKey = "dose_records"
    private let statisticsKey = "medication_statistics"
    private let logger = Logger.shared

    init() {
        loadSchedules()
        loadDoseRecords()
        loadStatistics()
        setupNotifications()
        startScheduleMonitoring()
    }

    // MARK: - Schedule Management
    func addSchedule(_ schedule: PillSchedule) throws {
        guard !schedule.label.isEmpty || schedule.scheduleType == .general else {
            throw ScheduleManagerError.invalidSchedule("Schedule label cannot be empty")
        }

        var newSchedule = schedule
        newSchedule.createdAt = Date()
        schedules.append(newSchedule)
        try saveSchedules()
        try scheduleNotification(for: newSchedule)
        logger.log("Added schedule at \(newSchedule.time)", level: .info)
    }

    func updateSchedule(_ schedule: PillSchedule) throws {
        if let index = schedules.firstIndex(where: { $0.id == schedule.id }) {
            schedules[index] = schedule
            try saveSchedules()
            try cancelNotification(for: schedule.id)
            if schedule.enabled {
                try scheduleNotification(for: schedule)
            }
            logger.log("Updated schedule: \(schedule.label)", level: .info)
        }
    }

    func deleteSchedule(_ schedule: PillSchedule) throws {
        schedules.removeAll { $0.id == schedule.id }
        try saveSchedules()
        try cancelNotification(for: schedule.id)
        logger.log("Deleted schedule: \(schedule.label)", level: .info)
    }

    // MARK: - Adherence Tracking
    func markDoseAsTaken(scheduleId: UUID, actualTime: Date = Date()) throws {
        if let index = doseRecords.firstIndex(where: { $0.scheduleId == scheduleId && $0.status == .pending }) {
            doseRecords[index].actualTime = actualTime
            let isLate = actualTime > doseRecords[index].scheduledTime.addingTimeInterval(5 * 60)
            doseRecords[index].status = isLate ? .late : .taken

            try saveDoseRecords()
            try updateStatistics(for: scheduleId)
            // Dismiss the notification when dose is marked as taken
            removeDeliveredNotification(for: scheduleId)
            
            // Trigger publisher update by replacing the array
            DispatchQueue.main.async {
                self.doseRecords = self.doseRecords
            }
            
            logger.log("Marked dose as taken for schedule: \(scheduleId)", level: .info)
        }
    }

    func markDoseAsSkipped(scheduleId: UUID, reason: String = "") throws {
        if let index = doseRecords.firstIndex(where: { $0.scheduleId == scheduleId && $0.status == .pending }) {
            doseRecords[index].status = .skipped
            doseRecords[index].notes = reason

            try saveDoseRecords()
            try updateStatistics(for: scheduleId)
            // Dismiss the notification when dose is marked as skipped
            removeDeliveredNotification(for: scheduleId)
            
            // Trigger publisher update by replacing the array
            DispatchQueue.main.async {
                self.doseRecords = self.doseRecords
            }
            
            logger.log("Marked dose as skipped for schedule: \(scheduleId)", level: .info)
        }
    }

    func getDoseRecords(for scheduleId: UUID) -> [DoseRecord] {
        doseRecords.filter { $0.scheduleId == scheduleId }
    }

    func getAdherencePercentage(for scheduleId: UUID) -> Double {
        statistics[scheduleId]?.adherencePercentage ?? 0
    }

    func getCurrentStreak(for scheduleId: UUID) -> Int {
        statistics[scheduleId]?.currentStreak ?? 0
    }

    // MARK: - Private Methods
    private func setupNotifications() {
        UNUserNotificationCenter.current().requestAuthorization(options: [.alert, .sound, .badge]) { granted, error in
            if granted {
                self.logger.log("Notification permission granted", level: .info)
            } else if let error = error {
                self.logger.log("Notification permission denied: \(error.localizedDescription)", level: .warning)
            }
        }
    }

    private func scheduleNotification(for schedule: PillSchedule) throws {
        let content = UNMutableNotificationContent()
        content.title = "Pill Reminder"
        content.body = schedule.label.isEmpty ? "Time to take your pills" : schedule.label
        content.sound = .default
        content.badge = NSNumber(value: UIApplication.shared.applicationIconBadgeNumber + 1)

        let calendar = Calendar.current
        let components = calendar.dateComponents([.hour, .minute], from: schedule.time)
        let trigger: UNNotificationTrigger? = UNCalendarNotificationTrigger(dateMatching: components, repeats: true)

        guard trigger != nil else {
            throw ScheduleManagerError.notificationFailed("Failed to create notification trigger")
        }

        let request = UNNotificationRequest(identifier: schedule.id.uuidString, content: content, trigger: trigger)
        UNUserNotificationCenter.current().add(request) { error in
            if let error = error {
                self.logger.log("Failed to schedule notification: \(error.localizedDescription)", level: .error)
            }
        }
    }

    private func cancelNotification(for id: UUID) throws {
        UNUserNotificationCenter.current().removePendingNotificationRequests(withIdentifiers: [id.uuidString])
    }
    /// Remove delivered notifications from the notification center
    private func removeDeliveredNotification(for scheduleId: UUID) {
        UNUserNotificationCenter.current().removeDeliveredNotifications(withIdentifiers: [scheduleId.uuidString])
    }
    
    /// Public method to dismiss a notification from the notification center
    func dismissNotification(for scheduleId: UUID) {
        removeDeliveredNotification(for: scheduleId)
        logger.log("Dismissed notification for schedule: \(scheduleId)", level: .info)
    }

    private func startScheduleMonitoring() {
        // Check every minute for upcoming doses
        Timer.scheduledTimer(withTimeInterval: 60, repeats: true) { [weak self] _ in
            self?.checkAndCreateDueRecords()
        }
    }

    private func checkAndCreateDueRecords() {
        let now = Date()
        let calendar = Calendar.current
        let today = calendar.startOfDay(for: now)
        
        for schedule in schedules where schedule.enabled {
            // Check if we haven't already created a record for today
            let existingRecord = doseRecords.first { 
                $0.scheduleId == schedule.id && calendar.startOfDay(for: $0.scheduledTime) == today
            }
            
            // If no record exists for today, create one
            if existingRecord == nil {
                let record = DoseRecord(scheduleId: schedule.id, scheduledTime: schedule.time, pillCount: schedule.pillCount)
                doseRecords.append(record)
                try? saveDoseRecords()
                logger.log("Created dose record for schedule: \(schedule.label)", level: .info)
            }
        }
    }


    private func updateStatistics(for scheduleId: UUID) throws {
        let records = doseRecords.filter { $0.scheduleId == scheduleId }
        var stats = statistics[scheduleId] ?? MedicationStatistics(scheduleId: scheduleId)

        stats.totalScheduled = records.count
        stats.totalTaken = records.filter { $0.status == .taken || $0.status == .late }.count
        stats.totalMissed = records.filter { $0.status == .missed }.count
        stats.totalSkipped = records.filter { $0.status == .skipped }.count

        // Calculate streaks
        let sortedRecords = records.sorted { $0.scheduledTime > $1.scheduledTime }
        var currentStreak = 0
        var longestStreak = 0
        var tempStreak = 0

        for record in sortedRecords {
            if record.status == .taken || record.status == .late {
                tempStreak += 1
                longestStreak = max(longestStreak, tempStreak)
            } else {
                if tempStreak > currentStreak {
                    currentStreak = tempStreak
                }
                tempStreak = 0
            }
        }

        stats.currentStreak = tempStreak > currentStreak ? tempStreak : currentStreak
        stats.longestStreak = longestStreak
        stats.lastTakenDate = sortedRecords.first(where: { $0.status == .taken || $0.status == .late })?.actualTime
        stats.lastMissedDate = sortedRecords.first(where: { $0.status == .missed })?.scheduledTime

        statistics[scheduleId] = stats
        try saveStatistics()
        
        // Trigger publisher update by replacing the dictionary
        DispatchQueue.main.async {
            self.statistics = self.statistics
        }
    }

    // MARK: - Persistence
    private func saveSchedules() throws {
        do {
            let encoded = try JSONEncoder().encode(schedules)
            UserDefaults.standard.set(encoded, forKey: schedulesKey)
        } catch {
            let error = ScheduleManagerError.saveFailed(error.localizedDescription)
            lastError = error
            logger.log("Save schedules failed: \(error.localizedDescription)", level: .error)
            throw error
        }
    }

    private func loadSchedules() {
        do {
            if let data = UserDefaults.standard.data(forKey: schedulesKey) {
                let decoded = try JSONDecoder().decode([PillSchedule].self, from: data)
                DispatchQueue.main.async {
                    self.schedules = decoded
                    self.logger.log("Loaded \(decoded.count) schedules", level: .info)
                }
            }
        } catch {
            let error = ScheduleManagerError.loadFailed(error.localizedDescription)
            lastError = error
            logger.log("Load schedules failed: \(error.localizedDescription)", level: .error)
        }
    }

    private func saveDoseRecords() throws {
        do {
            let encoded = try JSONEncoder().encode(doseRecords)
            UserDefaults.standard.set(encoded, forKey: doseRecordsKey)
        } catch {
            let error = ScheduleManagerError.saveFailed(error.localizedDescription)
            lastError = error
            throw error
        }
    }

    private func loadDoseRecords() {
        do {
            if let data = UserDefaults.standard.data(forKey: doseRecordsKey) {
                let decoded = try JSONDecoder().decode([DoseRecord].self, from: data)
                DispatchQueue.main.async {
                    self.doseRecords = decoded
                    self.logger.log("Loaded \(decoded.count) dose records", level: .info)
                }
            }
        } catch {
            logger.log("Load dose records failed: \(error.localizedDescription)", level: .warning)
        }
    }

    private func saveStatistics() throws {
        do {
            let encoded = try JSONEncoder().encode(statistics)
            UserDefaults.standard.set(encoded, forKey: statisticsKey)
        } catch {
            let error = ScheduleManagerError.saveFailed(error.localizedDescription)
            lastError = error
            throw error
        }
    }

    private func loadStatistics() {
        do {
            if let data = UserDefaults.standard.data(forKey: statisticsKey) {
                let decoded = try JSONDecoder().decode([String: MedicationStatistics].self, from: data)
                DispatchQueue.main.async {
                    self.statistics = decoded.reduce(into: [:]) { dict, pair in
                        if let uuid = UUID(uuidString: pair.key) {
                            dict[uuid] = pair.value
                        }
                    }
                }
            }
        } catch {
            logger.log("Load statistics failed: \(error.localizedDescription)", level: .warning)
        }
    }
}
