#include "serial_obj.h"
#include <windows.h>
#include <iostream>
#include <string>
#include <sstream>

// Global flag for Ctrl+C handling
bool g_running = true;

/**
 * @brief Console control handler for graceful shutdown
 */
BOOL WINAPI consoleHandler(DWORD ctrlType) {
    if (ctrlType == CTRL_C_EVENT || ctrlType == CTRL_BREAK_EVENT) {
        std::cout << "\n\nShutting down gracefully..." << std::endl;
        g_running = false;
        return TRUE;
    }
    return FALSE;
}

/**
 * @brief Parse pitch and roll values from glove data
 */
bool parsePitchRoll(const std::string& data, float& pitch, float& roll) {
    size_t pitchPos = data.find("Pitch:");
    if (pitchPos == std::string::npos) {
        pitchPos = data.find("pitch:");
    }

    size_t rollPos = data.find("Roll:");
    if (rollPos == std::string::npos) {
        rollPos = data.find("roll:");
    }

    if (pitchPos == std::string::npos || rollPos == std::string::npos) {
        return false;
    }

    try {
        // Extract pitch value
        size_t pitchStart = pitchPos + 6;
        std::string pitchStr = "";
        for (size_t i = pitchStart; i < data.length(); i++) {
            char c = data[i];
            if ((c >= '0' && c <= '9') || c == '.' || c == '-' || c == '+') {
                pitchStr += c;
            }
            else if (c != ' ' && c != '\t') {
                break;
            }
        }

        // Extract roll value
        size_t rollStart = rollPos + 5;
        std::string rollStr = "";
        for (size_t i = rollStart; i < data.length(); i++) {
            char c = data[i];
            if ((c >= '0' && c <= '9') || c == '.' || c == '-' || c == '+') {
                rollStr += c;
            }
            else if (c != ' ' && c != '\t') {
                break;
            }
        }

        // Convert to float
        if (!pitchStr.empty() && !rollStr.empty()) {
            pitch = std::stof(pitchStr);
            roll = std::stof(rollStr);
            return true;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Conversion error: " << e.what() << std::endl;
        return false;
    }

    return false;
}

/**
 * @brief Determine servo command based on roll angle
 * @param roll Roll angle in degrees
 * @return Command string: "L" (left), "R" (right), or "S" (stop)
 */
std::string moveServo_(float pitch, float roll) {
    if (pitch > 30) {
        return "L";  // Tilt right -> move left
    }
    else if (pitch < -30) {
        return "R";  // Tilt left -> move right
    }
    

    if (roll > 30) {
        return "U";  // Tilt right -> move left
    }
    else if (roll < -30) {
        return "D";  // Tilt left -> move right
    }
    
    return "S";
}


/**
 * @brief Main function
 */
int main() {
    // Set up Ctrl+C handler
    if (!SetConsoleCtrlHandler(consoleHandler, TRUE)) {
        std::cerr << "Warning: Could not set control handler" << std::endl;
    }

    // Create serial objects
    Serial_obj serial_glv;
    Serial_obj serial_rbt;

    const std::string COM_PORT_GLV = "COM9";
    const std::string COM_PORT_RBT = "COM11";
    const DWORD BAUD_RATE = CBR_9600;

    // Open glove port
    if (!serial_glv.open(COM_PORT_GLV, BAUD_RATE)) {
        std::cerr << "Failed to open glove port " << COM_PORT_GLV << std::endl;
        return 1;
    }
    std::cout << "Glove connected on " << COM_PORT_GLV << std::endl;

    // Open robot port
    if (!serial_rbt.open(COM_PORT_RBT, BAUD_RATE)) {
        std::cerr << "Failed to open robot port " << COM_PORT_RBT << std::endl;
        serial_glv.close();
        return 1;
    }
    std::cout << "Robot connected on " << COM_PORT_RBT << std::endl;

    std::cout << "\nListening for glove data (Ctrl+C to exit)...\n" << std::endl;

    // Buffers
    char readBuffer[256];
    float pitch = 0.0f;
    float roll = 0.0f;
    std::string lastCommand = "";

    const int SLEEP_MS = 50;  // 20Hz update rate
    int emptyReads = 0;

    // Main loop
    while (g_running) {
        DWORD bytesAvailable = serial_glv.getBytesAvailable();

        if (bytesAvailable > 0) {
            DWORD bytesRead = 0;

            // Read glove data
            if (serial_glv.read(readBuffer, sizeof(readBuffer) - 1, bytesRead)) {
                if (bytesRead > 0) {
                    readBuffer[bytesRead] = '\0';

                    // Parse pitch and roll
                    if (parsePitchRoll(readBuffer, pitch, roll)) {
                        // Determine servo command
                        std::string command = moveServo_(pitch, roll);
                            // Send command to robot
                            if (serial_rbt.writeString(command)) {
                                std::cout << "pitch: " << pitch
                                    << " -> Command: " << command << std::endl;
                                
                            }
                            else {
                                std::cerr << "Failed to send command to robot" << std::endl;
                            }
                        

                        emptyReads = 0;
                    }
                }
            }
            else {
                std::cerr << "Glove read error: "
                    << Serial_obj::getLastErrorString() << std::endl;
            }
        }
        else {
            emptyReads++;

            // Status message every 5 seconds
            if (emptyReads % 100 == 0) {
                std::cout << "[Waiting for glove data... "
                    << (emptyReads * SLEEP_MS / 1000)
                    << "s]" << std::endl;
            }
        }

        Sleep(SLEEP_MS);
    }

    // Cleanup
    std::cout << "\nSending stop command..." << std::endl;
    serial_rbt.writeString("S");  // Stop servo before exit
    Sleep(100);

    serial_glv.close();
    serial_rbt.close();
    std::cout << "Program terminated." << std::endl;

    return 0;
}