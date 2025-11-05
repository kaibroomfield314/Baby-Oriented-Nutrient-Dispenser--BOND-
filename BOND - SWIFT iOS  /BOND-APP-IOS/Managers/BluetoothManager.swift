import CoreBluetooth
import Combine
import Foundation

enum BluetoothError: LocalizedError {
    case notInitialized
    case notConnected
    case notReady
    case characteristicNotFound
    case commandFailed(String)
    case timeoutError

    var errorDescription: String? {
        switch self {
        case .notInitialized:
            return "Bluetooth is not initialized"
        case .notConnected:
            return "Not connected to a device"
        case .notReady:
            return "Device is not ready"
        case .characteristicNotFound:
            return "Characteristic not found"
        case .commandFailed(let message):
            return "Command failed: \(message)"
        case .timeoutError:
            return "Command timed out"
        }
    }
}

class BluetoothManager: NSObject, ObservableObject {
    @Published var isConnected = false
    @Published var discoveredDevices: [CBPeripheral] = []
    @Published var statusMessage = "Initializing Bluetooth..."
    @Published var lastResponse = ""
    @Published var lastError: Error?
    @Published var isScanning = false
    @Published var isReconnecting = false

    private var centralManager: CBCentralManager!
    private var connectedPeripheral: CBPeripheral?
    private var txCharacteristic: CBCharacteristic?
    private var isBluetoothReady = false
    private var commandTimeoutTimer: Timer?
    private var reconnectionTimer: Timer?
    private var commandContinuation: CheckedContinuation<Void, Error>?
    private let logger = Logger.shared

    // Persistent storage for last connected device
    private let lastConnectedPeripheralUUIDKey = "lastConnectedPeripheralUUID"
    private var lastConnectedPeripheralUUID: UUID? {
        get {
            if let uuidString = UserDefaults.standard.string(forKey: lastConnectedPeripheralUUIDKey) {
                return UUID(uuidString: uuidString)
            }
            return nil
        }
        set {
            if let uuid = newValue {
                UserDefaults.standard.set(uuid.uuidString, forKey: lastConnectedPeripheralUUIDKey)
            } else {
                UserDefaults.standard.removeObject(forKey: lastConnectedPeripheralUUIDKey)
            }
        }
    }

    private var reconnectAttempts = 0
    private let maxReconnectAttempts = 5

    // ESP32 Service and Characteristic UUIDs
    private let serviceUUID = CBUUID(string: "4fafc201-1fb5-459e-8fcc-c5c9c331914b")
    private let txCharacteristicUUID = CBUUID(string: "beb5483e-36e1-4688-b7f5-ea07361b26a8")

    override init() {
        super.init()
        centralManager = CBCentralManager(delegate: self, queue: nil)
    }

    // MARK: - Scanning
    func startScanning() {
        guard isBluetoothReady else {
            updateStatus("Bluetooth not ready. Please enable Bluetooth.", error: BluetoothError.notReady)
            return
        }

        DispatchQueue.main.async {
            self.discoveredDevices.removeAll()
            self.isScanning = true
            self.statusMessage = "Scanning..."
        }
        centralManager.scanForPeripherals(withServices: nil, options: nil)
        logger.log("Started Bluetooth scanning", level: .info)
    }

    func stopScanning() {
        centralManager.stopScan()
        DispatchQueue.main.async {
            self.isScanning = false
            self.statusMessage = "Scan stopped"
        }
        logger.log("Stopped Bluetooth scanning", level: .info)
    }

    // MARK: - Connection
    func connect(to peripheral: CBPeripheral) {
        connectedPeripheral = peripheral
        lastConnectedPeripheralUUID = peripheral.identifier
        reconnectAttempts = 0
        centralManager.connect(peripheral, options: nil)
        DispatchQueue.main.async {
            self.statusMessage = "Connecting..."
        }
        logger.log("Connecting to device: \(peripheral.name ?? "Unknown")", level: .info)
    }

    func disconnect() {
        guard let peripheral = connectedPeripheral else { return }
        lastConnectedPeripheralUUID = nil
        reconnectAttempts = 0
        stopReconnectionTimer()
        centralManager.cancelPeripheralConnection(peripheral)
        logger.log("Disconnecting from device", level: .info)
    }

    /// Automatically reconnect to the last connected device
    func reconnectToLastDevice() {
        guard let lastUUID = lastConnectedPeripheralUUID else {
            logger.log("No last connected device found", level: .info)
            return
        }

        logger.log("Attempting to reconnect to device: \(lastUUID)", level: .info)

        // First, try to find the peripheral in already discovered devices
        if let peripheral = discoveredDevices.first(where: { $0.identifier == lastUUID }) {
            connect(to: peripheral)
            return
        }

        // If not found, start scanning for it
        DispatchQueue.main.async {
            self.isReconnecting = true
            self.statusMessage = "Reconnecting..."
        }

        reconnectAttempts = 0
        startScanningForLastDevice()
    }

    private func startScanningForLastDevice() {
        guard isBluetoothReady else { return }

        centralManager.scanForPeripherals(withServices: nil, options: nil)

        // Set a timer to stop scanning and retry if not found
        reconnectionTimer = Timer.scheduledTimer(withTimeInterval: 5.0, repeats: false) { [weak self] _ in
            self?.handleReconnectTimeout()
        }
    }

    private func handleReconnectTimeout() {
        reconnectAttempts += 1

        if reconnectAttempts < maxReconnectAttempts {
            logger.log("Reconnection attempt \(reconnectAttempts) failed, retrying...", level: .warning)
            startScanningForLastDevice()
        } else {
            stopReconnectionTimer()
            logger.log("Max reconnection attempts reached", level: .error)
            DispatchQueue.main.async {
                self.isReconnecting = false
                self.statusMessage = "Failed to reconnect"
            }
        }
    }

    private func stopReconnectionTimer() {
        reconnectionTimer?.invalidate()
        reconnectionTimer = nil
    }

    // MARK: - Commands with Retry Logic
    func sendCommand(_ command: String, retryCount: Int = 3) async throws {
        guard isConnected else {
            throw BluetoothError.notConnected
        }

        guard let peripheral = connectedPeripheral,
              let characteristic = txCharacteristic else {
            throw BluetoothError.characteristicNotFound
        }

        guard let data = command.data(using: .utf8) else {
            throw BluetoothError.commandFailed("Invalid command format")
        }

        var lastError: Error?
        for attempt in 1...retryCount {
            do {
                try await performWrite(data: data, for: characteristic, on: peripheral)
                DispatchQueue.main.async {
                    self.statusMessage = "Sent: \(command)"
                }
                logger.log("Command sent: \(command)", level: .info)
                return
            } catch {
                lastError = error
                logger.log("Command attempt \(attempt) failed: \(error.localizedDescription)", level: .warning)
                if attempt < retryCount {
                    try? await Task.sleep(nanoseconds: 500_000_000) // 0.5 second delay
                }
            }
        }

        if let error = lastError {
            updateStatus("Command failed", error: error)
            throw error
        }
    }

    func dispensePill(compartment: Int, count: Int) async throws {
        let command = "DISPENSE:\(compartment):\(count)"
        try await sendCommand(command)
    }

    func requestStatus() async throws {
        try await sendCommand("STATUS")
    }

    func resetStatistics() async throws {
        try await sendCommand("RESET")
    }

    // MARK: - Private Helpers
    private func performWrite(data: Data, for characteristic: CBCharacteristic, on peripheral: CBPeripheral) async throws {
        return try await withCheckedThrowingContinuation { continuation in
            DispatchQueue.main.async {
                self.commandContinuation = continuation
                peripheral.writeValue(data, for: characteristic, type: .withResponse)
                // Set timeout
                self.commandTimeoutTimer?.invalidate()
                self.commandTimeoutTimer = Timer.scheduledTimer(withTimeInterval: 10, repeats: false) { [weak self] _ in
                    self?.commandTimeoutTimer?.invalidate()
                    if let cont = self?.commandContinuation {
                        self?.commandContinuation = nil
                        cont.resume(throwing: BluetoothError.timeoutError)
                    }
                }
            }
        }
    }

    private func updateStatus(_ message: String, error: Error? = nil) {
        DispatchQueue.main.async {
            self.statusMessage = message
            self.lastError = error
        }
        if let error = error {
            logger.log(message, level: .error)
        }
    }
}

// MARK: - CBCentralManagerDelegate
extension BluetoothManager: CBCentralManagerDelegate {
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        DispatchQueue.main.async {
            switch central.state {
            case .poweredOn:
                self.isBluetoothReady = true
                self.statusMessage = "Bluetooth ready"
                self.logger.log("Bluetooth powered on", level: .info)
            case .poweredOff:
                self.isBluetoothReady = false
                self.statusMessage = "Bluetooth is off"
                self.updateStatus("Bluetooth is off", error: BluetoothError.notReady)
            case .unauthorized:
                self.isBluetoothReady = false
                self.updateStatus("Bluetooth not authorized", error: BluetoothError.notReady)
            case .unsupported:
                self.isBluetoothReady = false
                self.updateStatus("Bluetooth not supported", error: BluetoothError.notReady)
            default:
                self.isBluetoothReady = false
                self.updateStatus("Bluetooth unavailable", error: BluetoothError.notReady)
            }
        }
    }

    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral, advertisementData: [String: Any], rssi RSSI: NSNumber) {
        DispatchQueue.main.async {
            if !self.discoveredDevices.contains(where: { $0.identifier == peripheral.identifier }) {
                self.discoveredDevices.append(peripheral)
                self.logger.log("Discovered device: \(peripheral.name ?? "Unknown")", level: .info)
            }

            // If we're reconnecting and found the target device, connect to it immediately
            if self.isReconnecting, peripheral.identifier == self.lastConnectedPeripheralUUID {
                self.logger.log("Found target device during reconnection: \(peripheral.name ?? "Unknown")", level: .info)
                self.centralManager.stopScan()
                self.stopReconnectionTimer()
                self.connect(to: peripheral)
            }
        }
    }

    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        DispatchQueue.main.async {
            self.isConnected = true
            self.isReconnecting = false
            self.reconnectAttempts = 0
            self.stopReconnectionTimer()
            self.statusMessage = "Connected to \(peripheral.name ?? "Device")"
            self.logger.log("Connected to device: \(peripheral.name ?? "Unknown")", level: .info)
        }
        peripheral.delegate = self
        peripheral.discoverServices([serviceUUID])
    }

    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
        DispatchQueue.main.async {
            self.isConnected = false
            if let error = error {
                self.updateStatus("Disconnected with error, attempting to reconnect...", error: error)
                self.logger.log("Disconnected with error: \(error.localizedDescription)", level: .warning)
                // Attempt automatic reconnection on unexpected disconnect
                self.reconnectToLastDevice()
            } else {
                self.statusMessage = "Disconnected"
                self.logger.log("Disconnected from device", level: .info)
            }
            self.connectedPeripheral = nil
            self.txCharacteristic = nil
        }
    }

    func centralManager(_ central: CBCentralManager, didFailToConnect peripheral: CBPeripheral, error: Error?) {
        updateStatus("Failed to connect", error: error)
        logger.log("Failed to connect: \(error?.localizedDescription ?? "Unknown error")", level: .error)
    }
}

// MARK: - CBPeripheralDelegate
extension BluetoothManager: CBPeripheralDelegate {
    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        if let error = error {
            logger.log("Service discovery failed: \(error.localizedDescription)", level: .error)
            return
        }

        guard let services = peripheral.services else { return }
        for service in services {
            peripheral.discoverCharacteristics([txCharacteristicUUID], for: service)
        }
    }

    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        if let error = error {
            logger.log("Characteristic discovery failed: \(error.localizedDescription)", level: .error)
            return
        }

        guard let characteristics = service.characteristics else { return }
        for characteristic in characteristics {
            if characteristic.uuid == txCharacteristicUUID {
                txCharacteristic = characteristic
                peripheral.setNotifyValue(true, for: characteristic)
                DispatchQueue.main.async {
                    self.statusMessage = "Ready to dispense"
                    self.logger.log("Ready to dispense", level: .info)
                }
            }
        }
    }

    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
        if let error = error {
            logger.log("Update value failed: \(error.localizedDescription)", level: .error)
            return
        }

        guard let data = characteristic.value,
              let message = String(data: data, encoding: .utf8) else { return }

        DispatchQueue.main.async {
            self.lastResponse = message
            self.statusMessage = "Response: \(message)"
            self.logger.log("Received response: \(message)", level: .info)
        }

        commandTimeoutTimer?.invalidate()
    }

    func peripheral(_ peripheral: CBPeripheral, didWriteValueFor characteristic: CBCharacteristic, error: Error?) {
        commandTimeoutTimer?.invalidate()
        
        if let error = error {
            logger.log("Write failed: \(error.localizedDescription)", level: .error)
            if let cont = commandContinuation {
                commandContinuation = nil
                cont.resume(throwing: error)
            }
        } else {
            if let cont = commandContinuation {
                commandContinuation = nil
                cont.resume()
            }
        }
    }
}
