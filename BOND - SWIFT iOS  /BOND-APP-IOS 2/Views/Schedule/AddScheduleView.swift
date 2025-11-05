import SwiftUI

struct AddScheduleView: View {
    @ObservedObject var scheduleManager: ScheduleManager
    @ObservedObject var pillManager: PillManager
    @Environment(\.dismiss) var dismiss

    @State private var selectedTime = Date()
    @State private var pillCount = 1
    @State private var label = ""
    @State private var scheduleType: PillSchedule.ScheduleType = .general
    @State private var selectedPill: Pill?
    @State private var selectedCompartment = 1

    var body: some View {
        NavigationView {
            Form {
                Section(header: Text("Time")) {
                    DatePicker("Reminder Time", selection: $selectedTime, displayedComponents: .hourAndMinute)
                }

                Section(header: Text("Type")) {
                    Picker("Schedule Type", selection: $scheduleType) {
                        Text("General Reminder").tag(PillSchedule.ScheduleType.general)
                        Text("Specific Pill").tag(PillSchedule.ScheduleType.specificPill)
                        Text("Specific Compartment").tag(PillSchedule.ScheduleType.specificCompartment)
                    }
                    .pickerStyle(.menu)
                }

                if scheduleType == .specificPill {
                    Section(header: Text("Select Pill")) {
                        if pillManager.pills.isEmpty {
                            Text("No pills available. Add pills first.")
                                .foregroundColor(.secondary)
                                .font(.subheadline)
                        } else {
                            Picker("Pill", selection: $selectedPill) {
                                Text("Choose a pill").tag(nil as Pill?)
                                ForEach(pillManager.pills) { pill in
                                    HStack {
                                        Circle()
                                            .fill(pill.pillColour)
                                            .frame(width: 12, height: 12)
                                        Text(pill.name)
                                    }.tag(pill as Pill?)
                                }
                            }
                            .pickerStyle(.menu)
                        }
                    }
                }

                if scheduleType == .specificCompartment {
                    Section(header: Text("Select Compartment")) {
                        Picker("Compartment", selection: $selectedCompartment) {
                            ForEach(1...5, id: \.self) { i in
                                Text("Slot \(i)").tag(i)
                            }
                        }
                        .pickerStyle(.menu)
                    }
                }

                Section(header: Text("Details")) {
                    TextField("Label (optional)", text: $label)

                    Stepper("Quantity: \(pillCount)", value: $pillCount, in: 1...10)
                }
            }
            .navigationTitle("Add Schedule")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarLeading) {
                    Button("Cancel") { dismiss() }
                }
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("Save") {
                        saveSchedule()
                    }
                    .disabled(!canSave)
                }
            }
        }
    }

    var canSave: Bool {
        switch scheduleType {
        case .general:
            return true
        case .specificPill:
            return selectedPill != nil
        case .specificCompartment:
            return true
        }
    }

    func saveSchedule() {
        // Auto-generate label if empty
        var finalLabel = label
        if finalLabel.isEmpty {
            switch scheduleType {
            case .general:
                finalLabel = "Pill Reminder"
            case .specificPill:
                finalLabel = selectedPill?.name ?? "Pill Schedule"
            case .specificCompartment:
                finalLabel = "Compartment \(selectedCompartment)"
            }
        }
        
        let schedule = PillSchedule(
            time: selectedTime,
            enabled: true,
            pillCount: pillCount,
            label: finalLabel,
            scheduleType: scheduleType,
            pillId: scheduleType == .specificPill ? selectedPill?.id : nil,
            compartment: scheduleType == .specificCompartment ? selectedCompartment : nil
        )
        do {
            try scheduleManager.addSchedule(schedule)
            dismiss()
        } catch {
            scheduleManager.lastError = error
        }
    }
}

#Preview {
    AddScheduleView(scheduleManager: ScheduleManager(), pillManager: PillManager())
}
