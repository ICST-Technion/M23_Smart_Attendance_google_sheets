#include <IOTFiles.h>
#include <vector>
#include <cassert>

const std::map<FileName, String> FILE_TO_PATH = {
    {ATTENDANCE_LOG, ATTENDANCELOG_PATH},
    {USER_LIST, USERLIST_PATH},
    {PENDING_USER_LIST, PENDINGUSERLIST_PATH},
};

const std::map<FileName, uint8_t> FILE_TO_LOCK = {
    {ATTENDANCE_LOG, ATTENDANCELOG_LOCK},
    {USER_LIST, USERLIST_LOCK},
    {PENDING_USER_LIST, PENDINGUSERLIST_LOCK},
};


void IOTFiles::addAttendanceLogEntry(const String& entry) {
    addEntry(ATTENDANCE_LOG, entry);
}

void IOTFiles::writeUserList(const UserList& user_list, FileName file_name) {
    assert(file_name == PENDING_USER_LIST || file_name == USER_LIST);
    lock(file_name);
    clearFile(file_name, true);
    for (const auto& user : user_list) {
        addEntry(file_name, user.id, user.uid, true);
    }
    unlock(file_name);
}

String IOTFiles::debugReadFile(FileName file_name) {
    return readFile(file_name);
}

void IOTFiles::lock(FileName file_name) {
    if (IOT_DEBUG) {
        Serial.println("Attempting to lock " + FILE_TO_PATH.at(file_name));
    }
    mutexes[FILE_TO_LOCK.at(file_name)].lock();
}

void IOTFiles::unlock(FileName file_name) {
    mutexes[FILE_TO_LOCK.at(file_name)].unlock();
    if (IOT_DEBUG) {
        Serial.println("Unlocked " + FILE_TO_PATH.at(file_name));
    }
}

File IOTFiles::open(FileName file_name, const String& mode) {
    const String& path = FILE_TO_PATH.at(file_name);
    File file = SPIFFS.open(path, mode.c_str());
    if (!file) {
        if (IOT_DEBUG) {
            Serial.println("Error opening file " + path);
        }
    }
    return file;
}

void IOTFiles::clearAllFiles() {
    for (uint8_t i = 0; i < AMOUNT_OF_FILES; i++) {
        lock(static_cast<FileName>(i));
    }
    if (!SPIFFS.format()) {
        if (IOT_DEBUG) {
            Serial.println("Format failed...");
        }
    }
    else {
        if (IOT_DEBUG) {
            Serial.println("Successfully formatted SPIFFS.");
        }
    }
    for (uint8_t i = 0; i < AMOUNT_OF_FILES; i++) {
        unlock(static_cast<FileName>(i));
    }
}

void IOTFiles::clearFile(FileName file_name, bool is_locked) {
    if (!is_locked) {
        lock(file_name);
    }
    File file = open(file_name, "w");
    file.close();
    if (!is_locked) {
        unlock(file_name);
    }
}

void IOTFiles::addEntry(FileName file_name, const String& entry, const bool is_locked) {
    if (!is_locked) {
        lock(file_name);
    }
    File file = open(file_name, "a");
    file.println(entry);
    file.close();
    if (!is_locked) {
        unlock(file_name);
    }
}

void IOTFiles::addEntry(FileName file_name, const String& id, const String& uid, const bool is_locked) {
    addEntry(file_name, id + "," + uid, is_locked);
}

String IOTFiles::readFile(FileName file_name) {
    lock(file_name);
    File file = open(file_name);
    String res;
    while (file.available()) {
        res += static_cast<char>(file.read());
    }
    unlock(file_name);
    return res;
}

void IOTFiles::clearPendingUserList() {
    clearFile(PENDING_USER_LIST);
}

void IOTFiles::addPendingUserEntry(const String& id, const String& uid){
    String entry = id + "," + uid;
    addEntry(PENDING_USER_LIST, entry);
}

// For testing
String IOTFiles::readUserList(){
    return readFile(USER_LIST);
}

String IOTFiles::readPendingUserList(){
    return readFile(PENDING_USER_LIST);
}

String IOTFiles::readAttendanceLog(){
    return readFile(ATTENDANCE_LOG);
}

bool IOTFiles::idExists(const String& id){
    if (IOT_DEBUG) {
        Serial.println("idExists");
    }
    return (!getLineWithString(PENDING_USER_LIST, id).isEmpty() ||
            !getLineWithString(USER_LIST, id).isEmpty());
}

bool IOTFiles::uidApproved(const String& uid){
    if (IOT_DEBUG) {
        Serial.println("uidApproved");
    }
    return (!getLineWithString(USER_LIST, uid).isEmpty());
}

String IOTFiles::getLineWithString(FileName file_name, const String& str){
    lock(file_name);
    File file = open(file_name);
    String line;
    String target_line = "";
    size_t size = file.size();
    line.reserve(size);

    line = file.readStringUntil('\n');
    while(!line.isEmpty()){
        // check if line includes id
        if(line.indexOf(str) != -1 ){
            target_line = line;
            break;
        }
        // read next line
        line = file.readStringUntil('\n');
    }
    file.close();
    unlock(file_name);
    return target_line;
}

std::vector<String> IOTFiles::getChanges(FileName file_name, uint16_t from_line) {
    lock(file_name);
    File file = open(file_name);
    for (int i = 0; i < from_line; i++) {
        file.readStringUntil('\n');
    }

    std::vector<String> pending_changes;
    while (file.available()) {
        String line = file.readStringUntil('\n');
        if (IOT_DEBUG) {
            Serial.println("Got line: \n" + line);
        }
        int cr_index = line.indexOf('\r');
        if (cr_index != -1) {
            if (IOT_DEBUG) {
                Serial.println("Removing cr");
            }
            line.remove(cr_index);
        }
        int nl_index = line.indexOf('\n');
        if (nl_index != -1) {
            if (IOT_DEBUG) {
                Serial.println("Removing nl");
            }
            line.remove(nl_index);
        }
        pending_changes.push_back(line);
    }

    unlock(file_name);
    return pending_changes;
}
