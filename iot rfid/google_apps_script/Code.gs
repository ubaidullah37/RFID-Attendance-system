/**
 * ================================================================
 *  RFID ATTENDANCE SYSTEM — Google Apps Script
 * ================================================================
 *  HOW TO DEPLOY:
 *  1. Go to https://script.google.com
 *  2. Click "New Project" → paste this entire file
 *  3. Click the floppy disk (Save) — name it "RFID Attendance"
 *  4. Click "Deploy" → "New deployment"
 *  5. Type: Web app
 *  6. Execute as: Me
 *  7. Who has access: Anyone
 *  8. Click Deploy → Authorize → Copy the Web App URL
 *  9. Paste that URL into rfid_attendance.ino → SHEET_URL
 * ================================================================
 */

// ---- CONFIG: paste your Google Sheet ID here ----
// Open your Google Sheet → look at the URL:
// https://docs.google.com/spreadsheets/d/  <SHEET_ID>  /edit
const SHEET_ID   = "1c4oypzryv6sBVuofCvpMMGUqSV4t1WetkO9xc7QOXZc";  // ✅ Your sheet
const SHEET_NAME = "Attendance";                                             // ← Sheet tab name


// ================================================================
//  doPost — called by ESP32 when a card is scanned
// ================================================================
function doPost(e) {
  try {
    var data    = JSON.parse(e.postData.contents);
    var name      = data.name      || "Unknown";
    var uid       = data.uid       || "N/A";
    var status    = data.status    || "N/A";
    var timestamp = data.timestamp || new Date().toLocaleString();

    appendToSheet(name, uid, status, timestamp);

    return ContentService
      .createTextOutput(JSON.stringify({ result: "success", name: name, status: status }))
      .setMimeType(ContentService.MimeType.JSON);

  } catch (err) {
    return ContentService
      .createTextOutput(JSON.stringify({ result: "error", message: err.message }))
      .setMimeType(ContentService.MimeType.JSON);
  }
}


// ================================================================
//  doGet — simple health check (visit URL in browser to test)
// ================================================================
function doGet(e) {
  return ContentService
    .createTextOutput("✅ RFID Attendance Script is running!")
    .setMimeType(ContentService.MimeType.TEXT);
}


// ================================================================
//  appendToSheet — writes one row to the spreadsheet
// ================================================================
function appendToSheet(name, uid, status, timestamp) {
  var ss    = SpreadsheetApp.openById(SHEET_ID);
  var sheet = ss.getSheetByName(SHEET_NAME);

  // Create sheet + header if it doesn't exist
  if (!sheet) {
    sheet = ss.insertSheet(SHEET_NAME);
    setupHeader(sheet);
  }

  // If sheet is brand new (only 1 row or empty), add header
  if (sheet.getLastRow() === 0) {
    setupHeader(sheet);
  }

  // Calculate duration for OUT records
  var duration = "";
  if (status === "OUT") {
    duration = calculateDuration(sheet, name);
  }

  // Append the attendance row
  sheet.appendRow([
    sheet.getLastRow(),   // Row number (auto-increment)
    name,
    uid,
    status,
    timestamp,
    duration
  ]);

  // Color-code the row
  var lastRow   = sheet.getLastRow();
  var rowRange  = sheet.getRange(lastRow, 1, 1, 6);

  if (status === "IN") {
    rowRange.setBackground("#d9ead3");   // light green
  } else if (status === "OUT") {
    rowRange.setBackground("#fff2cc");   // light yellow
  } else if (status === "DENIED") {
    rowRange.setBackground("#f4cccc");   // light red
  }

  // Auto-resize columns for readability
  sheet.autoResizeColumns(1, 6);
}


// ================================================================
//  setupHeader — creates the header row with formatting
// ================================================================
function setupHeader(sheet) {
  var headers = ["#", "Name", "UID", "Status", "Timestamp", "Duration (min)"];
  sheet.appendRow(headers);

  var headerRange = sheet.getRange(1, 1, 1, headers.length);
  headerRange.setBackground("#1a73e8");
  headerRange.setFontColor("#ffffff");
  headerRange.setFontWeight("bold");
  headerRange.setFontSize(11);
  headerRange.setHorizontalAlignment("center");

  // Freeze the header row
  sheet.setFrozenRows(1);
}


// ================================================================
//  calculateDuration — finds the matching IN record and returns
//  the duration in minutes between IN and OUT scans
// ================================================================
function calculateDuration(sheet, name) {
  try {
    var lastRow = sheet.getLastRow();
    if (lastRow < 2) return "";

    var data = sheet.getRange(2, 1, lastRow - 1, 6).getValues();

    // Search backwards for last IN record for this person
    for (var i = data.length - 1; i >= 0; i--) {
      if (data[i][1] === name && data[i][3] === "IN") {
        var inTime  = new Date(data[i][4]);
        var outTime = new Date();
        var diffMin = Math.round((outTime - inTime) / 60000);
        return diffMin > 0 ? diffMin : "";
      }
    }
  } catch (err) {
    // ignore errors in duration calc
  }
  return "";
}


// ================================================================
//  testAppend — run this manually in Apps Script editor to test
//  without needing the ESP32
// ================================================================
function testAppend() {
  appendToSheet("Card - Alice", "A1:B2:C3:D4", "IN",  "2024-01-15 09:00:00");
  appendToSheet("Tag  - Bob",   "11:22:33:44", "IN",  "2024-01-15 09:05:00");
  appendToSheet("Card - Alice", "A1:B2:C3:D4", "OUT", "2024-01-15 17:00:00");
  appendToSheet("UNKNOWN",      "FF:FF:FF:FF", "DENIED", "2024-01-15 09:30:00");
  Logger.log("Test rows appended successfully!");
}
