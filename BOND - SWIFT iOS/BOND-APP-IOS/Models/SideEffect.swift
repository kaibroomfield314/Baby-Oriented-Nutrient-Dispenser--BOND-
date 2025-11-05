import Foundation

struct SideEffect: Identifiable, Codable {
    let id: UUID
    var pillId: UUID // Link to specific medication
    var effectName: String // e.g., "Nausea", "Headache", "Dizziness"
    var severity: Severity // 1-5 scale
    var dateLogged: Date
    var notes: String = ""
    var managedWith: String = "" // What helped manage it (e.g., "Ginger tea", "Rest")
    
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
    }
    
    init(pillId: UUID, effectName: String, severity: Severity, notes: String = "", managedWith: String = "") {
        self.id = UUID()
        self.pillId = pillId
        self.effectName = effectName
        self.severity = severity
        self.dateLogged = Date()
        self.notes = notes
        self.managedWith = managedWith
    }
}
