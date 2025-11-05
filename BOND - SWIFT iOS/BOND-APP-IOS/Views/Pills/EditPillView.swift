import SwiftUI

struct EditPillView: View {
    @ObservedObject var pillManager: PillManager
    @Environment(\.dismiss) var dismiss

    let pill: Pill
    @State private var pillName = ""
    @State private var selectedColor = "blue"
    @State private var compartment = 1
    @State private var dosage = ""
    @State private var notes = ""

    init(pillManager: PillManager, pill: Pill) {
        self.pillManager = pillManager
        self.pill = pill
        _pillName = State(initialValue: pill.name)
        _selectedColor = State(initialValue: pill.colour)
        _compartment = State(initialValue: pill.compartment)
        _dosage = State(initialValue: pill.dosage)
        _notes = State(initialValue: pill.notes)
    }

    var body: some View {
        NavigationView {
            Form {
                Section(header: Text("Pill Information")) {
                    TextField("Pill Name", text: $pillName)

                    Picker("Compartment", selection: $compartment) {
                        ForEach(1...5, id: \.self) { i in
                            Text("Slot \(i)").tag(i)
                        }
                    }
                }

                Section(header: Text("Details")) {
                    TextField("Dosage (optional)", text: $dosage)
                        .placeholder("e.g., 500mg")

                    TextField("Notes (optional)", text: $notes)
                        .placeholder("e.g., Take with water")
                }

                Section(header: Text("Color")) {
                    ScrollView {
                        LazyVGrid(columns: [GridItem(.adaptive(minimum: 60))], spacing: 15) {
                            ForEach(ColorHelper.availableColors, id: \.self) { color in
                                Circle()
                                    .fill(ColorHelper.colorFromString(color))
                                    .frame(width: 50, height: 50)
                                    .overlay(
                                        Circle()
                                            .stroke(selectedColor == color ? Color.primary : Color.clear, lineWidth: 3)
                                    )
                                    .onTapGesture {
                                        selectedColor = color
                                    }
                            }
                        }
                        .padding(.vertical, 8)
                    }
                    .frame(height: 150)
                }
            }
            .navigationTitle("Edit Pill")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarLeading) {
                    Button("Cancel") { dismiss() }
                }
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("Save") {
                        updatePill()
                    }
                    .disabled(pillName.isEmpty)
                }
            }
        }
    }

    private func updatePill() {
        var updatedPill = pill
        updatedPill.name = pillName
        updatedPill.colour = selectedColor
        updatedPill.compartment = compartment
        updatedPill.dosage = dosage
        updatedPill.notes = notes

        do {
            try pillManager.updatePill(updatedPill)
            dismiss()
        } catch {
            pillManager.lastError = error
        }
    }
}

#Preview {
    EditPillView(pillManager: PillManager(), pill: Pill(name: "Test", colour: "blue", compartment: 1))
}
