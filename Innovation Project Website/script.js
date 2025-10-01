// Demo mode - simulates the dispenser locally
let stats = {
    totalDispensed: 0,
    successful: 0,
    failed: 0,
    systemEnabled: true,
    startTime: Date.now(),
  };
  
  let isDispensing = false;
  
  function updateUI() {
    document.getElementById("totalDispensed").textContent = stats.totalDispensed;
    document.getElementById("successful").textContent = stats.successful;
    document.getElementById("failed").textContent = stats.failed;
  
    let successRate =
      stats.totalDispensed > 0
        ? Math.round((stats.successful / stats.totalDispensed) * 100)
        : 0;
    document.getElementById("successRate").textContent = successRate + "%";
  
    // Update accuracy indicator
    let accuracyEl = document.getElementById("accuracyInfo");
    if (stats.totalDispensed > 0) {
      if (successRate >= 95) {
        accuracyEl.className = "accuracy accuracy-good";
        accuracyEl.textContent = "✓ Excellent accuracy";
      } else if (successRate >= 85) {
        accuracyEl.className = "accuracy accuracy-warning";
        accuracyEl.textContent = "⚠ Accuracy needs improvement";
      } else {
        accuracyEl.className = "accuracy accuracy-bad";
        accuracyEl.textContent = "✗ Poor accuracy - check mechanism";
      }
    }
  
    // Update uptime
    let uptime = Math.floor((Date.now() - stats.startTime) / 1000);
    let hours = Math.floor(uptime / 3600);
    let minutes = Math.floor((uptime % 3600) / 60);
    document.getElementById("uptime").textContent = hours + "h " + minutes + "m";
  
    document.getElementById("systemToggleText").textContent = stats.systemEnabled
      ? "Disable System"
      : "Enable System";
  
    if (!stats.systemEnabled) {
      document.getElementById("statusText").textContent = "System Disabled";
      document.getElementById("statusIndicator").className =
        "status-indicator offline";
    } else {
      document.getElementById("statusText").textContent = "System Online";
      document.getElementById("statusIndicator").className =
        "status-indicator online";
    }
  }
  
  function showAlert(message, type) {
    let alertBox = document.getElementById("alertBox");
    alertBox.textContent = message;
    alertBox.className = "alert " + type;
    alertBox.style.display = "block";
    setTimeout(() => {
      alertBox.style.display = "none";
    }, 5000);
  }
  
  function dispense() {
    if (isDispensing) return;
    if (!stats.systemEnabled) {
      showAlert("System is disabled. Enable it first.", "error");
      return;
    }
  
    isDispensing = true;
    let btn = document.getElementById("dispenseBtn");
    btn.disabled = true;
    btn.textContent = "DISPENSING...";
  
    // Simulate dispensing with random success/failure (90% success rate)
    setTimeout(() => {
      let success = Math.random() > 0.1;
      stats.totalDispensed++;
  
      if (success) {
        stats.successful++;
        showAlert("✓ Pill dispensed successfully!", "success");
      } else {
        stats.failed++;
        showAlert("✗ Dispense failed: Sensor timeout", "error");
      }
  
      let now = new Date();
      document.getElementById("lastDispense").textContent =
        now.toLocaleTimeString();
  
      updateUI();
  
      isDispensing = false;
      btn.disabled = false;
      btn.textContent = "DISPENSE PILL";
    }, 2000);
  }
  
  function toggleSystem() {
    stats.systemEnabled = !stats.systemEnabled;
    showAlert(
      stats.systemEnabled ? "✓ System enabled" : "⚠ System disabled",
      stats.systemEnabled ? "success" : "error"
    );
    updateUI();
  }
  
  function resetStats() {
    if (!confirm("Reset all statistics? This cannot be undone.")) return;
  
    stats.totalDispensed = 0;
    stats.successful = 0;
    stats.failed = 0;
    document.getElementById("lastDispense").textContent = "Never";
  
    showAlert("✓ Statistics reset", "success");
    updateUI();
  }
  
  // Initial load
  updateUI();
  
  // Update uptime every second
  setInterval(updateUI, 1000);