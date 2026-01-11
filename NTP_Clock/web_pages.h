#ifndef WEB_PAGES_H
#define WEB_PAGES_H

#include <Preferences.h>
#include <WiFi.h>

String getConfigPageHTML(Preferences& prefs) {
  String savedSSID = "";
  prefs.begin("wifi_config", true);
  savedSSID = prefs.getString("ssid", "");
  prefs.end();
  
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>NTP Clock Configuration</title>";
  html += "<style>";
  html += "body{font-family:Arial,sans-serif;max-width:600px;margin:20px auto;padding:20px;background:#f5f5f5;}";
  html += "h1{color:#333;}";
  html += ".form-group{margin-bottom:15px;}";
  html += "label{display:block;margin-bottom:5px;font-weight:bold;}";
  html += "input,select{width:100%;padding:8px;box-sizing:border-box;border:1px solid #ddd;border-radius:4px;}";
  html += "button{background:#4CAF50;color:white;padding:10px 20px;border:none;border-radius:4px;cursor:pointer;font-size:16px;}";
  html += "button:hover{background:#45a049;}";
  html += ".reset-btn{background:#f44336;margin-top:20px;}";
  html += ".reset-btn:hover{background:#da190b;}";
  html += ".note{margin-top:20px;padding:10px;background:#fff3cd;border-left:4px solid #ffc107;}";
  html += "</style></head><body>";
  html += "<h1>NTP Clock Configuration</h1>";
  html += "<form method='POST' action='/save'>";
  html += "<div class='form-group'><label>WiFi SSID:</label>";
  html += "<input type='text' name='ssid' value='" + savedSSID + "' required></div>";
  html += "<div class='form-group'><label>WiFi Password:</label>";
  html += "<input type='password' name='password' required></div>";
  html += "<div class='form-group'><label>Timezone (GMT offset in seconds):</label>";
  html += "<input type='number' name='timezone' value='-28800' required></div>";
  html += "<div class='form-group'><label>Daylight Saving Offset (seconds):</label>";
  html += "<input type='number' name='dst_offset' value='0'></div>";
  html += "<div class='form-group'><label>Brightness (0-15):</label>";
  html += "<input type='number' name='brightness' min='0' max='15' value='8'></div>";
  html += "<div class='form-group'><label>Hour Format:</label>";
  html += "<select name='hour_format'><option value='24'>24-hour</option><option value='12'>12-hour</option></select></div>";
  html += "<button type='submit'>Save and Restart</button>";
  html += "</form>";
  html += "<form method='POST' action='/factory-reset'>";
  html += "<button type='submit' class='reset-btn'>Factory Reset</button>";
  html += "</form>";
  html += "<div class='note'><strong>Note:</strong> After saving, the device will restart and connect to WiFi.</div>";
  html += "</body></html>";
  return html;
}

String getSaveSuccessPageHTML() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Settings Saved</title>";
  html += "<style>";
  html += "body{font-family:Arial,sans-serif;max-width:600px;margin:20px auto;padding:20px;text-align:center;}";
  html += "h1{color:#4CAF50;}";
  html += "</style></head><body>";
  html += "<h1>Settings Saved!</h1>";
  html += "<p>The device is restarting and will connect to WiFi.</p>";
  html += "</body></html>";
  return html;
}

String getFactoryResetPageHTML() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Factory Reset</title>";
  html += "<style>";
  html += "body{font-family:Arial,sans-serif;max-width:600px;margin:20px auto;padding:20px;text-align:center;}";
  html += "h1{color:#f44336;}";
  html += "</style></head><body>";
  html += "<h1>Factory Reset Complete</h1>";
  html += "<p>All settings have been cleared. The device is restarting.</p>";
  html += "</body></html>";
  return html;
}

#endif // WEB_PAGES_H
