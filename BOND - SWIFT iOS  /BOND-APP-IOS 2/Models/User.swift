import Foundation

struct User: Identifiable, Codable {
    let id: UUID
    var name: String
    var role: UserRole
    var email: String?
    var createdAt: Date = Date()
    var lastActiveAt: Date = Date()
    var isActive: Bool = true

    enum UserRole: String, Codable {
        case patient
        case caregiver
        case admin
    }

    init(name: String, role: UserRole, email: String? = nil) {
        self.id = UUID()
        self.name = name
        self.role = role
        self.email = email
    }
}

struct UserPreferences: Codable {
    var userId: UUID
    var isDarkModeEnabled: Bool = false
    var enableNotifications: Bool = true
    var notificationTime: Date = Date(timeIntervalSince1970: 0)
    var enableVoiceOver: Bool = false
    var useDynamicType: Bool = true
    var language: String = "en"
    var timezone: String = TimeZone.current.identifier
    var pillReminderAdvance: Int = 5
    var enableHealthAppSync: Bool = false
    var enableAutoDispense: Bool = false
}

struct FamilyGroup: Identifiable, Codable {
    let id: UUID
    var name: String
    var adminId: UUID
    var members: [UUID] = []
    var createdAt: Date = Date()

    init(name: String, adminId: UUID) {
        self.id = UUID()
        self.name = name
        self.adminId = adminId
        self.members = [adminId]
    }
}

struct CaregiverRelationship: Identifiable, Codable {
    let id: UUID
    let caregiverId: UUID
    let patientId: UUID
    let relationshipType: String
    var hasAccessToAdherence: Bool = true
    var hasAccessToSchedules: Bool = true
    var hasAccessToReminders: Bool = false
    var createdAt: Date = Date()

    init(caregiverId: UUID, patientId: UUID, relationshipType: String) {
        self.id = UUID()
        self.caregiverId = caregiverId
        self.patientId = patientId
        self.relationshipType = relationshipType
    }
}
