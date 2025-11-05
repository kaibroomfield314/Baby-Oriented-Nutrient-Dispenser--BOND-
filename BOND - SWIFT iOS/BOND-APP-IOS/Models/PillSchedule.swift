import Foundation

struct PillSchedule: Identifiable, Codable {
    var id = UUID()
    var time: Date
    var enabled: Bool
    var pillCount: Int
    var label: String
    var scheduleType: ScheduleType
    var pillId: UUID? // For specific pill scheduling
    var compartment: Int? // For compartment scheduling
    var createdAt: Date = Date()

    enum ScheduleType: String, Codable {
        case general
        case specificPill
        case specificCompartment
    }
}

// MARK: - Adherence Tracking
struct DoseRecord: Identifiable, Codable {
    let id: UUID
    let scheduleId: UUID
    let scheduledTime: Date
    var actualTime: Date?
    var status: DoseStatus
    let pillCount: Int
    var notes: String = ""

    enum DoseStatus: String, Codable {
        case pending    // Not yet due
        case missed     // Due but not taken
        case taken      // Completed on time
        case late       // Completed but after scheduled time
        case skipped    // User marked as skipped
    }

    init(scheduleId: UUID, scheduledTime: Date, pillCount: Int) {
        self.id = UUID()
        self.scheduleId = scheduleId
        self.scheduledTime = scheduledTime
        self.actualTime = nil
        self.status = .pending
        self.pillCount = pillCount
    }
}

// MARK: - Statistics
struct MedicationStatistics: Codable {
    let scheduleId: UUID
    var totalScheduled: Int = 0
    var totalTaken: Int = 0
    var totalMissed: Int = 0
    var totalSkipped: Int = 0
    var adherencePercentage: Double {
        guard totalScheduled > 0 else { return 0 }
        return Double(totalTaken) / Double(totalScheduled) * 100
    }
    var currentStreak: Int = 0
    var longestStreak: Int = 0
    var lastTakenDate: Date?
    var lastMissedDate: Date?
}
