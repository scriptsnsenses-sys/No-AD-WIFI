/*
 * ESP32 SLOW WiFi Repeater
 * ------------------------
 *  - STA side : joins YOUR home WiFi (internet source)
 *  - AP side  : creates its own "slow" WiFi network
 *  - NAPT     : routes AP clients to the internet via the STA link
 *  - Throttle : 802.11b only + 20 MHz + 5 dBm TX on the AP radio
 *               (plus a byte-level demo server on the ESP32 itself)
 *
 *  Test:
 *    1. Join "SlowWiFi" / "12345678" from a phone/PC.
 *    2. Open http://192.168.4.1/      -> tiny page (proves routing works)
 *    3. Click "Slow download"         -> 2 KB at exactly 2 ms/byte
 *    4. Run a speed test / load a website -> internet works, just slowly.
 */

#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include <lwip/ip4_addr.h>

// =============== Your home WiFi (upstream / internet) ===============
const char* STA_SSID = "YOUR-WIFI-SID";
const char* STA_PASS = "YOUR-WIFI-PASSWORD";

// =============== ESP32's own "slow" AP ==============================
const char* AP_SSID  = "NoAds";
const char* AP_PASS  = "No ADs in Mobile Games";

// =============== Radio throttle knobs ===============================
const bool   FORCE_11B_ON_AP = true;   // 802.11b only — biggest slowdown
const int8_t AP_TX_POWER     = 20;     // 20 * 0.25 dBm = 5 dBm (very weak)
                                       // raise to 40/52 (10/13 dBm) if clients can't connect

// =============== Demo byte-throttle (ESP32's own server only) =======
const unsigned long byteDelayMs = 2;    // 2 ms/byte ~ 500 B/s ~ 4 kbit/s
const size_t        DL_SIZE     = 2048;

WiFiServer server(80);

// =====================================================================
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== ESP32 Slow WiFi Repeater ===");

  // ---------- 1. Connect to home WiFi (STA) ----------
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(STA_SSID, STA_PASS);
  Serial.printf("Joining \"%s\"", STA_SSID);
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - t0 > 20000) {
      Serial.println("\nERROR: cannot join home WiFi. Check STA_SSID/STA_PASS.");
      return;
    }
  }
  Serial.printf("\nConnected to home WiFi! STA IP: %s\n",
                WiFi.localIP().toString().c_str());

  // ---------- 2. Start the slow AP ----------
  if (!WiFi.softAP(AP_SSID, AP_PASS)) {
    Serial.println("ERROR: softAP() failed");
    return;
  }
  delay(300);                         // let the AP fully come up
  Serial.printf("Slow AP SSID : %s\n", AP_SSID);
  Serial.printf("Slow AP IP   : %s\n", WiFi.softAPIP().toString().c_str());

  // ---------- 3. Throttle the AP radio ----------
  if (FORCE_11B_ON_AP) {
    esp_err_t e = esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11B);
    Serial.printf("set_protocol(AP -> 802.11b only) : %s\n", esp_err_to_name(e));
  }
  esp_err_t e1 = esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);
  esp_err_t e2 = esp_wifi_set_max_tx_power(AP_TX_POWER);
  Serial.printf("set_bandwidth(HT20)               : %s\n", esp_err_to_name(e1));
  Serial.printf("set_max_tx_power(%d = %.2f dBm)   : %s\n",
                AP_TX_POWER, AP_TX_POWER * 0.25, esp_err_to_name(e2));

  // ---------- 4. Enable NAPT so AP clients reach the internet ----------
  esp_netif_t* ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
  if (!ap_netif) {
    Serial.println("ERROR: AP netif handle not found");
  } else {
    esp_err_t err = esp_netif_napt_enable(ap_netif);
    if (err == ESP_OK) {
      Serial.println("NAPT enabled  ->  AP clients can reach the internet");
    } else {
      Serial.printf("NAPT enable FAILED: %s\n", esp_err_to_name(err));
      Serial.println("  -> needs CONFIG_LWIP_IPV4_NAPT in sdkconfig;");
      Serial.println("     normally enabled in stock arduino-esp32 builds.");
    }
  }

  // ---------- 5. Point DHCP clients at public DNS ----------
  // (ESP32 does not run a DNS forwarder; without this, clients get 192.168.4.1
  //  as DNS and name resolution fails even though NAT works.)
  if (ap_netif) {
    esp_netif_dhcps_stop(ap_netif);
    esp_ip4_addr_t dns[2];
    dns[0].addr = htonl(0x08080808);   // 8.8.8.8
    dns[1].addr = htonl(0x01010101);   // 1.1.1.1
    esp_err_t de = esp_netif_dhcps_option(ap_netif, ESP_NETIF_OP_SET,
                                          ESP_NETIF_DOMAIN_NAME_SERVER,
                                          dns, sizeof(dns));
    esp_netif_dhcps_start(ap_netif);
    Serial.printf("DHCP DNS (8.8.8.8, 1.1.1.1) : %s\n", esp_err_to_name(de));
  }

  server.begin();
  Serial.println("\nReady. On a slow-network client:");
  Serial.printf("  http://%s/         (tiny page)\n", WiFi.softAPIP().toString().c_str());
  Serial.printf("  http://%s/download (2 KB at %lu ms/byte)\n",
                WiFi.softAPIP().toString().c_str(), byteDelayMs);
}

// =====================================================================
void loop() {
  WiFiClient c = server.available();
  if (!c) return;
  serveClient(c);
  c.stop();
}

// =====================================================================
void sendSlow(WiFiClient& c, const uint8_t* data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (!c.connected()) return;
    c.write(data[i]);
    if (byteDelayMs) delay(byteDelayMs);
  }
}

void serveClient(WiFiClient& c) {
  Serial.printf("[HTTP] client %s\n", c.remoteIP().toString().c_str());

  // read request line + headers (short timeout)
  String req = "";
  unsigned long t0 = millis();
  while (c.connected() && millis() - t0 < 1500) {
    while (c.available()) req += (char)c.read();
    if (req.indexOf("\r\n\r\n") >= 0 || req.indexOf("\n\n") >= 0) break;
    delay(1);
  }

  // ---------- /download : hard 2 ms/byte throttle demo ----------
  if (req.indexOf("/download") >= 0) {
    c.print("HTTP/1.1 200 OK\r\n");
    c.print("Content-Type: application/octet-stream\r\n");
    c.print("Content-Disposition: attachment; filename=\"slowtest.txt\"\r\n");
    c.printf("Content-Length: %u\r\n", (unsigned)DL_SIZE);
    c.print("Connection: close\r\n\r\n");

    unsigned long start = millis();
    for (size_t i = 0; i < DL_SIZE; i++) {
      if (!c.connected()) break;
      c.write((uint8_t)('A' + (i % 26)));
      if (byteDelayMs) delay(byteDelayMs);
    }
    unsigned long ms = millis() - start;
    if (ms == 0) ms = 1;
    Serial.printf("[HTTP] sent %u B in %lu ms = %lu B/s\n",
                  (unsigned)DL_SIZE, ms,
                  (unsigned long)((DL_SIZE * 1000UL) / ms));
    return;
  }

  // ---------- / : small demo page ----------
  String html =
    "<!DOCTYPE html><html><head>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>ESP32 Slow Repeater</title></head><body>"
    "<h1>ESP32 Slow Repeater</h1>"
    "<p>Upstream (home WiFi): connected<br>"
    "STA IP: " + WiFi.localIP().toString() + "</p>"
    "<p>Slow AP: 192.168.4.1, 802.11b-only, " +
    String(AP_TX_POWER * 0.25, 2) + " dBm</p>"
    "<p><a href='/download'>Download 2 KB at " +
    String(byteDelayMs) + " ms/byte (this IS the 2 ms throttle)</a></p>"
    "<p>General internet works too, just slowly (radio-throttled, "
    "not byte-throttled). Try a speed test.</p>"
    "</body></html>";

  String resp = "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: " + String(html.length()) + "\r\n"
                "Connection: close\r\n\r\n" + html;
  c.write((const uint8_t*)resp.c_str(), resp.length());
}
