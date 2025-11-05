import SwiftUI

struct ScheduleView: View {
    @ObservedObject var scheduleManager: ScheduleManager
    @ObservedObject var bluetoothManager: BluetoothManager
    @ObservedObject var pillManager: PillManager
    @State private var showingAddSheet = false
    @State private var editingSchedule: PillSchedule?

    var body: some View {
        NavigationView {
            List {
                ForEach(scheduleManager.schedules) { schedule in
                    ScheduleRow(schedule: schedule, scheduleManager: scheduleManager, pillManager: pillManager)
                        .contentShape(Rectangle())
                        .onTapGesture {
                            editingSchedule = schedule
                        }
                }
                .onDelete(perform: deleteSchedules)
            }
            .navigationTitle("Schedules")
            .toolbar {
                Button(action: { showingAddSheet = true }) {
                    Image(systemName: "plus")
                }
            }
            .sheet(isPresented: $showingAddSheet) {
                AddScheduleView(scheduleManager: scheduleManager, pillManager: pillManager)
            }
            .sheet(item: $editingSchedule) { schedule in
                EditScheduleView(scheduleManager: scheduleManager, pillManager: pillManager, schedule: schedule)
            }
            .overlay {
                if scheduleManager.schedules.isEmpty {
                    VStack(spacing: 20) {
                        Image(systemName: "calendar")
                            .font(.system(size: 70))
                            .foregroundColor(.gray)
                        Text("No Schedules")
                            .font(.title2)
                            .fontWeight(.semibold)
                        Text("Create schedules for your medications")
                            .foregroundColor(.secondary)
                            .multilineTextAlignment(.center)
                    }
                    .padding()
                }
            }
        }
    }

    private func deleteSchedules(at offsets: IndexSet) {
        for index in offsets {
            do {
                try scheduleManager.deleteSchedule(scheduleManager.schedules[index])
            } catch {
                scheduleManager.lastError = error
            }
        }
    }
}

#Preview {
    ScheduleView(scheduleManager: ScheduleManager(), bluetoothManager: BluetoothManager(), pillManager: PillManager())
}
