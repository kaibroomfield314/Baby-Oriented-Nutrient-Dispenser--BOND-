import Foundation
import Combine

enum PillManagerError: LocalizedError {
    case saveFailed(String)
    case loadFailed(String)
    case invalidPill(String)

    var errorDescription: String? {
        switch self {
        case .saveFailed(let message):
            return "Failed to save pills: \(message)"
        case .loadFailed(let message):
            return "Failed to load pills: \(message)"
        case .invalidPill(let message):
            return "Invalid pill: \(message)"
        }
    }
}

class PillManager: ObservableObject {
    @Published var pills: [Pill] = []
    @Published var lastError: Error?

    private let pillsKey = "saved_pills"
    private let logger = Logger.shared

    init() {
        loadPills()
    }

    func addPill(_ pill: Pill) throws {
        guard !pill.name.isEmpty else {
            throw PillManagerError.invalidPill("Pill name cannot be empty")
        }

        pills.append(pill)
        try savePills()
        logger.log("Added pill: \(pill.name)", level: .info)
    }

    func updatePill(_ pill: Pill) throws {
        guard !pill.name.isEmpty else {
            throw PillManagerError.invalidPill("Pill name cannot be empty")
        }

        if let index = pills.firstIndex(where: { $0.id == pill.id }) {
            pills[index] = pill
            try savePills()
            logger.log("Updated pill: \(pill.name)", level: .info)
        }
    }

    func deletePill(_ pill: Pill) throws {
        pills.removeAll { $0.id == pill.id }
        try savePills()
        logger.log("Deleted pill: \(pill.name)", level: .info)
    }

    func getPill(by id: UUID) -> Pill? {
        pills.first { $0.id == id }
    }

    private func savePills() throws {
        do {
            let encoded = try JSONEncoder().encode(pills)
            UserDefaults.standard.set(encoded, forKey: pillsKey)
        } catch {
            let error = PillManagerError.saveFailed(error.localizedDescription)
            lastError = error
            logger.log("Save failed: \(error.localizedDescription)", level: .error)
            throw error
        }
    }

    private func loadPills() {
        do {
            if let data = UserDefaults.standard.data(forKey: pillsKey) {
                let decoded = try JSONDecoder().decode([Pill].self, from: data)
                DispatchQueue.main.async {
                    self.pills = decoded
                    self.logger.log("Loaded \(decoded.count) pills", level: .info)
                }
            }
        } catch {
            let error = PillManagerError.loadFailed(error.localizedDescription)
            lastError = error
            logger.log("Load failed: \(error.localizedDescription)", level: .error)
        }
    }
}
