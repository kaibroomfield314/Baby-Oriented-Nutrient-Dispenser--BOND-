import Foundation

struct Symptom: Identifiable, Codable {
    let id: UUID
    var symptomName: String // e.g., "Cough", "Chest Pain", "Fatigue"
    var severity: Severity // 1-5 scale
    var dateLogged: Date
    var notes: String = ""
    var relatedMedications: [UUID] = [] // Pills that might be related
    
    enum Severity: Int, Codable {
        case mild = 1
        case moderate = 2
        case moderate_severe = 3
        case severe = 4
        case very_severe = 5
        
        var description: String {
            switch self {
            case .mild: return "Mild"
            case .moderate: return "Moderate"
            case .moderate_severe: return "Moderate-Severe"
            case .severe: return "Severe"
            case .very_severe: return "Very Severe"
            }
        }
        
        var color: String {
            switch self {
            case .mild: return "green"
            case .moderate: return "yellow"
            case .moderate_severe: return "orange"
            case .severe: return "red"
            case .very_severe: return "red"
            }
        }
    }
    
    init(symptomName: String, severity: Severity, notes: String = "", relatedMedications: [UUID] = []) {
        self.id = UUID()
        self.symptomName = symptomName
        self.severity = severity
        self.dateLogged = Date()
        self.notes = notes
        self.relatedMedications = relatedMedications
    }
}
