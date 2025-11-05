import SwiftUI

struct SettingsView: View {
    @ObservedObject var userManager: UserManager
    @State private var showingUserSelection = false
    @State private var showingAddUser = false

    var body: some View {
        NavigationView {
            List {
                // Current User Section
                if let currentUser = userManager.currentUser {
                    Section(header: Text("Current User")) {
                        HStack {
                            VStack(alignment: .leading, spacing: 4) {
                                Text(currentUser.name)
                                    .font(.headline)
                                Text(currentUser.role.rawValue.capitalized)
                                    .font(.caption)
                                    .foregroundColor(.secondary)
                            }
                            Spacer()
                            Button(action: { showingUserSelection = true }) {
                                Text("Switch")
                                    .font(.caption)
                                    .padding(.horizontal, 12)
                                    .padding(.vertical, 6)
                                    .background(Color.blue.opacity(0.2))
                                    .cornerRadius(6)
                            }
                        }
                    }
                }

                // Appearance Section
                Section(header: Text("Appearance")) {
                    if let preferences = userManager.userPreferences {
                        Toggle("Dark Mode", isOn: Binding(
                            get: { preferences.isDarkModeEnabled },
                            set: { newValue in
                                var updatedPrefs = preferences
                                updatedPrefs.isDarkModeEnabled = newValue
                                try? userManager.updatePreferences(updatedPrefs)
                            }
                        ))

                        Toggle("Use Dynamic Type", isOn: Binding(
                            get: { preferences.useDynamicType },
                            set: { newValue in
                                var updatedPrefs = preferences
                                updatedPrefs.useDynamicType = newValue
                                try? userManager.updatePreferences(updatedPrefs)
                            }
                        ))
                    }
                }

                // Notifications Section
                Section(header: Text("Notifications")) {
                    if let preferences = userManager.userPreferences {
                        Toggle("Enable Notifications", isOn: Binding(
                            get: { preferences.enableNotifications },
                            set: { newValue in
                                var updatedPrefs = preferences
                                updatedPrefs.enableNotifications = newValue
                                try? userManager.updatePreferences(updatedPrefs)
                            }
                        ))

                        if preferences.enableNotifications {
                            Stepper(
                                "Advance Reminder: \(preferences.pillReminderAdvance) min",
                                value: Binding(
                                    get: { preferences.pillReminderAdvance },
                                    set: { newValue in
                                        var updatedPrefs = preferences
                                        updatedPrefs.pillReminderAdvance = newValue
                                        try? userManager.updatePreferences(updatedPrefs)
                                    }
                                ),
                                in: 0...60
                            )
                        }
                    }
                }

                // User Management Section
                Section(header: Text("User Management")) {
                    NavigationLink(destination: UserListView(userManager: userManager)) {
                        Text("Manage Users")
                    }

                    Button(action: { showingAddUser = true }) {
                        HStack {
                            Image(systemName: "plus.circle.fill")
                            Text("Add New User")
                        }
                        .foregroundColor(.blue)
                    }
                }

                // About Section
                Section(header: Text("About")) {
                    HStack {
                        Text("Version")
                        Spacer()
                        Text("1.0.0")
                            .foregroundColor(.secondary)
                    }

                    HStack {
                        Text("Build")
                        Spacer()
                        Text("1")
                            .foregroundColor(.secondary)
                    }
                }
            }
            .navigationTitle("Settings")
            .sheet(isPresented: $showingUserSelection) {
                UserSelectionView(userManager: userManager, isPresented: $showingUserSelection)
            }
            .sheet(isPresented: $showingAddUser) {
                AddUserView(userManager: userManager, isPresented: $showingAddUser)
            }
        }
    }
}

struct UserSelectionView: View {
    @ObservedObject var userManager: UserManager
    @Binding var isPresented: Bool

    var body: some View {
        NavigationView {
            List(userManager.allUsers) { user in
                Button(action: {
                    try? userManager.setCurrentUser(user)
                    isPresented = false
                }) {
                    HStack {
                        VStack(alignment: .leading, spacing: 4) {
                            Text(user.name)
                                .font(.headline)
                                .foregroundColor(.primary)
                            Text(user.role.rawValue.capitalized)
                                .font(.caption)
                                .foregroundColor(.secondary)
                        }
                        Spacer()
                        if userManager.currentUser?.id == user.id {
                            Image(systemName: "checkmark.circle.fill")
                                .foregroundColor(.blue)
                        }
                    }
                }
            }
            .navigationTitle("Select User")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("Done") { isPresented = false }
                }
            }
        }
    }
}

struct AddUserView: View {
    @ObservedObject var userManager: UserManager
    @Binding var isPresented: Bool
    @State private var name = ""
    @State private var selectedRole = User.UserRole.patient
    @State private var email = ""

    var body: some View {
        NavigationView {
            Form {
                Section(header: Text("User Information")) {
                    TextField("Name", text: $name)

                    Picker("Role", selection: $selectedRole) {
                        Text("Patient").tag(User.UserRole.patient)
                        Text("Caregiver").tag(User.UserRole.caregiver)
                        Text("Admin").tag(User.UserRole.admin)
                    }

                    TextField("Email (optional)", text: $email)
                        .keyboardType(.emailAddress)
                }
            }
            .navigationTitle("Add User")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarLeading) {
                    Button("Cancel") { isPresented = false }
                }
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("Save") {
                        do {
                            try userManager.createUser(name: name, role: selectedRole, email: email.isEmpty ? nil : email)
                            isPresented = false
                        } catch {
                            userManager.lastError = error
                        }
                    }
                    .disabled(name.isEmpty)
                }
            }
        }
    }
}

struct UserListView: View {
    @ObservedObject var userManager: UserManager

    var body: some View {
        List(userManager.allUsers) { user in
            NavigationLink(destination: UserDetailView(user: user, userManager: userManager)) {
                HStack {
                    VStack(alignment: .leading, spacing: 4) {
                        Text(user.name)
                            .font(.headline)
                        Text(user.role.rawValue.capitalized)
                            .font(.caption)
                            .foregroundColor(.secondary)
                    }
                    Spacer()
                    if userManager.currentUser?.id == user.id {
                        Text("Current")
                            .font(.caption2)
                            .padding(.horizontal, 8)
                            .padding(.vertical, 4)
                            .background(Color.blue.opacity(0.2))
                            .cornerRadius(4)
                    }
                }
            }
        }
        .navigationTitle("Users")
    }
}

struct UserDetailView: View {
    let user: User
    @ObservedObject var userManager: UserManager
    @Environment(\.dismiss) var dismiss

    var body: some View {
        List {
            Section(header: Text("Information")) {
                HStack {
                    Text("Name")
                    Spacer()
                    Text(user.name)
                        .foregroundColor(.secondary)
                }

                HStack {
                    Text("Role")
                    Spacer()
                    Text(user.role.rawValue.capitalized)
                        .foregroundColor(.secondary)
                }

                if let email = user.email {
                    HStack {
                        Text("Email")
                        Spacer()
                        Text(email)
                            .foregroundColor(.secondary)
                    }
                }

                HStack {
                    Text("Created")
                    Spacer()
                    Text(user.createdAt, style: .date)
                        .foregroundColor(.secondary)
                }
            }

            Section {
                Button(role: .destructive, action: {
                    try? userManager.deleteUser(user)
                    dismiss()
                }) {
                    HStack {
                        Image(systemName: "trash.fill")
                        Text("Delete User")
                    }
                }
            }
        }
        .navigationTitle(user.name)
    }
}

#Preview {
    SettingsView(userManager: UserManager())
}
