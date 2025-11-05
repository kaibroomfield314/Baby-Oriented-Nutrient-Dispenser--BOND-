import Foundation
import Combine
import UserNotifications

enum AppointmentManagerError: LocalizedError {
    case appointmentNotFound
    case saveFailed(String)
    case loadFailed(String)
    
    var errorDescription: String? {
        switch self {
        case .appointmentNotFound:
            return "Appointment not found"
        case .saveFailed(let message):
            return "Failed to save appointment: \(message)"
        case .loadFailed(let message):
            return "Failed to load appointments: \(message)"
        }
    }
}

class AppointmentManager: ObservableObject {
    @Published var appointments: [Appointment] = []
    @Published var lastError: Error?
    
    private let appointmentsKey = "app_appointments"
    private let logger = Logger.shared
    
    init() {
        loadAppointments()
    }
    
    func createAppointment(doctorName: String, specialty: String, hospitalName: String, phoneNumber: String, email: String? = nil, appointmentDate: Date, notes: String = "") throws {
        let appointment = Appointment(doctorName: doctorName, specialty: specialty, hospitalName: hospitalName, phoneNumber: phoneNumber, email: email, appointmentDate: appointmentDate, notes: notes)
        appointments.append(appointment)
        try saveAppointments()
        
        if appointment.reminderEnabled {
            scheduleReminder(for: appointment)
        }
        
        logger.log("Created appointment with \(doctorName)", level: .info)
    }
    
    func updateAppointment(_ appointment: Appointment) throws {
        if let index = appointments.firstIndex(where: { $0.id == appointment.id }) {
            appointments[index] = appointment
            try saveAppointments()
            
            if appointment.reminderEnabled {
                scheduleReminder(for: appointment)
            }
            
            logger.log("Updated appointment with \(appointment.doctorName)", level: .info)
        }
    }
    
    func deleteAppointment(_ appointment: Appointment) throws {
        appointments.removeAll { $0.id == appointment.id }
        try saveAppointments()
        
        UNUserNotificationCenter.current().removePendingNotificationRequests(withIdentifiers: [appointment.id.uuidString])
        
        logger.log("Deleted appointment with \(appointment.doctorName)", level: .info)
    }
    
    func getAppointment(by id: UUID) -> Appointment? {
        appointments.first { $0.id == id }
    }
    
    func upcomingAppointments() -> [Appointment] {
        appointments.filter { $0.appointmentDate > Date() }
            .sorted { $0.appointmentDate < $1.appointmentDate }
    }
    
    func pastAppointments() -> [Appointment] {
        appointments.filter { $0.appointmentDate <= Date() }
            .sorted { $0.appointmentDate > $1.appointmentDate }
    }
    
    private func scheduleReminder(for appointment: Appointment) {
        let content = UNMutableNotificationContent()
        content.title = "Appointment Reminder"
        content.body = "Appointment with \(appointment.doctorName) in \(appointment.specialty)"
        content.sound = .default
        
        let triggerDate = appointment.appointmentDate.addingTimeInterval(TimeInterval(-appointment.reminderMinutesBefore * 60))
        let trigger = UNCalendarNotificationTrigger(dateMatching: Calendar.current.dateComponents([.year, .month, .day, .hour, .minute], from: triggerDate), repeats: false)
        let request = UNNotificationRequest(identifier: appointment.id.uuidString, content: content, trigger: trigger)
        
        UNUserNotificationCenter.current().add(request) { error in
            if let error = error {
                self.logger.log("Failed to schedule appointment reminder: \(error.localizedDescription)", level: .error)
            }
        }
    }
    
    private func saveAppointments() throws {
        do {
            let encoded = try JSONEncoder().encode(appointments)
            UserDefaults.standard.set(encoded, forKey: appointmentsKey)
        } catch {
            let error = AppointmentManagerError.saveFailed(error.localizedDescription)
            lastError = error
            logger.log("Save appointments failed: \(error.localizedDescription)", level: .error)
            throw error
        }
    }
    
    private func loadAppointments() {
        do {
            if let data = UserDefaults.standard.data(forKey: appointmentsKey) {
                let decoded = try JSONDecoder().decode([Appointment].self, from: data)
                DispatchQueue.main.async {
                    self.appointments = decoded
                    self.logger.log("Loaded \(decoded.count) appointments", level: .info)
                }
            }
        } catch {
            logger.log("Load appointments failed: \(error.localizedDescription)", level: .warning)
        }
    }
}
