#include <GoogleSheet.h>

const std::map<FileName, String> FILE_TO_POST_EXTENSION {
    {USER_LIST, GET_USERS},
    {ATTENDANCE_LOG, ADD_MULTIPLE_LOG},
    {PENDING_USER_LIST, ADD_MULTIPLE_USERS},
};

String GoogleSheet::getUserList(){
    return getDataWithPost("", getPostUrl(USER_LIST));
}

bool GoogleSheet::postEntry(FileName file_name, const String& entry) {
    return postMultiEntries(file_name, std::vector<String>({entry}));
}

String GoogleSheet::getPostUrl(FileName file_name) {
    return url + "?" + FILE_TO_POST_EXTENSION.at(file_name);
}

bool GoogleSheet::postMultiEntries(FileName file_name, const std::vector<String>& entries) {
    String data = "";
    for (const auto& entry : entries) {
        data += entry + "\n";
    }
    data.remove(data.length() - 1); // Remove trailing \n
    return postData(data, getPostUrl(file_name));
}

void GoogleSheet::init() {
    WiFi.mode(WIFI_STA);
    IPAddress dns(8,8,8,8);
    WiFi.setAutoReconnect(true);
    if (PASSWORD != "") {
      WiFi.begin(SSID, PASSWORD);
    }
    else {
      WiFi.begin(SSID);
    }

    if (IOT_DEBUG) {
        Serial.print("Connecting to WiFi..");
    }
    for (uint8_t i = 0; !WiFi.isConnected() && i < CONNECTION_ATTEMPTS_BEFORE_RECONNECT; i++) {
        if (IOT_DEBUG) {
            Serial.print(".");
        }
        delay(1000);
    }
    if (IOT_DEBUG) {
        Serial.println();
    }
    if (!WiFi.isConnected()) {
        reconnectWiFi();
    }
}

void GoogleSheet::reconnectWiFi() {
    if (IOT_DEBUG) {
        Serial.print("Attemting to reconnect..");
    }
    WiFi.reconnect();
    uint8_t i = 0;
    do {
        if (IOT_DEBUG) {
            Serial.print(".");
        }
        delay(1000);
        if (i++ % CONNECTION_ATTEMPTS_BEFORE_RECONNECT == 0) {
            WiFi.reconnect();
        }
    } while (!WiFi.isConnected());
    if (IOT_DEBUG) {
        Serial.println("WiFi Connected.");
    }
}

void GoogleSheet::validateWiFi(bool force_reconnect) {
    if (!force_reconnect && WiFi.isConnected()) {
        return;
    }

    reconnectWiFi();
}

bool GoogleSheet::establishConnection(const String& override_url) {
    validateWiFi();
    const String& final_url = override_url != "" ? override_url : url;
    if (IOT_DEBUG) {
        Serial.println("Connecting to " + final_url);
    }
    http.begin(final_url.c_str());
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    return true;
}

void GoogleSheet::endConnection() {
    http.end();
}

const String GoogleSheet::get_url(const String& google_script_id) {
    return "https://script.google.com/macros/s/" + google_script_id + "/exec";
}

String GoogleSheet::getDataWithPost(const String& data, const String& post_url){
    establishConnection(post_url);
    http.addHeader("Content-Type", "text/csv");
    int http_response_code = http.POST(data);
    if (IOT_DEBUG) {
        Serial.print("HTTP Status Code: ");
        Serial.println(http_response_code);
    }
    String payload = http.getString();
    endConnection();
    return payload;
}

bool GoogleSheet::postData(const String& data, const String& post_url){
    establishConnection(post_url);
    http.addHeader("Content-Type", "text/csv");
    int http_response_code = http.POST(data);
    if (IOT_DEBUG) {
        Serial.print("HTTP Status Code: ");
        Serial.println(http_response_code);
    }
    String payload = http.getString();
    endConnection();
    return http_response_code == 200;
}

uint16_t GoogleSheet::postChanges(FileName file_name, const std::vector<String>& pending_changes) {
    bool res = postMultiEntries(file_name, pending_changes);
    return res * pending_changes.size();
}
