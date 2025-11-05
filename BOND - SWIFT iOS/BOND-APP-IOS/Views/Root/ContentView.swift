import SwiftUI

struct ContentView: View {
    @StateObject private var bluetoothManager = BluetoothManager()
    @StateObject private var scheduleManager = ScheduleManager()
    @StateObject private var pillManager = PillManager()
    @StateObject private var userManager = UserManager()
    @StateObject private var appointmentManager = AppointmentManager()
    @StateObject private var sideEffectManager = SideEffectManager()
    @StateObject private var symptomManager = SymptomManager()
    @State private var selectedTab = 0
    @State private var showErrorAlert = false
    @State private var errorMessage = ""
    @Environment(\.colorScheme) var colorScheme

    var body: some View {
        TabView(selection: $selectedTab) {
            DispenseView(bluetoothManager: bluetoothManager, pillManager: pillManager)
                .tabItem {
                    Label("Dispense", systemImage: "pills.fill")
                }
                .tag(0)

            PillLibraryView(pillManager: pillManager)
                .tabItem {
                    Label("Pills", systemImage: "list.bullet.rectangle")
                }
                .tag(1)

            AdherenceView(scheduleManager: scheduleManager, pillManager: pillManager)
                .tabItem {
                    Label("Adherence", systemImage: "chart.bar.fill")
                }
                .tag(2)

            ScheduleView(scheduleManager: scheduleManager, bluetoothManager: bluetoothManager, pillManager: pillManager)
                .tabItem {
                    Label("Schedule", systemImage: "calendar")
                }
                .tag(3)

            AppointmentsView(appointmentManager: appointmentManager)
                .tabItem {
                    Label("Appointments", systemImage: "calendar.badge.clock")
                }
                .tag(4)

            SideEffectsView(sideEffectManager: sideEffectManager, pillManager: pillManager)
                .tabItem {
                    Label("Side Effects", systemImage: "exclamationmark.circle")
                }
                .tag(5)

            SymptomsView(symptomManager: symptomManager, pillManager: pillManager)
                .tabItem {
                    Label("Symptoms", systemImage: "heart.fill")
                }
                .tag(6)

            ConnectionView(bluetoothManager: bluetoothManager)
                .tabItem {
                    Label("Connect", systemImage: "antenna.radiowaves.left.and.right")
                }
                .tag(7)

            SettingsView(userManager: userManager)
                .tabItem {
                    Label("Settings", systemImage: "gear")
                }
                .tag(8)
        }
        .onReceive(pillManager.$lastError) { error in
            if let error = error {
                errorMessage = error.localizedDescription
                showErrorAlert = true
            }
        }
        .onReceive(scheduleManager.$lastError) { error in
            if let error = error {
                errorMessage = error.localizedDescription
                showErrorAlert = true
            }
        }
        .onReceive(bluetoothManager.$lastError) { error in
            if let error = error {
                errorMessage = error.localizedDescription
                showErrorAlert = true
            }
        }
        .onReceive(userManager.$lastError) { error in
            if let error = error {
                errorMessage = error.localizedDescription
                showErrorAlert = true
            }
        }
        .onReceive(appointmentManager.$lastError) { error in
            if let error = error {
                errorMessage = error.localizedDescription
                showErrorAlert = true
            }
        }
        .onReceive(sideEffectManager.$lastError) { error in
            if let error = error {
                errorMessage = error.localizedDescription
                showErrorAlert = true
            }
        }
        .onReceive(symptomManager.$lastError) { error in
            if let error = error {
                errorMessage = error.localizedDescription
                showErrorAlert = true
            }
        }
        .alert("Error", isPresented: $showErrorAlert) {
            Button("OK") { showErrorAlert = false }
        } message: {
            Text(errorMessage)
        }
    }
}

#Preview {
    ContentView()
}
