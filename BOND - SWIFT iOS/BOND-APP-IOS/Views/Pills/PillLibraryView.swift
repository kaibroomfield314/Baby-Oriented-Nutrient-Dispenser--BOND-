import SwiftUI

struct PillLibraryView: View {
    @ObservedObject var pillManager: PillManager
    @State private var showingAddSheet = false
    @State private var editingPill: Pill?

    var body: some View {
        NavigationView {
            List {
                ForEach(pillManager.pills) { pill in
                    HStack {
                        Circle()
                            .fill(pill.pillColour)
                            .frame(width: 40, height: 40)

                        VStack(alignment: .leading, spacing: 4) {
                            Text(pill.name)
                                .font(.headline)
                            VStack(alignment: .leading, spacing: 2) {
                                Text("Compartment \(pill.compartment)")
                                    .font(.caption)
                                    .foregroundColor(.secondary)
                                if !pill.dosage.isEmpty {
                                    Text(pill.dosage)
                                        .font(.caption)
                                        .foregroundColor(.secondary)
                                }
                            }
                        }

                        Spacer()
                    }
                    .padding(.vertical, 4)
                    .contentShape(Rectangle())
                    .onTapGesture {
                        editingPill = pill
                    }
                }
                .onDelete(perform: deletePills)
            }
            .navigationTitle("Pill Library")
            .toolbar {
                Button(action: { showingAddSheet = true }) {
                    Image(systemName: "plus")
                }
            }
            .sheet(isPresented: $showingAddSheet) {
                AddPillView(pillManager: pillManager)
            }
            .sheet(item: $editingPill) { pill in
                EditPillView(pillManager: pillManager, pill: pill)
            }
            .overlay {
                if pillManager.pills.isEmpty {
                    VStack(spacing: 20) {
                        Image(systemName: "pills")
                            .font(.system(size: 70))
                            .foregroundColor(.gray)
                        Text("No Pills Yet")
                            .font(.title2)
                            .fontWeight(.semibold)
                        Text("Add pills to organize your dispenser")
                            .foregroundColor(.secondary)
                            .multilineTextAlignment(.center)
                    }
                    .padding()
                }
            }
        }
    }

    private func deletePills(at offsets: IndexSet) {
        for index in offsets {
            do {
                try pillManager.deletePill(pillManager.pills[index])
            } catch {
                pillManager.lastError = error
            }
        }
    }
}

#Preview {
    PillLibraryView(pillManager: PillManager())
}
