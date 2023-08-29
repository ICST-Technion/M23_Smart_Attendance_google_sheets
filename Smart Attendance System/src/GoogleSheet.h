#ifndef GOOGLE_SHEET_H
#define GOOGLE_SHEET_H

#include <string>
#include <map>

#include <HTTPClient.h>
#include <WiFi.h>

const String GOOGLE_SCRIPT_ID = "AKfycbwE-wRd4-k9RimSp_svSTjKdhQbHha_pTppacQrqA0_s8QRCHWLDjZIl7IhHYOTQRB3";

const std::map<String, String> SSID_TO_PASS = {{"TechPublic", ""}};

class GoogleSheet{
public:
    GoogleSheet() : http(), url(get_url(GOOGLE_SCRIPT_ID)) {};
    GoogleSheet(const GoogleSheet&) = default;
    GoogleSheet& operator=(const GoogleSheet&) = delete;

    void init(){
        init_wifi();
    }

    void readDataFromGoogleSheet(){
        if (WiFi.status() != WL_CONNECTED) {
            throw std::runtime_error("WiFi not connected.");
        }

        HTTPClient http;
        Serial.println("Making a request");
        Serial.println(url);
        http.begin(url.c_str()); //Specify the URL and certificate
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        int httpCode = http.GET();
        String payload;
        if (httpCode > 0) { //Check for the returning code
            payload = http.getString();
            Serial.println(httpCode);
            Serial.println(payload);
            //display1.println(payload);
        }
        else {
            Serial.println("Error on HTTP request");
        }
        http.end();
    }

    void post_data(){
        http.begin(url.c_str());
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        // Specify content-type header
        http.addHeader("Content-Type", "text/csv");
        // Data to send with HTTP POST
        String httpRequestData = "bla1,bla2,bla3";
        // Send HTTP POST request
        int httpResponseCode = http.POST(httpRequestData);
        Serial.print("HTTP Status Code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);
        http.end();
    }


private:
    HTTPClient http;
    const String url;

    void init_wifi() {
        WiFi.mode(WIFI_STA);
        WiFi.begin("TechPublic"); //TODO: Scan SSIDs.
        Serial.print("Connecting to WiFi ..");
        while (WiFi.status() != WL_CONNECTED)
        {
            Serial.print('.');
            delay(1000);
        }
        Serial.println(WiFi.localIP());
    }

    const String get_url(const String& google_script_id) {
        return "https://script.google.com/macros/s/" + google_script_id + "/exec";
    }
};

#endif