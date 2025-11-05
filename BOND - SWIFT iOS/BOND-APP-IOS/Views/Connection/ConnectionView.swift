import SwiftUI
import CoreBluetooth

struct ConnectionView: View {
    @ObservedObject var bluetoothManager: BluetoothManager
    @State private var searchText = ""

    var filteredDevices: [CBPeripheral] {
        if searchText.isEmpty {
            return bluetoothManager.discoveredDevices
        } else {
            return bluetoothManager.discoveredDevices.filter { device in
                let name = device.name ?? "Unknown"
                let uuid = device.identifier.uuidString
                return name.localizedCaseInsensitiveContains(searchText) ||
                    uuid.localizedCaseInsensitiveContains(searchText)
            }
        }
    }

    var body: some View {
        NavigationView {
            VStack(spacing: 0) {
                // Connection Status Card
                VStack(spacing: 12) {
                    HStack {
                        Circle()
                            .fill(bluetoothManager.isConnected ? Color.green : Color.red)
                            .frame(width: 12, height: 12)

                        VStack(alignment: .leading, spacing: 2) {
                            Text(bluetoothManager.isConnected ? "Connected" : "Not Connected")
                                .font(.headline)
                            Text(bluetoothManager.statusMessage)
                                .font(.caption)
                                .foregroundColor(.secondary)
                        }

                        Spacer()
                    }
                    .padding()
                    .background(bluetoothManager.isConnected ? Color.green.opacity(0.1) : Color.red.opacity(0.1))
                    .cornerRadius(12)
                }
                .padding()

                // Search Bar
                HStack {
                    Image(systemName: "magnifyingglass")
                        .foregroundColor(.gray)
                    TextField("Search devices...", text: $searchText)
                        .textFieldStyle(.plain)

                    if !searchText.isEmpty {
                        Button(action: { searchText = "" }) {
                            Image(systemName: "xmark.circle.fill")
                                .foregroundColor(.gray)
                        }
                    }
                }
                .padding(10)
                .background(Color.gray.opacity(0.1))
                .cornerRadius(10)
                .padding()

                // Device List
                List(filteredDevices, id: \.identifier) { device in
                    Button(action: {
                        bluetoothManager.connect(to: device)
                    }) {
                        HStack {
                            Image(systemName: "antenna.radiowaves.left.and.right")
                                .foregroundColor(.blue)
                            VStack(alignment: .leading) {
                                Text(device.name ?? "Unknown Device")
                                    .fontWeight(.semibold)
                                    .foregroundColor(.primary)
                                Text(device.identifier.uuidString)
                                    .font(.caption)
                                    .foregroundColor(.secondary)
                            }
                            Spacer()
                            if device.name == "PillDispenser" || device.name == "NutrientDispenser" {
                                Image(systemName: "checkmark.circle.fill")
                                    .foregroundColor(.green)
                            }
                        }
                    }
                }

                // Action Buttons
                VStack(spacing: 10) {
                    if bluetoothManager.isConnected {
                        Button(action: {
                            bluetoothManager.disconnect()
                        }) {
                            HStack {
                                Image(systemName: "wifi.slash")
                                Text("Disconnect")
                            }
                            .frame(maxWidth: .infinity)
                            .padding()
                            .background(Color.red)
                            .foregroundColor(.white)
                            .cornerRadius(12)
                        }
                    } else {
                        Button(action: {
                            bluetoothManager.startScanning()
                        }) {
                            if bluetoothManager.isScanning {
                                HStack(spacing: 8) {
                                    ProgressView()
                                        .progressViewStyle(.circular)
                                        .tint(.white)
                                    Text("Scanning...")
                                }
                            } else {
                                HStack {
                                    Image(systemName: "magnifyingglass")
                                    Text("Scan for Devices")
                                }
                            }
                        }
                        .frame(maxWidth: .infinity)
                        .padding()
                        .background(Color.blue)
                        .foregroundColor(.white)
                        .cornerRadius(12)
                        .disabled(bluetoothManager.isScanning)
                    }

                    Text(bluetoothManager.statusMessage)
                        .font(.caption)
                        .foregroundColor(.secondary)
                        .multilineTextAlignment(.center)
                        .padding(.horizontal)
                }
                .padding()
            }
            .navigationTitle("Bluetooth Connection")
        }
    }
}

#Preview {
    ConnectionView(bluetoothManager: BluetoothManager())
}
