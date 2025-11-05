import Foundation
import Combine

enum UserManagerError: LocalizedError {
    case userNotFound
    case userAlreadyExists
    case saveFailed(String)
    case loadFailed(String)

    var errorDescription: String? {
        switch self {
        case .userNotFound:
            return "User not found"
        case .userAlreadyExists:
            return "User already exists"
        case .saveFailed(let message):
            return "Failed to save user: \(message)"
        case .loadFailed(let message):
            return "Failed to load users: \(message)"
        }
    }
}

class UserManager: ObservableObject {
    @Published var currentUser: User?
    @Published var allUsers: [User] = []
    @Published var userPreferences: UserPreferences?
    @Published var caregiverRelationships: [CaregiverRelationship] = []
    @Published var lastError: Error?

    private let usersKey = "app_users"
    private let preferencesKey = "user_preferences"
    private let relationshipsKey = "caregiver_relationships"
    private let currentUserKey = "current_user_id"
    private let logger = Logger.shared

    init() {
        loadAllUsers()
        loadCurrentUser()
        loadPreferences()
        loadRelationships()
    }

    func createUser(name: String, role: User.UserRole, email: String? = nil) throws {
        let newUser = User(name: name, role: role, email: email)

        if allUsers.contains(where: { $0.name.lowercased() == name.lowercased() }) {
            throw UserManagerError.userAlreadyExists
        }

        allUsers.append(newUser)
        try saveUsers()

        var preferences = UserPreferences(userId: newUser.id)
        try savePreferences(preferences)

        logger.log("Created user: \(newUser.name)", level: .info)
    }

    func setCurrentUser(_ user: User) throws {
        currentUser = user
        UserDefaults.standard.set(user.id.uuidString, forKey: currentUserKey)
        loadPreferences()
        logger.log("Set current user: \(user.name)", level: .info)
    }

    func updateUser(_ user: User) throws {
        if let index = allUsers.firstIndex(where: { $0.id == user.id }) {
            allUsers[index] = user
            try saveUsers()

            if currentUser?.id == user.id {
                currentUser = user
            }

            logger.log("Updated user: \(user.name)", level: .info)
        }
    }

    func deleteUser(_ user: User) throws {
        allUsers.removeAll { $0.id == user.id }
        try saveUsers()

        if currentUser?.id == user.id {
            currentUser = allUsers.first
            if let currentUser = currentUser {
                try setCurrentUser(currentUser)
            }
        }

        logger.log("Deleted user: \(user.name)", level: .info)
    }

    func getUser(by id: UUID) -> User? {
        allUsers.first { $0.id == id }
    }

    func updatePreferences(_ preferences: UserPreferences) throws {
        userPreferences = preferences
        try savePreferences(preferences)
        logger.log("Updated user preferences", level: .info)
    }

    func toggleDarkMode() throws {
        guard var prefs = userPreferences else { return }
        prefs.isDarkModeEnabled.toggle()
        try updatePreferences(prefs)
    }

    func toggleNotifications() throws {
        guard var prefs = userPreferences else { return }
        prefs.enableNotifications.toggle()
        try updatePreferences(prefs)
    }

    func createCaregiverRelationship(caregiverId: UUID, patientId: UUID, relationshipType: String) throws {
        let relationship = CaregiverRelationship(caregiverId: caregiverId, patientId: patientId, relationshipType: relationshipType)

        if caregiverRelationships.contains(where: { $0.caregiverId == caregiverId && $0.patientId == patientId }) {
            throw UserManagerError.userAlreadyExists
        }

        caregiverRelationships.append(relationship)
        try saveRelationships()
        logger.log("Created caregiver relationship", level: .info)
    }

    func getCaregivingRelationships(for patientId: UUID) -> [CaregiverRelationship] {
        caregiverRelationships.filter { $0.patientId == patientId }
    }

    func getPatientsForCaregiver(_ caregiverId: UUID) -> [User] {
        let patientIds = caregiverRelationships
            .filter { $0.caregiverId == caregiverId }
            .map { $0.patientId }

        return allUsers.filter { patientIds.contains($0.id) }
    }

    func updateRelationshipPermissions(_ relationship: CaregiverRelationship) throws {
        if let index = caregiverRelationships.firstIndex(where: { $0.id == relationship.id }) {
            caregiverRelationships[index] = relationship
            try saveRelationships()
            logger.log("Updated relationship permissions", level: .info)
        }
    }

    private func saveUsers() throws {
        do {
            let encoded = try JSONEncoder().encode(allUsers)
            UserDefaults.standard.set(encoded, forKey: usersKey)
        } catch {
            let error = UserManagerError.saveFailed(error.localizedDescription)
            lastError = error
            logger.log("Save users failed: \(error.localizedDescription)", level: .error)
            throw error
        }
    }

    private func loadAllUsers() {
        do {
            if let data = UserDefaults.standard.data(forKey: usersKey) {
                let decoded = try JSONDecoder().decode([User].self, from: data)
                DispatchQueue.main.async {
                    self.allUsers = decoded
                    self.logger.log("Loaded \(decoded.count) users", level: .info)
                }
            }
        } catch {
            logger.log("Load users failed: \(error.localizedDescription)", level: .warning)
        }
    }

    private func loadCurrentUser() {
        if let userIdString = UserDefaults.standard.string(forKey: currentUserKey),
           let userId = UUID(uuidString: userIdString) {
            currentUser = allUsers.first { $0.id == userId }
        }
    }

    private func savePreferences(_ preferences: UserPreferences) throws {
        do {
            let encoded = try JSONEncoder().encode(preferences)
            UserDefaults.standard.set(encoded, forKey: preferencesKey)
        } catch {
            let error = UserManagerError.saveFailed(error.localizedDescription)
            lastError = error
            throw error
        }
    }

    private func loadPreferences() {
        guard let userId = currentUser?.id else { return }

        do {
            if let data = UserDefaults.standard.data(forKey: preferencesKey) {
                let decoded = try JSONDecoder().decode(UserPreferences.self, from: data)
                DispatchQueue.main.async {
                    self.userPreferences = decoded
                }
            } else {
                let defaultPreferences = UserPreferences(userId: userId)
                try? savePreferences(defaultPreferences)
                DispatchQueue.main.async {
                    self.userPreferences = defaultPreferences
                }
            }
        } catch {
            logger.log("Load preferences failed: \(error.localizedDescription)", level: .warning)
        }
    }

    private func saveRelationships() throws {
        do {
            let encoded = try JSONEncoder().encode(caregiverRelationships)
            UserDefaults.standard.set(encoded, forKey: relationshipsKey)
        } catch {
            let error = UserManagerError.saveFailed(error.localizedDescription)
            lastError = error
            throw error
        }
    }

    private func loadRelationships() {
        do {
            if let data = UserDefaults.standard.data(forKey: relationshipsKey) {
                let decoded = try JSONDecoder().decode([CaregiverRelationship].self, from: data)
                DispatchQueue.main.async {
                    self.caregiverRelationships = decoded
                }
            }
        } catch {
            logger.log("Load relationships failed: \(error.localizedDescription)", level: .warning)
        }
    }
}
