import SwiftUI

struct EditScheduleView: View {
    @ObservedObject var scheduleManager: ScheduleManager
    @ObservedObject var pillManager: PillManager
    @Environment(\.dismiss) var dismiss

    let schedule: PillSchedule
    @State private var selectedTime = Date()
    @State private var pillCount = 1
    @State private var label = ""
    @State private var scheduleType: PillSchedule.ScheduleType = .general
    @State private var selectedPill: Pill?
    @State private var selectedCompartment = 1

    init(scheduleManager: ScheduleManager, pillManager: PillManager, schedule: PillSchedule) {
        self.scheduleManager = scheduleManager
        self.pillManager = pillManager
        self.schedule = schedule
        _selectedTime = State(initialValue: schedule.time)
        _pillCount = State(initialValue: schedule.pillCount)
        _label = State(initialValue: schedule.label)
        _scheduleType = State(initialValue: schedule.scheduleType)
        _selectedCompartment = State(initialValue: schedule.compartment ?? 1)

        if let pillId = schedule.pillId {
            _selectedPill = State(initialValue: pillManager.pills.first(where: { $0.id == pillId }))
        }
    }

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
            .navigationTitle("Edit Schedule")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarLeading) {
                    Button("Cancel") { dismiss() }
                }
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("Save") {
                        updateSchedule()
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

    func updateSchedule() {
        var updatedSchedule = schedule
        updatedSchedule.time = selectedTime
        updatedSchedule.pillCount = pillCount
        updatedSchedule.label = label
        updatedSchedule.scheduleType = scheduleType
        updatedSchedule.pillId = scheduleType == .specificPill ? selectedPill?.id : nil
        updatedSchedule.compartment = scheduleType == .specificCompartment ? selectedCompartment : nil

        do {
            try scheduleManager.updateSchedule(updatedSchedule)
            dismiss()
        } catch {
            scheduleManager.lastError = error
        }
    }
}

#Preview {
    EditScheduleView(
        scheduleManager: ScheduleManager(),
        pillManager: PillManager(),
        schedule: PillSchedule(time: Date(), enabled: true, pillCount: 1, label: "Test", scheduleType: .general)
    )
}
