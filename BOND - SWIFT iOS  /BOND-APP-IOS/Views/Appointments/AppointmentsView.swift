import SwiftUI

struct AppointmentsView: View {
    @ObservedObject var appointmentManager: AppointmentManager
    @State private var showingAddAppointment = false
    @State private var selectedTab = 0
    
    var body: some View {
        NavigationView {
            VStack {
                Picker("Appointments", selection: $selectedTab) {
                    Text("Upcoming").tag(0)
                    Text("Past").tag(1)
                }
                .pickerStyle(.segmented)
                .padding()
                
                List {
                    if selectedTab == 0 {
                        if appointmentManager.upcomingAppointments().isEmpty {
                            Text("No upcoming appointments")
                                .foregroundColor(.secondary)
                        } else {
                            ForEach(appointmentManager.upcomingAppointments()) { appointment in
                                NavigationLink(destination: AppointmentDetailView(appointment: appointment, appointmentManager: appointmentManager)) {
                                    AppointmentRow(appointment: appointment)
                                }
                            }
                        }
                    } else {
                        if appointmentManager.pastAppointments().isEmpty {
                            Text("No past appointments")
                                .foregroundColor(.secondary)
                        } else {
                            ForEach(appointmentManager.pastAppointments()) { appointment in
                                NavigationLink(destination: AppointmentDetailView(appointment: appointment, appointmentManager: appointmentManager)) {
                                    AppointmentRow(appointment: appointment)
                                }
                            }
                        }
                    }
                }
            }
            .navigationTitle("Appointments")
            .toolbar {
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button(action: { showingAddAppointment = true }) {
                        Image(systemName: "plus.circle.fill")
                    }
                }
            }
            .sheet(isPresented: $showingAddAppointment) {
                AddAppointmentView(appointmentManager: appointmentManager, isPresented: $showingAddAppointment)
            }
        }
    }
}

struct AppointmentRow: View {
    let appointment: Appointment
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                VStack(alignment: .leading, spacing: 4) {
                    Text(appointment.doctorName)
                        .font(.headline)
                    Text(appointment.specialty)
                        .font(.caption)
                        .foregroundColor(.secondary)
                }
                Spacer()
                Text(appointment.appointmentDate, style: .date)
                    .font(.caption2)
                    .foregroundColor(.secondary)
            }
            
            HStack(spacing: 12) {
                Label(appointment.hospitalName, systemImage: "building.2")
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
        }
        .padding(.vertical, 4)
    }
}

struct AppointmentDetailView: View {
    let appointment: Appointment
    @ObservedObject var appointmentManager: AppointmentManager
    @Environment(\.dismiss) var dismiss
    
    var body: some View {
        List {
            Section(header: Text("Doctor Information")) {
                HStack {
                    Text("Name")
                    Spacer()
                    Text(appointment.doctorName)
                        .foregroundColor(.secondary)
                }
                
                HStack {
                    Text("Specialty")
                    Spacer()
                    Text(appointment.specialty)
                        .foregroundColor(.secondary)
                }
                
                HStack {
                    Text("Hospital")
                    Spacer()
                    Text(appointment.hospitalName)
                        .foregroundColor(.secondary)
                }
                
                HStack {
                    Text("Phone")
                    Spacer()
                    Text(appointment.phoneNumber)
                        .foregroundColor(.blue)
                        .onTapGesture {
                            if let url = URL(string: "tel://\(appointment.phoneNumber)") {
                                UIApplication.shared.open(url)
                            }
                        }
                }
                
                if let email = appointment.email {
                    HStack {
                        Text("Email")
                        Spacer()
                        Text(email)
                            .foregroundColor(.blue)
                            .onTapGesture {
                                if let url = URL(string: "mailto:\(email)") {
                                    UIApplication.shared.open(url)
                                }
                            }
                    }
                }
            }
            
            Section(header: Text("Appointment Details")) {
                HStack {
                    Text("Date & Time")
                    Spacer()
                    VStack(alignment: .trailing, spacing: 2) {
                        Text(appointment.appointmentDate, style: .date)
                        Text(appointment.appointmentDate, style: .time)
                    }
                    .foregroundColor(.secondary)
                    .font(.caption)
                }
                
                if !appointment.notes.isEmpty {
                    HStack {
                        Text("Notes")
                        Spacer()
                        Text(appointment.notes)
                            .foregroundColor(.secondary)
                    }
                }
            }
            
            Section {
                Button(role: .destructive, action: {
                    try? appointmentManager.deleteAppointment(appointment)
                    dismiss()
                }) {
                    HStack {
                        Image(systemName: "trash.fill")
                        Text("Delete Appointment")
                    }
                }
            }
        }
        .navigationTitle("Appointment Details")
    }
}

struct AddAppointmentView: View {
    @ObservedObject var appointmentManager: AppointmentManager
    @Binding var isPresented: Bool
    @State private var doctorName = ""
    @State private var specialty = ""
    @State private var hospitalName = ""
    @State private var phoneNumber = ""
    @State private var email = ""
    @State private var appointmentDate = Date()
    @State private var notes = ""
    
    var body: some View {
        NavigationView {
            Form {
                Section(header: Text("Doctor Information")) {
                    TextField("Doctor Name", text: $doctorName)
                    TextField("Specialty", text: $specialty, prompt: Text("e.g., Cardiologist"))
                    TextField("Hospital/Clinic", text: $hospitalName)
                    TextField("Phone Number", text: $phoneNumber)
                    TextField("Email (optional)", text: $email)
                        .keyboardType(.emailAddress)
                }
                
                Section(header: Text("Appointment Details")) {
                    DatePicker("Date & Time", selection: $appointmentDate)
                    TextField("Notes (optional)", text: $notes, axis: .vertical)
                        .lineLimit(3...)
                }
            }
            .navigationTitle("Add Appointment")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarLeading) {
                    Button("Cancel") { isPresented = false }
                }
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("Save") {
                        do {
                            try appointmentManager.createAppointment(
                                doctorName: doctorName,
                                specialty: specialty,
                                hospitalName: hospitalName,
                                phoneNumber: phoneNumber,
                                email: email.isEmpty ? nil : email,
                                appointmentDate: appointmentDate,
                                notes: notes
                            )
                            isPresented = false
                        } catch {
                            appointmentManager.lastError = error
                        }
                    }
                    .disabled(doctorName.isEmpty || hospitalName.isEmpty)
                }
            }
        }
    }
}

#Preview {
    AppointmentsView(appointmentManager: AppointmentManager())
}
