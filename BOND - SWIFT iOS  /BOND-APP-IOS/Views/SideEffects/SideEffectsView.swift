import SwiftUI

struct SideEffectsView: View {
    @ObservedObject var sideEffectManager: SideEffectManager
    @ObservedObject var pillManager: PillManager
    @State private var showingAddSideEffect = false
    
    var body: some View {
        NavigationView {
            VStack {
                if sideEffectManager.sideEffects.isEmpty {
                    VStack(spacing: 16) {
                        Image(systemName: "checkmark.circle")
                            .font(.system(size: 48))
                            .foregroundColor(.green)
                        Text("No Side Effects Logged")
                            .font(.headline)
                        Text("Start logging any side effects you experience")
                            .foregroundColor(.secondary)
                    }
                    .frame(maxHeight: .infinity, alignment: .center)
                } else {
                    List {
                        Section(header: Text("Most Common Side Effects")) {
                            ForEach(sideEffectManager.getMostCommonSideEffects()) { summary in
                                VStack(alignment: .leading, spacing: 4) {
                                    HStack {
                                        Text(summary.name)
                                            .font(.headline)
                                        Spacer()
                                        Text("\(summary.count)x")
                                            .font(.caption)
                                            .padding(.horizontal, 8)
                                            .padding(.vertical, 4)
                                            .background(Color.blue.opacity(0.2))
                                            .cornerRadius(4)
                                    }
                                    
                                    HStack {
                                        Text("Avg Severity:")
                                            .font(.caption)
                                            .foregroundColor(.secondary)
                                        ProgressView(value: summary.averageSeverity / 5.0)
                                            .frame(height: 6)
                                    }
                                }
                            }
                        }
                        
                        Section(header: Text("All Side Effects")) {
                            ForEach(sideEffectManager.sideEffects.sorted { $0.dateLogged > $1.dateLogged }) { sideEffect in
                                NavigationLink(destination: SideEffectDetailView(sideEffect: sideEffect, sideEffectManager: sideEffectManager, pillManager: pillManager)) {
                                    SideEffectRow(sideEffect: sideEffect, pillManager: pillManager)
                                }
                            }
                            .onDelete { indices in
                                for index in indices {
                                    let sideEffect = sideEffectManager.sideEffects.sorted { $0.dateLogged > $1.dateLogged }[index]
                                    try? sideEffectManager.deleteSideEffect(sideEffect)
                                }
                            }
                        }
                    }
                }
            }
            .navigationTitle("Side Effects")
            .toolbar {
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button(action: { showingAddSideEffect = true }) {
                        Image(systemName: "plus.circle.fill")
                    }
                }
            }
            .sheet(isPresented: $showingAddSideEffect) {
                AddSideEffectView(sideEffectManager: sideEffectManager, pillManager: pillManager, isPresented: $showingAddSideEffect)
            }
        }
    }
}

struct SideEffectRow: View {
    let sideEffect: SideEffect
    @ObservedObject var pillManager: PillManager
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                VStack(alignment: .leading, spacing: 4) {
                    Text(sideEffect.effectName)
                        .font(.headline)
                    if let pill = pillManager.getPill(by: sideEffect.pillId) {
                        Text(pill.name)
                            .font(.caption)
                            .foregroundColor(.secondary)
                    }
                }
                Spacer()
                VStack(alignment: .trailing, spacing: 2) {
                    Text(sideEffect.severity.description)
                        .font(.caption2)
                        .padding(.horizontal, 8)
                        .padding(.vertical, 4)
                        .background(severityColor(sideEffect.severity).opacity(0.2))
                        .cornerRadius(4)
                    Text(sideEffect.dateLogged, style: .date)
                        .font(.caption2)
                        .foregroundColor(.secondary)
                }
            }
            
            if !sideEffect.managedWith.isEmpty {
                HStack(spacing: 4) {
                    Image(systemName: "checkmark.circle.fill")
                        .font(.caption)
                        .foregroundColor(.green)
                    Text("Managed with: \(sideEffect.managedWith)")
                        .font(.caption)
                        .foregroundColor(.secondary)
                }
            }
        }
        .padding(.vertical, 4)
    }
    
    private func severityColor(_ severity: SideEffect.Severity) -> Color {
        switch severity {
        case .mild: return .green
        case .moderate: return .yellow
        case .moderate_severe: return .orange
        case .severe: return .red
        case .very_severe: return .red
        }
    }
}

struct SideEffectDetailView: View {
    let sideEffect: SideEffect
    @ObservedObject var sideEffectManager: SideEffectManager
    @ObservedObject var pillManager: PillManager
    @Environment(\.dismiss) var dismiss
    
    var body: some View {
        List {
            Section(header: Text("Effect Details")) {
                HStack {
                    Text("Name")
                    Spacer()
                    Text(sideEffect.effectName)
                        .foregroundColor(.secondary)
                }
                
                HStack {
                    Text("Severity")
                    Spacer()
                    Text(sideEffect.severity.description)
                        .padding(.horizontal, 8)
                        .padding(.vertical, 4)
                        .background(severityColor(sideEffect.severity).opacity(0.2))
                        .cornerRadius(4)
                }
                
                HStack {
                    Text("Date")
                    Spacer()
                    Text(sideEffect.dateLogged, style: .date)
                        .foregroundColor(.secondary)
                }
            }
            
            if let pill = pillManager.getPill(by: sideEffect.pillId) {
                Section(header: Text("Related Medication")) {
                    HStack {
                        Text("Medication")
                        Spacer()
                        Text(pill.name)
                            .foregroundColor(.secondary)
                    }
                    
                    HStack {
                        Text("Dosage")
                        Spacer()
                        Text(pill.dosage)
                            .foregroundColor(.secondary)
                    }
                }
            }
            
            if !sideEffect.notes.isEmpty {
                Section(header: Text("Notes")) {
                    Text(sideEffect.notes)
                }
            }
            
            if !sideEffect.managedWith.isEmpty {
                Section(header: Text("Management")) {
                    Text(sideEffect.managedWith)
                }
            }
            
            Section {
                Button(role: .destructive, action: {
                    try? sideEffectManager.deleteSideEffect(sideEffect)
                    dismiss()
                }) {
                    HStack {
                        Image(systemName: "trash.fill")
                        Text("Delete Side Effect")
                    }
                }
            }
        }
        .navigationTitle("Side Effect Details")
    }
    
    private func severityColor(_ severity: SideEffect.Severity) -> Color {
        switch severity {
        case .mild: return .green
        case .moderate: return .yellow
        case .moderate_severe: return .orange
        case .severe: return .red
        case .very_severe: return .red
        }
    }
}

struct AddSideEffectView: View {
    @ObservedObject var sideEffectManager: SideEffectManager
    @ObservedObject var pillManager: PillManager
    @Binding var isPresented: Bool
    @State private var effectName = ""
    @State private var selectedSeverity = SideEffect.Severity.moderate
    @State private var notes = ""
    @State private var managedWith = ""
    @State private var selectedPillId: UUID?
    
    var body: some View {
        NavigationView {
            Form {
                Section(header: Text("Medication")) {
                    Picker("Related Medication", selection: $selectedPillId) {
                        Text("None").tag(UUID?(nil))
                        ForEach(pillManager.pills) { pill in
                            Text(pill.name).tag(UUID?(pill.id))
                        }
                    }
                }
                
                Section(header: Text("Side Effect Details")) {
                    TextField("Effect Name", text: $effectName, prompt: Text("e.g., Nausea, Headache"))
                    
                    Picker("Severity", selection: $selectedSeverity) {
                        Text("Mild").tag(SideEffect.Severity.mild)
                        Text("Moderate").tag(SideEffect.Severity.moderate)
                        Text("Moderate-Severe").tag(SideEffect.Severity.moderate_severe)
                        Text("Severe").tag(SideEffect.Severity.severe)
                        Text("Very Severe").tag(SideEffect.Severity.very_severe)
                    }
                    
                    TextField("Notes (optional)", text: $notes, axis: .vertical)
                        .lineLimit(3...)
                    
                    TextField("Managed with (optional)", text: $managedWith, prompt: Text("e.g., Ginger tea, Rest"))
                }
            }
            .navigationTitle("Log Side Effect")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarLeading) {
                    Button("Cancel") { isPresented = false }
                }
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("Save") {
                        do {
                            try sideEffectManager.logSideEffect(
                                pillId: selectedPillId ?? UUID(),
                                effectName: effectName,
                                severity: selectedSeverity,
                                notes: notes,
                                managedWith: managedWith
                            )
                            isPresented = false
                        } catch {
                            sideEffectManager.lastError = error
                        }
                    }
                    .disabled(effectName.isEmpty)
                }
            }
        }
    }
}

#Preview {
    SideEffectsView(sideEffectManager: SideEffectManager(), pillManager: PillManager())
}
