import SwiftUI

struct Pill: Identifiable, Codable, Hashable {
    var id = UUID()
    var name: String
    var colour: String
    var compartment: Int
    var dosage: String = "" // e.g., "500mg"
    var notes: String = ""

    var pillColour: Color {
        switch colour {
        case "red": return .red
        case "blue": return .blue
        case "green": return .green
        case "orange": return .orange
        case "purple": return .purple
        case "pink": return .pink
        case "yellow": return .yellow
        case "cyan": return .cyan
        case "mint": return .mint
        case "indigo": return .indigo
        default: return .blue
        }
    }
}
