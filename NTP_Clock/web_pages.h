#ifndef WEB_PAGES_H
#define WEB_PAGES_H

#include <Preferences.h>
#include <WiFi.h>

String getConfigPageHTML(Preferences& prefs) {
  // Load saved WiFi credentials
  String savedSSID = "";
  String savedPassword = "";
  prefs.begin("wifi_config", true);
  savedSSID = prefs.getString("ssid", "");
  savedPassword = prefs.getString("password", "");
  prefs.end();
  
  // Load saved preferences
  long savedTimezone = -28800;
  int savedDSTOffset = 0;
  int savedBrightness = 8;
  bool saved24Hour = true;
  prefs.begin("ntp_clock", true);
  savedTimezone = prefs.getLong("timezone", -28800);
  savedDSTOffset = prefs.getInt("dst_offset", 0);
  savedBrightness = prefs.getInt("brightness", 8);
  saved24Hour = prefs.getBool("24hour", true);
  prefs.end();
  
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<meta charset='UTF-8'>";
  html += "<title>NTP Clock Configuration</title>";
  html += "<style>";
  html += "body{font-family:Arial,sans-serif;max-width:600px;margin:20px auto;padding:20px;background:#f5f5f5;}";
  html += "h1{color:#333;margin-bottom:20px;}";
  html += ".form-group{margin-bottom:15px;}";
  html += "label{display:block;margin-bottom:5px;font-weight:bold;color:#555;}";
  html += "input,select{width:100%;padding:8px;box-sizing:border-box;border:1px solid #ddd;border-radius:4px;font-size:14px;}";
  html += "input:focus,select:focus{outline:none;border-color:#4CAF50;}";
  html += "button{background:#4CAF50;color:white;padding:10px 20px;border:none;border-radius:4px;cursor:pointer;font-size:16px;width:100%;margin-top:10px;}";
  html += "button:hover{background:#45a049;}";
  html += ".reset-btn{background:#f44336;margin-top:20px;}";
  html += ".reset-btn:hover{background:#da190b;}";
  html += ".note{margin-top:20px;padding:10px;background:#fff3cd;border-left:4px solid #ffc107;border-radius:4px;}";
  html += ".info{margin-top:15px;padding:10px;background:#e3f2fd;border-left:4px solid #2196F3;border-radius:4px;font-size:0.9em;}";
  html += "</style></head><body>";
  html += "<h1>NTP Clock Configuration</h1>";
  html += "<form method='POST' action='/save'>";
  html += "<div class='form-group'><label>WiFi SSID:</label>";
  html += "<input type='text' name='ssid' value='" + savedSSID + "' required></div>";
  html += "<div class='form-group'><label>WiFi Password:</label>";
  html += "<input type='password' name='password' value='" + savedPassword + "' placeholder='Leave blank to keep current password'></div>";
  html += "<div class='form-group'><label>Timezone:</label>";
  html += "<select name='timezone' required>";
  html += "<option value='-28800'" + String(savedTimezone == -28800 ? " selected" : "") + ">Pacific Time (PT) - UTC-8 (PST/PDT with DST)</option>";
  html += "<option value='-21600'" + String(savedTimezone == -21600 ? " selected" : "") + ">Mountain Time (MT) - UTC-7 (MST/MDT with DST)</option>";
  html += "<option value='-18000'" + String(savedTimezone == -18000 ? " selected" : "") + ">Central Time (CT) - UTC-6 (CST/CDT with DST)</option>";
  html += "<option value='-14400'" + String(savedTimezone == -14400 ? " selected" : "") + ">Eastern Time (ET) - UTC-5 (EST/EDT with DST)</option>";
  html += "<option value='0'" + String(savedTimezone == 0 ? " selected" : "") + ">UTC (Coordinated Universal Time)</option>";
  html += "<option value='3600'" + String(savedTimezone == 3600 ? " selected" : "") + ">Central European Time (CET) - UTC+1</option>";
  html += "<option value='7200'" + String(savedTimezone == 7200 ? " selected" : "") + ">Central European Summer Time (CEST) - UTC+2</option>";
  html += "<option value='28800'" + String(savedTimezone == 28800 ? " selected" : "") + ">China Standard Time (CST) - UTC+8</option>";
  html += "<option value='32400'" + String(savedTimezone == 32400 ? " selected" : "") + ">Japan Standard Time (JST) - UTC+9</option>";
  html += "<option value='36000'" + String(savedTimezone == 36000 ? " selected" : "") + ">Australian Eastern Standard Time (AEST) - UTC+10</option>";
  html += "</select></div>";
  html += "<div class='form-group'><label>Daylight Saving Offset (seconds):</label>";
  html += "<input type='number' name='dst_offset' value='" + String(savedDSTOffset) + "'>";
  html += "<small style='display:block;color:#666;margin-top:5px;'>Usually 0 (DST handled automatically) or 3600 (1 hour)</small></div>";
  html += "<div class='form-group'><label>Brightness (0-15):</label>";
  html += "<input type='number' name='brightness' min='0' max='15' value='" + String(savedBrightness) + "'></div>";
  html += "<div class='form-group'><label>Hour Format:</label>";
  html += "<select name='hour_format'>";
  html += "<option value='24'" + String(saved24Hour ? " selected" : "") + ">24-hour</option>";
  html += "<option value='12'" + String(saved24Hour ? "" : " selected") + ">12-hour</option>";
  html += "</select></div>";
  html += "<button type='submit'>Save and Restart</button>";
  html += "</form>";
  html += "<form method='POST' action='/factory-reset'>";
  html += "<button type='submit' class='reset-btn'>Factory Reset</button>";
  html += "</form>";
  html += "<div class='note'><strong>Note:</strong> After saving, the device will restart and connect to WiFi.</div>";
  html += "<div class='info'><strong>Current IP:</strong> " + WiFi.localIP().toString() + " (if connected) or " + WiFi.softAPIP().toString() + " (AP mode)</div>";
  html += "</body></html>";
  return html;
}

String getSaveSuccessPageHTML() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<meta charset='UTF-8'>";
  html += "<title>Settings Saved</title>";
  html += "<style>";
  html += "body{font-family:Arial,sans-serif;max-width:600px;margin:20px auto;padding:20px;text-align:center;}";
  html += "h1{color:#4CAF50;}";
  html += "p{margin-top:20px;color:#666;}";
  html += "</style></head><body>";
  html += "<h1>Settings Saved!</h1>";
  html += "<p>The device is restarting and will connect to WiFi.</p>";
  html += "<p>You will be redirected to the configuration page shortly.</p>";
  html += "</body></html>";
  return html;
}

String getFactoryResetPageHTML() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<meta charset='UTF-8'>";
  html += "<title>Factory Reset</title>";
  html += "<style>";
  html += "body{font-family:Arial,sans-serif;max-width:600px;margin:20px auto;padding:20px;text-align:center;}";
  html += "h1{color:#f44336;}";
  html += "p{margin-top:20px;color:#666;}";
  html += "</style></head><body>";
  html += "<h1>Factory Reset Complete</h1>";
  html += "<p>All settings have been cleared. The device is restarting.</p>";
  html += "<p>The device will start in AP mode. Connect to the access point and configure at 192.168.4.1</p>";
  html += "</body></html>";
  return html;
}

#endif // WEB_PAGES_H
