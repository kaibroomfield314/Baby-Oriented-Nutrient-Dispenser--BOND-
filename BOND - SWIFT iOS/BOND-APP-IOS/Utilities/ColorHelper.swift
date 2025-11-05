import SwiftUI

struct ColorHelper {
    static let availableColors = ["red", "blue", "green", "orange", "purple", "pink", "yellow", "cyan", "mint", "indigo"]

    static func colorFromString(_ colorName: String) -> Color {
        switch colorName {
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
