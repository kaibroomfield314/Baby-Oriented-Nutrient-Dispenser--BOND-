//
//  BOND_APP_IOSTests.swift
//  BOND-APP-IOSTests
//
//  Created by Kai Broomfield on 15/10/2025.
//

import Testing
@testable import BOND_APP_IOS

struct PillManagerTests {
    var pillManager: PillManager!

    init() {
        pillManager = PillManager()
        // Clear any existing pills
        UserDefaults.standard.removeObject(forKey: "saved_pills")
    }

    @Test func addPill() throws {
        let pill = Pill(name: "Aspirin", colour: "red", compartment: 1)
        try pillManager.addPill(pill)

        #expect(pillManager.pills.count == 1)
        #expect(pillManager.pills.first?.name == "Aspirin")
    }

    @Test func addPillWithEmptyName() throws {
        let pill = Pill(name: "", colour: "blue", compartment: 1)

        do {
            try pillManager.addPill(pill)
            Issue.record("Expected PillManagerError.invalidPill")
        } catch PillManagerError.invalidPill {
            // Expected
        }
    }

    @Test func updatePill() throws {
        var pill = Pill(name: "Aspirin", colour: "red", compartment: 1)
        try pillManager.addPill(pill)

        pill.name = "Ibuprofen"
        try pillManager.updatePill(pill)

        #expect(pillManager.pills.first?.name == "Ibuprofen")
    }

    @Test func deletePill() throws {
        let pill = Pill(name: "Aspirin", colour: "red", compartment: 1)
        try pillManager.addPill(pill)

        #expect(pillManager.pills.count == 1)

        try pillManager.deletePill(pill)

        #expect(pillManager.pills.count == 0)
    }

    @Test func getPillById() throws {
        let pill = Pill(name: "Aspirin", colour: "red", compartment: 1)
        try pillManager.addPill(pill)

        let retrieved = pillManager.getPill(by: pill.id)
        #expect(retrieved?.name == "Aspirin")
    }
}

struct ScheduleManagerTests {
    var scheduleManager: ScheduleManager!

    init() {
        scheduleManager = ScheduleManager()
        UserDefaults.standard.removeObject(forKey: "pill_schedules")
        UserDefaults.standard.removeObject(forKey: "dose_records")
        UserDefaults.standard.removeObject(forKey: "medication_statistics")
    }

    @Test func addSchedule() throws {
        let schedule = PillSchedule(
            time: Date(),
            enabled: true,
            pillCount: 1,
            label: "Morning",
            scheduleType: .general
        )

        try scheduleManager.addSchedule(schedule)

        #expect(scheduleManager.schedules.count == 1)
        #expect(scheduleManager.schedules.first?.label == "Morning")
    }

    @Test func updateSchedule() throws {
        var schedule = PillSchedule(
            time: Date(),
            enabled: true,
            pillCount: 1,
            label: "Morning",
            scheduleType: .general
        )

        try scheduleManager.addSchedule(schedule)

        schedule.label = "Evening"
        schedule.enabled = false
        try scheduleManager.updateSchedule(schedule)

        #expect(scheduleManager.schedules.first?.label == "Evening")
        #expect(scheduleManager.schedules.first?.enabled == false)
    }

    @Test func deleteSchedule() throws {
        let schedule = PillSchedule(
            time: Date(),
            enabled: true,
            pillCount: 1,
            label: "Morning",
            scheduleType: .general
        )

        try scheduleManager.addSchedule(schedule)
        #expect(scheduleManager.schedules.count == 1)

        try scheduleManager.deleteSchedule(schedule)
        #expect(scheduleManager.schedules.count == 0)
    }

    @Test func getDoseRecords() throws {
        let schedule = PillSchedule(
            time: Date(),
            enabled: true,
            pillCount: 1,
            label: "Morning",
            scheduleType: .general
        )

        try scheduleManager.addSchedule(schedule)

        let records = scheduleManager.getDoseRecords(for: schedule.id)
        #expect(records.isEmpty)
    }

    @Test func markDoseAsTaken() throws {
        let schedule = PillSchedule(
            time: Date(),
            enabled: true,
            pillCount: 1,
            label: "Morning",
            scheduleType: .general
        )

        try scheduleManager.addSchedule(schedule)

        // Create a dose record manually
        let record = DoseRecord(scheduleId: schedule.id, scheduledTime: Date(), pillCount: 1)
        scheduleManager.doseRecords.append(record)

        try scheduleManager.markDoseAsTaken(scheduleId: schedule.id)

        let taken = scheduleManager.doseRecords.first(where: { $0.status == .taken || $0.status == .late })
        #expect(taken != nil)
    }
}

struct ColorHelperTests {
    @Test func colorConversion() {
        let redColor = ColorHelper.colorFromString("red")
        let blueColor = ColorHelper.colorFromString("blue")
        let invalidColor = ColorHelper.colorFromString("invalid")

        #expect(ColorHelper.availableColors.contains("red"))
        #expect(ColorHelper.availableColors.contains("blue"))
        #expect(ColorHelper.availableColors.count == 10)
        #expect(invalidColor == .blue) // Default to blue
    }
}
