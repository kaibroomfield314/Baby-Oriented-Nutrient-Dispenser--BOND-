import SwiftUI

struct AddPillView: View {
    @ObservedObject var pillManager: PillManager
    @Environment(\.dismiss) var dismiss

    @State private var pillName = ""
    @State private var selectedColor = "blue"
    @State private var compartment = 1
    @State private var dosage = ""
    @State private var notes = ""

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

                Section(header: Text("Colour")) {
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
                    .frame(height: 200)
                }
            }
            .navigationTitle("Add Pill")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarLeading) {
                    Button("Cancel") { dismiss() }
                }
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("Save") {
                        savePill()
                    }
                    .disabled(pillName.isEmpty)
                }
            }
        }
    }

    private func savePill() {
        let pill = Pill(
            name: pillName,
            colour: selectedColor,
            compartment: compartment,
            dosage: dosage,
            notes: notes
        )
        do {
            try pillManager.addPill(pill)
            dismiss()
        } catch {
            pillManager.lastError = error
        }
    }
}

extension View {
    func placeholder(_ text: String) -> some View {
        self
    }
}

#Preview {
    AddPillView(pillManager: PillManager())
}
