import SwiftUI

struct SymptomsView: View {
    @ObservedObject var symptomManager: SymptomManager
    @ObservedObject var pillManager: PillManager
    @State private var showingAddSymptom = false
    @State private var selectedTimeframe = 7
    
    var body: some View {
        NavigationView {
            VStack {
                Picker("Timeframe", selection: $selectedTimeframe) {
                    Text("Last 7 days").tag(7)
                    Text("Last 30 days").tag(30)
                    Text("All time").tag(365)
                }
                .pickerStyle(.segmented)
                .padding()
                
                if symptomManager.symptoms.isEmpty {
                    VStack(spacing: 16) {
                        Image(systemName: "smiley")
                            .font(.system(size: 48))
                            .foregroundColor(.green)
                        Text("No Symptoms Logged")
                            .font(.headline)
                        Text("Start tracking symptoms to see trends")
                            .foregroundColor(.secondary)
                    }
                    .frame(maxHeight: .infinity, alignment: .center)
                } else {
                    List {
                        Section(header: Text("Most Common Symptoms")) {
                            ForEach(symptomManager.getMostCommonSymptoms()) { summary in
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
                                    
                                    Text("Last: \(summary.lastOccurrence, style: .date)")
                                        .font(.caption2)
                                        .foregroundColor(.secondary)
                                }
                            }
                        }
                        
                        Section(header: Text("Recent Symptoms")) {
                            ForEach(symptomManager.getSymptomsSince(days: selectedTimeframe).sorted { $0.dateLogged > $1.dateLogged }) { symptom in
                                NavigationLink(destination: SymptomDetailView(symptom: symptom, symptomManager: symptomManager, pillManager: pillManager)) {
                                    SymptomRow(symptom: symptom, pillManager: pillManager)
                                }
                            }
                            .onDelete { indices in
                                for index in indices {
                                    let symptom = symptomManager.getSymptomsSince(days: selectedTimeframe).sorted { $0.dateLogged > $1.dateLogged }[index]
                                    try? symptomManager.deleteSymptom(symptom)
                                }
                            }
                        }
                    }
                }
            }
            .navigationTitle("Symptoms")
            .toolbar {
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button(action: { showingAddSymptom = true }) {
                        Image(systemName: "plus.circle.fill")
                    }
                }
            }
            .sheet(isPresented: $showingAddSymptom) {
                AddSymptomView(symptomManager: symptomManager, pillManager: pillManager, isPresented: $showingAddSymptom)
            }
        }
    }
}

struct SymptomRow: View {
    let symptom: Symptom
    @ObservedObject var pillManager: PillManager
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                VStack(alignment: .leading, spacing: 4) {
                    Text(symptom.symptomName)
                        .font(.headline)
                }
                Spacer()
                VStack(alignment: .trailing, spacing: 2) {
                    HStack(spacing: 4) {
                        ForEach(0..<symptom.severity.rawValue, id: \.self) { _ in
                            Image(systemName: "circle.fill")
                                .font(.caption)
                                .foregroundColor(severityColor(symptom.severity))
                        }
                    }
                    Text(symptom.dateLogged, style: .date)
                        .font(.caption2)
                        .foregroundColor(.secondary)
                }
            }
            
            if !symptom.relatedMedications.isEmpty {
                HStack(spacing: 4) {
                    Image(systemName: "pill")
                        .font(.caption)
                        .foregroundColor(.blue)
                    Text("Related to \(symptom.relatedMedications.count) med(s)")
                        .font(.caption)
                        .foregroundColor(.secondary)
                }
            }
        }
        .padding(.vertical, 4)
    }
    
    private func severityColor(_ severity: Symptom.Severity) -> Color {
        switch severity {
        case .mild: return .green
        case .moderate: return .yellow
        case .moderate_severe: return .orange
        case .severe: return .red
        case .very_severe: return .red
        }
    }
}

struct SymptomDetailView: View {
    let symptom: Symptom
    @ObservedObject var symptomManager: SymptomManager
    @ObservedObject var pillManager: PillManager
    @Environment(\.dismiss) var dismiss
    
    var body: some View {
        List {
            Section(header: Text("Symptom Details")) {
                HStack {
                    Text("Name")
                    Spacer()
                    Text(symptom.symptomName)
                        .foregroundColor(.secondary)
                }
                
                HStack {
                    Text("Severity")
                    Spacer()
                    HStack(spacing: 4) {
                        ForEach(0..<symptom.severity.rawValue, id: \.self) { _ in
                            Image(systemName: "circle.fill")
                                .font(.caption)
                                .foregroundColor(severityColor(symptom.severity))
                        }
                    }
                }
                
                HStack {
                    Text("Date")
                    Spacer()
                    Text(symptom.dateLogged, style: .date)
                        .foregroundColor(.secondary)
                }
            }
            
            if !symptom.relatedMedications.isEmpty {
                Section(header: Text("Related Medications")) {
                    ForEach(symptom.relatedMedications, id: \.self) { medicationId in
                        if let pill = pillManager.getPill(by: medicationId) {
                            HStack {
                                Text(pill.name)
                                Spacer()
                                Text(pill.dosage)
                                    .foregroundColor(.secondary)
                            }
                        }
                    }
                }
            }
            
            if !symptom.notes.isEmpty {
                Section(header: Text("Notes")) {
                    Text(symptom.notes)
                }
            }
            
            Section {
                Button(role: .destructive, action: {
                    try? symptomManager.deleteSymptom(symptom)
                    dismiss()
                }) {
                    HStack {
                        Image(systemName: "trash.fill")
                        Text("Delete Symptom")
                    }
                }
            }
        }
        .navigationTitle("Symptom Details")
    }
    
    private func severityColor(_ severity: Symptom.Severity) -> Color {
        switch severity {
        case .mild: return .green
        case .moderate: return .yellow
        case .moderate_severe: return .orange
        case .severe: return .red
        case .very_severe: return .red
        }
    }
}

struct AddSymptomView: View {
    @ObservedObject var symptomManager: SymptomManager
    @ObservedObject var pillManager: PillManager
    @Binding var isPresented: Bool
    @State private var symptomName = ""
    @State private var selectedSeverity = Symptom.Severity.moderate
    @State private var notes = ""
    @State private var selectedMedications: Set<UUID> = []
    
    var body: some View {
        NavigationView {
            Form {
                Section(header: Text("Symptom Details")) {
                    TextField("Symptom Name", text: $symptomName, prompt: Text("e.g., Cough, Chest Pain"))
                    
                    Picker("Severity", selection: $selectedSeverity) {
                        Text("Mild").tag(Symptom.Severity.mild)
                        Text("Moderate").tag(Symptom.Severity.moderate)
                        Text("Moderate-Severe").tag(Symptom.Severity.moderate_severe)
                        Text("Severe").tag(Symptom.Severity.severe)
                        Text("Very Severe").tag(Symptom.Severity.very_severe)
                    }
                    
                    TextField("Notes (optional)", text: $notes, axis: .vertical)
                        .lineLimit(3...)
                }
                
                Section(header: Text("Related Medications")) {
                    ForEach(pillManager.pills) { pill in
                        Toggle(isOn: Binding(
                            get: { selectedMedications.contains(pill.id) },
                            set: { isOn in
                                if isOn {
                                    selectedMedications.insert(pill.id)
                                } else {
                                    selectedMedications.remove(pill.id)
                                }
                            }
                        )) {
                            Text(pill.name)
                        }
                    }
                }
            }
            .navigationTitle("Log Symptom")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarLeading) {
                    Button("Cancel") { isPresented = false }
                }
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("Save") {
                        do {
                            try symptomManager.logSymptom(
                                symptomName: symptomName,
                                severity: selectedSeverity,
                                notes: notes,
                                relatedMedications: Array(selectedMedications)
                            )
                            isPresented = false
                        } catch {
                            symptomManager.lastError = error
                        }
                    }
                    .disabled(symptomName.isEmpty)
                }
            }
        }
    }
}

#Preview {
    SymptomsView(symptomManager: SymptomManager(), pillManager: PillManager())
}
