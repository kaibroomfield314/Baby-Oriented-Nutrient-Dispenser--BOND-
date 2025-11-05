import Foundation
import Combine

enum SideEffectManagerError: LocalizedError {
    case sideEffectNotFound
    case saveFailed(String)
    case loadFailed(String)
    
    var errorDescription: String? {
        switch self {
        case .sideEffectNotFound:
            return "Side effect not found"
        case .saveFailed(let message):
            return "Failed to save side effect: \(message)"
        case .loadFailed(let message):
            return "Failed to load side effects: \(message)"
        }
    }
}

class SideEffectManager: ObservableObject {
    @Published var sideEffects: [SideEffect] = []
    @Published var lastError: Error?
    
    private let sideEffectsKey = "app_side_effects"
    private let logger = Logger.shared
    
    init() {
        loadSideEffects()
    }
    
    func logSideEffect(pillId: UUID, effectName: String, severity: SideEffect.Severity, notes: String = "", managedWith: String = "") throws {
        let sideEffect = SideEffect(pillId: pillId, effectName: effectName, severity: severity, notes: notes, managedWith: managedWith)
        sideEffects.append(sideEffect)
        try saveSideEffects()
        
        logger.log("Logged side effect: \(effectName) (severity: \(severity.description))", level: .info)
    }
    
    func updateSideEffect(_ sideEffect: SideEffect) throws {
        if let index = sideEffects.firstIndex(where: { $0.id == sideEffect.id }) {
            sideEffects[index] = sideEffect
            try saveSideEffects()
            
            logger.log("Updated side effect: \(sideEffect.effectName)", level: .info)
        }
    }
    
    func deleteSideEffect(_ sideEffect: SideEffect) throws {
        sideEffects.removeAll { $0.id == sideEffect.id }
        try saveSideEffects()
        
        logger.log("Deleted side effect: \(sideEffect.effectName)", level: .info)
    }
    
    func getSideEffect(by id: UUID) -> SideEffect? {
        sideEffects.first { $0.id == id }
    }
    
    func getSideEffects(for pillId: UUID) -> [SideEffect] {
        sideEffects.filter { $0.pillId == pillId }
            .sorted { $0.dateLogged > $1.dateLogged }
    }
    
    func getSideEffectsByName(_ name: String) -> [SideEffect] {
        sideEffects.filter { $0.effectName.lowercased().contains(name.lowercased()) }
            .sorted { $0.dateLogged > $1.dateLogged }
    }
    
    func getMostCommonSideEffects(limit: Int = 5) -> [SideEffectSummary] {
        let grouped = Dictionary(grouping: sideEffects, by: { $0.effectName })
        return grouped.map { name, effects in
            SideEffectSummary(
                name: name,
                count: effects.count,
                averageSeverity: Double(effects.map { $0.severity.rawValue }.reduce(0, +)) / Double(effects.count)
            )
        }
        .sorted { $0.count > $1.count }
        .prefix(limit)
        .map { $0 }
    }
    
    private func saveSideEffects() throws {
        do {
            let encoded = try JSONEncoder().encode(sideEffects)
            UserDefaults.standard.set(encoded, forKey: sideEffectsKey)
        } catch {
            let error = SideEffectManagerError.saveFailed(error.localizedDescription)
            lastError = error
            logger.log("Save side effects failed: \(error.localizedDescription)", level: .error)
            throw error
        }
    }
    
    private func loadSideEffects() {
        do {
            if let data = UserDefaults.standard.data(forKey: sideEffectsKey) {
                let decoded = try JSONDecoder().decode([SideEffect].self, from: data)
                DispatchQueue.main.async {
                    self.sideEffects = decoded
                    self.logger.log("Loaded \(decoded.count) side effects", level: .info)
                }
            }
        } catch {
            logger.log("Load side effects failed: \(error.localizedDescription)", level: .warning)
        }
    }
}

struct SideEffectSummary: Identifiable {
    let id = UUID()
    let name: String
    let count: Int
    let averageSeverity: Double
}
