import SwiftUI

struct AppTheme {
    // MARK: - Colors
    static let primaryColor = Color.blue
    static let secondaryColor = Color.gray
    static let accentColor = Color.green
    static let warningColor = Color.orange
    static let errorColor = Color.red
    static let successColor = Color.green

    // MARK: - Background Colors
    static func backgroundColor(for colorScheme: ColorScheme) -> Color {
        colorScheme == .dark ? Color(UIColor(red: 0.1, green: 0.1, blue: 0.1, alpha: 1)) : Color.white
    }

    static func secondaryBackgroundColor(for colorScheme: ColorScheme) -> Color {
        colorScheme == .dark ? Color(UIColor(red: 0.15, green: 0.15, blue: 0.15, alpha: 1)) : Color(UIColor(red: 0.95, green: 0.95, blue: 0.95, alpha: 1))
    }

    // MARK: - Text Colors
    static func textColor(for colorScheme: ColorScheme) -> Color {
        colorScheme == .dark ? Color.white : Color.black
    }

    static func secondaryTextColor(for colorScheme: ColorScheme) -> Color {
        colorScheme == .dark ? Color(UIColor(red: 0.7, green: 0.7, blue: 0.7, alpha: 1)) : Color.secondary
    }

    // MARK: - Spacing
    static let spacing: CGFloat = 16
    static let smallSpacing: CGFloat = 8
    static let largeSpacing: CGFloat = 24

    // MARK: - Corner Radius
    static let cornerRadius: CGFloat = 12
    static let smallCornerRadius: CGFloat = 8
    static let largeCornerRadius: CGFloat = 16

    // MARK: - Animation
    static let defaultAnimation = Animation.easeInOut(duration: 0.3)
    static let quickAnimation = Animation.easeInOut(duration: 0.15)
    static let slowAnimation = Animation.easeInOut(duration: 0.5)
}

// MARK: - View Modifiers
struct DarkModeAware: ViewModifier {
    @Environment(\.colorScheme) var colorScheme

    func body(content: Content) -> some View {
        content
            .environment(\.colorScheme, colorScheme)
    }
}

extension View {
    func darkModeAware() -> some View {
        modifier(DarkModeAware())
    }

    func adaptiveBackground(for colorScheme: ColorScheme) -> some View {
        background(AppTheme.backgroundColor(for: colorScheme))
    }

    func adaptiveText(for colorScheme: ColorScheme) -> some View {
        foregroundColor(AppTheme.textColor(for: colorScheme))
    }
}
