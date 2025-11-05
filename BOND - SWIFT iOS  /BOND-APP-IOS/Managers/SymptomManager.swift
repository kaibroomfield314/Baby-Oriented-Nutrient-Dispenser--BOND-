import Foundation
import Combine

enum SymptomManagerError: LocalizedError {
    case symptomNotFound
    case saveFailed(String)
    case loadFailed(String)
    
    var errorDescription: String? {
        switch self {
        case .symptomNotFound:
            return "Symptom not found"
        case .saveFailed(let message):
            return "Failed to save symptom: \(message)"
        case .loadFailed(let message):
            return "Failed to load symptoms: \(message)"
        }
    }
}

class SymptomManager: ObservableObject {
    @Published var symptoms: [Symptom] = []
    @Published var lastError: Error?
    
    private let symptomsKey = "app_symptoms"
    private let logger = Logger.shared
    
    init() {
        loadSymptoms()
    }
    
    func logSymptom(symptomName: String, severity: Symptom.Severity, notes: String = "", relatedMedications: [UUID] = []) throws {
        let symptom = Symptom(symptomName: symptomName, severity: severity, notes: notes, relatedMedications: relatedMedications)
        symptoms.append(symptom)
        try saveSymptoms()
        
        logger.log("Logged symptom: \(symptomName) (severity: \(severity.description))", level: .info)
    }
    
    func updateSymptom(_ symptom: Symptom) throws {
        if let index = symptoms.firstIndex(where: { $0.id == symptom.id }) {
            symptoms[index] = symptom
            try saveSymptoms()
            
            logger.log("Updated symptom: \(symptom.symptomName)", level: .info)
        }
    }
    
    func deleteSymptom(_ symptom: Symptom) throws {
        symptoms.removeAll { $0.id == symptom.id }
        try saveSymptoms()
        
        logger.log("Deleted symptom: \(symptom.symptomName)", level: .info)
    }
    
    func getSymptom(by id: UUID) -> Symptom? {
        symptoms.first { $0.id == id }
    }
    
    func getSymptomsByName(_ name: String) -> [Symptom] {
        symptoms.filter { $0.symptomName.lowercased().contains(name.lowercased()) }
            .sorted { $0.dateLogged > $1.dateLogged }
    }
    
    func getSymptomsSince(days: Int) -> [Symptom] {
        let cutoffDate = Calendar.current.date(byAdding: .day, value: -days, to: Date()) ?? Date()
        return symptoms.filter { $0.dateLogged >= cutoffDate }
            .sorted { $0.dateLogged > $1.dateLogged }
    }
    
    func getMostCommonSymptoms(limit: Int = 5) -> [SymptomSummary] {
        let grouped = Dictionary(grouping: symptoms, by: { $0.symptomName })
        return grouped.map { name, symptomList in
            SymptomSummary(
                name: name,
                count: symptomList.count,
                averageSeverity: Double(symptomList.map { $0.severity.rawValue }.reduce(0, +)) / Double(symptomList.count),
                lastOccurrence: symptomList.max(by: { $0.dateLogged < $1.dateLogged })?.dateLogged ?? Date()
            )
        }
        .sorted { $0.count > $1.count }
        .prefix(limit)
        .map { $0 }
    }
    
    func getSymptomsTrend(for symptomName: String, days: Int = 30) -> [SymptomTrendPoint] {
        let cutoffDate = Calendar.current.date(byAdding: .day, value: -days, to: Date()) ?? Date()
        let filtered = symptoms.filter { $0.symptomName == symptomName && $0.dateLogged >= cutoffDate }
        
        let grouped = Dictionary(grouping: filtered) { symptom in
            Calendar.current.startOfDay(for: symptom.dateLogged)
        }
        
        return grouped.map { date, symptomList in
            SymptomTrendPoint(
                date: date,
                count: symptomList.count,
                averageSeverity: Double(symptomList.map { $0.severity.rawValue }.reduce(0, +)) / Double(symptomList.count)
            )
        }
        .sorted { $0.date < $1.date }
    }
    
    private func saveSymptoms() throws {
        do {
            let encoded = try JSONEncoder().encode(symptoms)
            UserDefaults.standard.set(encoded, forKey: symptomsKey)
        } catch {
            let error = SymptomManagerError.saveFailed(error.localizedDescription)
            lastError = error
            logger.log("Save symptoms failed: \(error.localizedDescription)", level: .error)
            throw error
        }
    }
    
    private func loadSymptoms() {
        do {
            if let data = UserDefaults.standard.data(forKey: symptomsKey) {
                let decoded = try JSONDecoder().decode([Symptom].self, from: data)
                DispatchQueue.main.async {
                    self.symptoms = decoded
                    self.logger.log("Loaded \(decoded.count) symptoms", level: .info)
                }
            }
        } catch {
            logger.log("Load symptoms failed: \(error.localizedDescription)", level: .warning)
        }
    }
}

struct SymptomSummary: Identifiable {
    let id = UUID()
    let name: String
    let count: Int
    let averageSeverity: Double
    let lastOccurrence: Date
}

struct SymptomTrendPoint: Identifiable {
    let id = UUID()
    let date: Date
    let count: Int
    let averageSeverity: Double
}
