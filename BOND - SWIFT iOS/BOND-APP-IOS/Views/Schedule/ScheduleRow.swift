import SwiftUI

struct ScheduleRow: View {
    let schedule: PillSchedule
    @ObservedObject var scheduleManager: ScheduleManager
    @ObservedObject var pillManager: PillManager

    var scheduleDescription: String {
        switch schedule.scheduleType {
        case .specificPill:
            if let pillId = schedule.pillId,
               let pill = pillManager.pills.first(where: { $0.id == pillId }) {
                return "\(schedule.pillCount) × \(pill.name)"
            }
            return "\(schedule.pillCount) pill(s)"
        case .specificCompartment:
            if let compartment = schedule.compartment {
                return "\(schedule.pillCount) × Compartment \(compartment)"
            }
            return "\(schedule.pillCount) pill(s)"
        case .general:
            return "\(schedule.pillCount) pill(s)"
        }
    }

    var body: some View {
        HStack {
            VStack(alignment: .leading, spacing: 4) {
                Text(schedule.time, style: .time)
                    .font(.title2)
                    .fontWeight(.bold)

                Text(schedule.label.isEmpty ? "Pill reminder" : schedule.label)
                    .font(.subheadline)
                    .foregroundColor(.secondary)

                HStack(spacing: 4) {
                    if schedule.scheduleType == .specificPill,
                       let pillId = schedule.pillId,
                       let pill = pillManager.pills.first(where: { $0.id == pillId }) {
                        Circle()
                            .fill(pill.pillColour)
                            .frame(width: 8, height: 8)
                    }
                    Text(scheduleDescription)
                        .font(.caption)
                        .foregroundColor(.secondary)
                }
            }

            Spacer()

            Toggle("", isOn: Binding(
                get: { schedule.enabled },
                set: { newValue in
                    var updatedSchedule = schedule
                    updatedSchedule.enabled = newValue
                    do {
                        try scheduleManager.updateSchedule(updatedSchedule)
                    } catch {
                        scheduleManager.lastError = error
                    }
                }
            ))
        }
        .padding(.vertical, 4)
    }
}

#Preview {
    ScheduleRow(
        schedule: PillSchedule(time: Date(), enabled: true, pillCount: 1, label: "Test", scheduleType: .general),
        scheduleManager: ScheduleManager(),
        pillManager: PillManager()
    )
}
