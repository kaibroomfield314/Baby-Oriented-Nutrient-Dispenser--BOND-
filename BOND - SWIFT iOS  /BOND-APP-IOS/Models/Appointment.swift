import Foundation

struct Appointment: Identifiable, Codable {
    let id: UUID
    var doctorName: String
    var specialty: String // e.g., "Cardiologist", "General Practitioner"
    var hospitalName: String
    var phoneNumber: String
    var email: String?
    var appointmentDate: Date
    var notes: String = ""
    var reminderEnabled: Bool = true
    var reminderMinutesBefore: Int = 60 // Default 1 hour before
    var createdAt: Date = Date()
    
    init(doctorName: String, specialty: String, hospitalName: String, phoneNumber: String, email: String? = nil, appointmentDate: Date, notes: String = "", reminderEnabled: Bool = true) {
        self.id = UUID()
        self.doctorName = doctorName
        self.specialty = specialty
        self.hospitalName = hospitalName
        self.phoneNumber = phoneNumber
        self.email = email
        self.appointmentDate = appointmentDate
        self.notes = notes
        self.reminderEnabled = reminderEnabled
    }
}
