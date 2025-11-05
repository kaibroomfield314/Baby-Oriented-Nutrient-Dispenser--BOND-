import SwiftUI

struct AdherenceView: View {
    @ObservedObject var scheduleManager: ScheduleManager
    @ObservedObject var pillManager: PillManager
    @State private var selectedSchedule: PillSchedule?

    var body: some View {
        NavigationView {
            List {
                if scheduleManager.schedules.isEmpty {
                    VStack(spacing: 20) {
                        Image(systemName: "calendar")
                            .font(.system(size: 70))
                            .foregroundColor(.gray)
                        Text("No Schedules")
                            .font(.title2)
                            .fontWeight(.semibold)
                        Text("Create schedules to track adherence")
                            .foregroundColor(.secondary)
                            .multilineTextAlignment(.center)
                    }
                    .padding()
                    .frame(maxWidth: .infinity, alignment: .center)
                } else {
                    ForEach(scheduleManager.schedules) { schedule in
                        let pillName = schedule.pillId.flatMap { pillManager.getPill(by: $0) }?.name ?? "Unknown"
                        AdherenceRowView(
                            schedule: schedule,
                            statistics: scheduleManager.statistics[schedule.id],
                            pilName: pillName
                        )
                        .onTapGesture {
                            selectedSchedule = schedule
                        }
                    }
                }
            }
            .navigationTitle("Adherence Tracking")
            .sheet(item: $selectedSchedule) { schedule in
                AdherenceDetailView(
                    schedule: schedule,
                    scheduleManager: scheduleManager,
                    pillManager: pillManager
                )
            }
        }
    }
}

struct AdherenceRowView: View {
    let schedule: PillSchedule
    let statistics: MedicationStatistics?
    let pilName: String

    var adherencePercentage: Double {
        statistics?.adherencePercentage ?? 0
    }

    var adherenceColor: Color {
        if adherencePercentage >= 80 {
            return .green
        } else if adherencePercentage >= 60 {
            return .yellow
        } else {
            return .red
        }
    }

    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                VStack(alignment: .leading, spacing: 4) {
                    Text(schedule.label.isEmpty ? "Pill reminder" : schedule.label)
                        .font(.headline)
                    Text(schedule.time, style: .time)
                        .font(.subheadline)
                        .foregroundColor(.secondary)
                }
                Spacer()
                VStack(alignment: .trailing, spacing: 4) {
                    HStack(spacing: 4) {
                        Circle()
                            .fill(adherenceColor)
                            .frame(width: 8, height: 8)
                        Text("\(Int(adherencePercentage))%")
                            .font(.headline)
                            .foregroundColor(adherenceColor)
                    }
                    Text("Streak: \(statistics?.currentStreak ?? 0) days")
                        .font(.caption)
                        .foregroundColor(.secondary)
                }
            }

            // Progress bar
            ProgressView(value: adherencePercentage / 100)
                .tint(adherenceColor)

            HStack(spacing: 20) {
                VStack(alignment: .leading, spacing: 2) {
                    Text("Taken")
                        .font(.caption)
                        .foregroundColor(.secondary)
                    Text("\(statistics?.totalTaken ?? 0)")
                        .font(.headline)
                        .foregroundColor(.green)
                }

                VStack(alignment: .leading, spacing: 2) {
                    Text("Missed")
                        .font(.caption)
                        .foregroundColor(.secondary)
                    Text("\(statistics?.totalMissed ?? 0)")
                        .font(.headline)
                        .foregroundColor(.red)
                }

                VStack(alignment: .leading, spacing: 2) {
                    Text("Total")
                        .font(.caption)
                        .foregroundColor(.secondary)
                    Text("\(statistics?.totalScheduled ?? 0)")
                        .font(.headline)
                        .foregroundColor(.blue)
                }
            }
        }
        .padding(.vertical, 8)
    }
}

struct AdherenceDetailView: View {
    let schedule: PillSchedule
    @ObservedObject var scheduleManager: ScheduleManager
    @ObservedObject var pillManager: PillManager
    @Environment(\.dismiss) var dismiss

    var doseRecords: [DoseRecord] {
        scheduleManager.getDoseRecords(for: schedule.id)
    }

    var body: some View {
        NavigationView {
            List {
                Section(header: Text("Schedule Info")) {
                    VStack(alignment: .leading, spacing: 8) {
                        Text(schedule.label.isEmpty ? "Pill reminder" : schedule.label)
                            .font(.headline)
                        Text(schedule.time, style: .time)
                            .font(.subheadline)
                            .foregroundColor(.secondary)
                        Text("\(schedule.pillCount) pill(s)")
                            .font(.caption)
                            .foregroundColor(.secondary)
                    }
                }

                Section(header: Text("Statistics")) {
                    let stats = scheduleManager.statistics[schedule.id]
                    HStack {
                        VStack(alignment: .leading) {
                            Text("Adherence")
                                .font(.subheadline)
                                .foregroundColor(.secondary)
                            Text("\(Int(stats?.adherencePercentage ?? 0))%")
                                .font(.title2)
                                .fontWeight(.bold)
                        }
                        Spacer()
                        VStack(alignment: .trailing) {
                            Text("Current Streak")
                                .font(.subheadline)
                                .foregroundColor(.secondary)
                            Text("\(stats?.currentStreak ?? 0) days")
                                .font(.title2)
                                .fontWeight(.bold)
                        }
                    }
                }

                Section(header: Text("Pending Doses")) {
                    let pendingDoses = doseRecords.filter { $0.status == .pending }.sorted(by: { $0.scheduledTime < $1.scheduledTime })
                    if pendingDoses.isEmpty {
                        Text("No pending doses")
                            .foregroundColor(.secondary)
                    } else {
                        ForEach(pendingDoses) { record in
                            PendingDoseRow(
                                record: record,
                                scheduleId: schedule.id,
                                scheduleManager: scheduleManager
                            )
                        }
                    }
                }

                Section(header: Text("Dose History")) {
                    let nonPendingDoses = doseRecords.filter { $0.status != .pending }.sorted(by: { $0.scheduledTime > $1.scheduledTime }).prefix(10)
                    if nonPendingDoses.isEmpty {
                        Text("No dose history yet")
                            .foregroundColor(.secondary)
                    } else {
                        ForEach(nonPendingDoses) { record in
                            DoseRecordRow(record: record)
                        }
                    }
                }
            }
            .navigationTitle("Adherence Details")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                Button("Done") { dismiss() }
            }
        }
    }
}

struct DoseRecordRow: View {
    let record: DoseRecord

    var statusColor: Color {
        switch record.status {
        case .taken: return .green
        case .late: return .yellow
        case .missed: return .red
        case .skipped: return .gray
        case .pending: return .blue
        }
    }

    var statusIcon: String {
        switch record.status {
        case .taken: return "checkmark.circle.fill"
        case .late: return "clock.fill"
        case .missed: return "xmark.circle.fill"
        case .skipped: return "slash.circle.fill"
        case .pending: return "questionmark.circle.fill"
        }
    }

    var body: some View {
        HStack {
            VStack(alignment: .leading, spacing: 4) {
                Text(record.scheduledTime, style: .date)
                    .font(.subheadline)
                    .fontWeight(.semibold)
                Text(record.scheduledTime, style: .time)
                    .font(.caption)
                    .foregroundColor(.secondary)
            }

            Spacer()

            HStack(spacing: 8) {
                VStack(alignment: .trailing, spacing: 2) {
                    Text(record.status.rawValue.capitalized)
                        .font(.caption)
                        .fontWeight(.semibold)
                    if let actualTime = record.actualTime {
                        Text(actualTime, style: .time)
                            .font(.caption2)
                            .foregroundColor(.secondary)
                    }
                }

                Image(systemName: statusIcon)
                    .foregroundColor(statusColor)
            }
        }
        .padding(.vertical, 4)
    }
}

struct PendingDoseRow: View {
    let record: DoseRecord
    let scheduleId: UUID
    @ObservedObject var scheduleManager: ScheduleManager
    @State private var isExpanded = false
    @State private var errorMessage: String?
    @State private var isProcessing = false

    var body: some View {
        VStack(spacing: 12) {
            HStack {
                VStack(alignment: .leading, spacing: 4) {
                    Text(record.scheduledTime, style: .date)
                        .font(.subheadline)
                        .fontWeight(.semibold)
                    Text(record.scheduledTime, style: .time)
                        .font(.caption)
                        .foregroundColor(.secondary)
                }

                Spacer()

                HStack(spacing: 8) {
                    Image(systemName: "bell.badge.fill")
                        .foregroundColor(.orange)
                    Text("Pending")
                        .font(.caption)
                        .fontWeight(.semibold)
                }
            }

            if isExpanded {
                Divider()

                HStack(spacing: 10) {
                    Button(action: markAsTaken) {
                        if isProcessing {
                            HStack(spacing: 4) {
                                ProgressView()
                                    .progressViewStyle(.circular)
                                    .scaleEffect(0.8)
                                Text("Processing...")
                                    .font(.caption)
                                    .fontWeight(.semibold)
                            }
                        } else {
                            HStack(spacing: 4) {
                                Image(systemName: "checkmark.circle.fill")
                                Text("Taken")
                                    .font(.caption)
                                    .fontWeight(.semibold)
                            }
                        }
                    }
                    .frame(maxWidth: .infinity)
                    .padding(.vertical, 8)
                    .background(Color.green.opacity(0.2))
                    .foregroundColor(.green)
                    .cornerRadius(8)
                    .disabled(isProcessing)

                    Button(action: markAsSkipped) {
                        if isProcessing {
                            HStack(spacing: 4) {
                                ProgressView()
                                    .progressViewStyle(.circular)
                                    .scaleEffect(0.8)
                                Text("Processing...")
                                    .font(.caption)
                                    .fontWeight(.semibold)
                            }
                        } else {
                            HStack(spacing: 4) {
                                Image(systemName: "slash.circle.fill")
                                Text("Skip")
                                    .font(.caption)
                                    .fontWeight(.semibold)
                            }
                        }
                    }
                    .frame(maxWidth: .infinity)
                    .padding(.vertical, 8)
                    .background(Color.gray.opacity(0.2))
                    .foregroundColor(.gray)
                    .cornerRadius(8)
                    .disabled(isProcessing)

                    Button(action: dismissNotification) {
                        HStack(spacing: 4) {
                            Image(systemName: "xmark.circle.fill")
                            Text("Dismiss")
                        }
                        .font(.caption)
                        .fontWeight(.semibold)
                    }
                    .frame(maxWidth: .infinity)
                    .padding(.vertical, 8)
                    .background(Color.red.opacity(0.2))
                    .foregroundColor(.red)
                    .cornerRadius(8)
                    .disabled(isProcessing)
                }

                if let error = errorMessage {
                    Text(error)
                        .font(.caption)
                        .foregroundColor(.red)
                }
            }
        }
        .padding(.vertical, 8)
        .opacity(isProcessing ? 0.7 : 1.0)
        .onTapGesture {
            if !isProcessing {
                withAnimation {
                    isExpanded.toggle()
                }
            }
        }
    }

    private func markAsTaken() {
        withAnimation {
            isProcessing = true
            errorMessage = nil
        }
        
        Task {
            do {
                try scheduleManager.markDoseAsTaken(scheduleId: scheduleId)
                try await Task.sleep(nanoseconds: 300_000_000) // 0.3 seconds
                withAnimation {
                    isProcessing = false
                    isExpanded = false
                }
            } catch {
                withAnimation {
                    isProcessing = false
                    errorMessage = error.localizedDescription
                }
            }
        }
    }

    private func markAsSkipped() {
        withAnimation {
            isProcessing = true
            errorMessage = nil
        }
        
        Task {
            do {
                try scheduleManager.markDoseAsSkipped(scheduleId: scheduleId, reason: "Manually skipped from Adherence view")
                try await Task.sleep(nanoseconds: 300_000_000) // 0.3 seconds
                withAnimation {
                    isProcessing = false
                    isExpanded = false
                }
            } catch {
                withAnimation {
                    isProcessing = false
                    errorMessage = error.localizedDescription
                }
            }
        }
    }

    private func dismissNotification() {
        withAnimation {
            scheduleManager.dismissNotification(for: scheduleId)
            isExpanded = false
        }
    }
}


#Preview {
    AdherenceView(scheduleManager: ScheduleManager(), pillManager: PillManager())
}
