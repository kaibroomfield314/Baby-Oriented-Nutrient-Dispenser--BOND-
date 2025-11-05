import SwiftUI

struct DispenseView: View {
    @ObservedObject var bluetoothManager: BluetoothManager
    @ObservedObject var pillManager: PillManager
    @State private var selectedPill: Pill?
    @State private var pillCount = 1
    @State private var showingConfirmation = false
    @State private var dispenseMode: DispenseMode = .byPill
    @State private var isDispensing = false

    enum DispenseMode {
        case byPill
        case byCompartment
    }

    var body: some View {
        NavigationView {
            VStack(spacing: 20) {
                Picker("Dispense Mode", selection: $dispenseMode) {
                    Text("By Pill").tag(DispenseMode.byPill)
                    Text("By Compartment").tag(DispenseMode.byCompartment)
                }
                .pickerStyle(.segmented)
                .padding(.horizontal)

                if dispenseMode == .byPill {
                    pillDispenseView
                } else {
                    compartmentDispenseView
                }

                Spacer()
            }
            .navigationTitle("Pill Dispenser")
        }
    }

    var pillDispenseView: some View {
        VStack(spacing: 30) {
            Image(systemName: "cross.case.fill")
                .font(.system(size: 80))
                .foregroundColor(.blue)

            Text("Dispense by Pill")
                .font(.title2)
                .fontWeight(.bold)

            if pillManager.pills.isEmpty {
                VStack(spacing: 15) {
                    Image(systemName: "exclamationmark.triangle")
                        .font(.system(size: 50))
                        .foregroundColor(.orange)
                    Text("No pills configured")
                        .font(.headline)
                    Text("Add pills in the Pills tab first")
                        .font(.subheadline)
                        .foregroundColor(.secondary)
                }
                .padding()
            } else {
                VStack(spacing: 15) {
                    Menu {
                        ForEach(pillManager.pills) { pill in
                            Button(action: { selectedPill = pill }) {
                                HStack {
                                    Circle()
                                        .fill(pill.pillColour)
                                        .frame(width: 12, height: 12)
                                    Text(pill.name)
                                    Text("(Slot \(pill.compartment))")
                                        .foregroundColor(.secondary)
                                }
                            }
                        }
                    } label: {
                        HStack {
                            if let pill = selectedPill {
                                Circle()
                                    .fill(pill.pillColour)
                                    .frame(width: 20, height: 20)
                                Text(pill.name)
                                    .fontWeight(.semibold)
                            } else {
                                Text("Select Pill")
                                    .foregroundColor(.secondary)
                            }
                            Spacer()
                            Image(systemName: "chevron.down")
                        }
                        .padding()
                        .background(Color.gray.opacity(0.1))
                        .cornerRadius(12)
                    }

                    HStack {
                        Text("Quantity:")
                            .font(.headline)
                        Spacer()

                        Button(action: { if pillCount > 1 { pillCount -= 1 } }) {
                            Image(systemName: "minus.circle.fill")
                                .font(.title2)
                                .foregroundColor(.red)
                        }

                        Text("\(pillCount)")
                            .font(.title)
                            .fontWeight(.bold)
                            .frame(width: 60)

                        Button(action: { if pillCount < 5 { pillCount += 1 } }) {
                            Image(systemName: "plus.circle.fill")
                                .font(.title2)
                                .foregroundColor(.green)
                        }
                    }
                    .padding()
                    .background(Color.gray.opacity(0.1))
                    .cornerRadius(12)
                }
                .padding(.horizontal)

                Button(action: dispensePill) {
                    if isDispensing {
                        ProgressView()
                            .progressViewStyle(.circular)
                            .tint(.white)
                    } else {
                        HStack {
                            Image(systemName: "drop.fill")
                            Text("Dispense \(selectedPill?.name ?? "Pill")")
                                .fontWeight(.semibold)
                        }
                    }
                }
                .frame(maxWidth: .infinity)
                .padding()
                .background(canDispense ? Color.blue : Color.gray)
                .foregroundColor(.white)
                .cornerRadius(12)
                .disabled(!canDispense || isDispensing)
                .padding(.horizontal)

                if showingConfirmation {
                    Text("✓ Dispensing \(pillCount) \(selectedPill?.name ?? "pill")(s)...")
                        .font(.subheadline)
                        .foregroundColor(.green)
                        .transition(.opacity)
                }
            }

            statusSection
        }
    }

    var compartmentDispenseView: some View {
        VStack(spacing: 30) {
            Image(systemName: "square.grid.3x3.fill")
                .font(.system(size: 80))
                .foregroundColor(.purple)

            Text("Dispense by Compartment")
                .font(.title2)
                .fontWeight(.bold)

            VStack(spacing: 15) {
                HStack {
                    Text("Compartment:")
                        .font(.headline)
                    Spacer()

                    Picker("", selection: $pillCount) {
                        ForEach(1...5, id: \.self) { i in
                            Text("\(i)").tag(i)
                        }
                    }
                    .pickerStyle(.menu)
                }
                .padding()
                .background(Color.gray.opacity(0.1))
                .cornerRadius(12)
            }
            .padding(.horizontal)

            Button(action: dispenseByCompartment) {
                if isDispensing {
                    ProgressView()
                        .progressViewStyle(.circular)
                        .tint(.white)
                } else {
                    HStack {
                        Image(systemName: "drop.fill")
                        Text("Dispense from Compartment \(pillCount)")
                            .fontWeight(.semibold)
                    }
                }
            }
            .frame(maxWidth: .infinity)
            .padding()
            .background(bluetoothManager.isConnected ? Color.purple : Color.gray)
            .foregroundColor(.white)
            .cornerRadius(12)
            .disabled(!bluetoothManager.isConnected || isDispensing)
            .padding(.horizontal)

            if showingConfirmation {
                Text("✓ Dispensing from compartment \(pillCount)...")
                    .font(.subheadline)
                    .foregroundColor(.green)
                    .transition(.opacity)
            }

            statusSection
        }
    }

    var statusSection: some View {
        VStack(spacing: 10) {
            Text(bluetoothManager.statusMessage)
                .font(.subheadline)
                .foregroundColor(.secondary)
                .multilineTextAlignment(.center)
                .padding(.horizontal)

            if bluetoothManager.isConnected {
                HStack(spacing: 20) {
                    Button(action: requestStatus) {
                        Label("Status", systemImage: "chart.bar")
                            .font(.caption)
                            .padding(.horizontal, 12)
                            .padding(.vertical, 6)
                            .background(Color.blue.opacity(0.2))
                            .cornerRadius(8)
                    }

                    Button(action: resetStats) {
                        Label("Reset", systemImage: "arrow.clockwise")
                            .font(.caption)
                            .padding(.horizontal, 12)
                            .padding(.vertical, 6)
                            .background(Color.orange.opacity(0.2))
                            .cornerRadius(8)
                    }
                }
            }
        }
    }

    var canDispense: Bool {
        bluetoothManager.isConnected && selectedPill != nil && !isDispensing
    }

    func dispensePill() {
        guard let pill = selectedPill else { return }
        isDispensing = true
        Task {
            defer { isDispensing = false }

            do {
                try await bluetoothManager.dispensePill(compartment: pill.compartment, count: pillCount)
                showingConfirmation = true
                try? await Task.sleep(nanoseconds: 2_000_000_000)
                showingConfirmation = false
            } catch {
                bluetoothManager.lastError = error
            }
        }
    }

    func dispenseByCompartment() {
        isDispensing = true
        Task {
            defer { isDispensing = false }

            do {
                try await bluetoothManager.sendCommand("DISPENSE:\(pillCount)")
                showingConfirmation = true
                try? await Task.sleep(nanoseconds: 2_000_000_000)
                showingConfirmation = false
            } catch {
                bluetoothManager.lastError = error
            }
        }
    }

    func requestStatus() {
        Task {
            do {
                try await bluetoothManager.requestStatus()
            } catch {
                bluetoothManager.lastError = error
            }
        }
    }

    func resetStats() {
        Task {
            do {
                try await bluetoothManager.resetStatistics()
            } catch {
                bluetoothManager.lastError = error
            }
        }
    }
}

#Preview {
    DispenseView(bluetoothManager: BluetoothManager(), pillManager: PillManager())
}
