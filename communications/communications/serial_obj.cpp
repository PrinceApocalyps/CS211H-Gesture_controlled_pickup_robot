#include "serial_obj.h"
#include <iostream>

// Default constructor
Serial_obj::Serial_obj()
    : hSerial(INVALID_HANDLE_VALUE)
    , baudRate(CBR_9600)
    , isConnected(false) {
    ZeroMemory(&dcbSerialParams, sizeof(DCB));
    dcbSerialParams.DCBlength = sizeof(DCB);
}

// Parameterized constructor
Serial_obj::Serial_obj(const std::string& port, DWORD baud)
    : hSerial(INVALID_HANDLE_VALUE)
    , portName(port)
    , baudRate(baud)
    , isConnected(false) {
    ZeroMemory(&dcbSerialParams, sizeof(DCB));
    dcbSerialParams.DCBlength = sizeof(DCB);
    open(port, baud);
}

// Destructor
Serial_obj::~Serial_obj() {
    close();
}

// Open and configure the serial port
bool Serial_obj::open(const std::string& port, DWORD baud) {
    // Close if already open
    if (isConnected) {
        close();
    }

    portName = port;
    baudRate = baud;

    // Format port name for Windows (e.g., "COM9" -> "\\\\.\\COM9")
    std::string fullPortName = "\\\\.\\" + port;

    std::cout << "Opening " << port << " at " << baud << " baud..." << std::endl;

    // Open the serial port
    hSerial = CreateFileA(
        fullPortName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,                    // No sharing
        NULL,                 // No security attributes
        OPEN_EXISTING,        // Port must exist
        0,                    // Non-overlapped I/O
        NULL                  // No template
    );

    if (hSerial == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        std::cerr << "ERROR: Unable to open " << port << std::endl;

        if (error == ERROR_FILE_NOT_FOUND) {
            std::cerr << "-> Port not found. Check Device Manager." << std::endl;
        }
        else if (error == ERROR_ACCESS_DENIED) {
            std::cerr << "-> Access denied. Close other programs using this port." << std::endl;
        }
        else {
            std::cerr << "-> Error code: " << error << std::endl;
        }
        return false;
    }

    std::cout << "? Port opened successfully!" << std::endl;

    // Get current serial port configuration
    if (!GetCommState(hSerial, &dcbSerialParams)) {
        std::cerr << "Error: Could not get serial port state." << std::endl;
        CloseHandle(hSerial);
        hSerial = INVALID_HANDLE_VALUE;
        return false;
    }

    // Configure serial port parameters
    dcbSerialParams.BaudRate = baud;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;
    dcbSerialParams.fRtsControl = RTS_CONTROL_ENABLE;

    if (!SetCommState(hSerial, &dcbSerialParams)) {
        std::cerr << "Error: Could not set serial port parameters." << std::endl;
        CloseHandle(hSerial);
        hSerial = INVALID_HANDLE_VALUE;
        return false;
    }

    // Configure timeouts
    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout = 50;          // Max time between chars (ms)
    timeouts.ReadTotalTimeoutConstant = 50;     // Base timeout for read
    timeouts.ReadTotalTimeoutMultiplier = 10;   // Multiplier per byte
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(hSerial, &timeouts)) {
        std::cerr << "Error: Could not set timeouts." << std::endl;
        CloseHandle(hSerial);
        hSerial = INVALID_HANDLE_VALUE;
        return false;
    }

    isConnected = true;
    std::cout << "Serial port configured and ready." << std::endl;
    return true;
}

// Close the serial port
void Serial_obj::close() {
    if (hSerial != INVALID_HANDLE_VALUE) {
        CloseHandle(hSerial);
        hSerial = INVALID_HANDLE_VALUE;
        isConnected = false;
        std::cout << "Serial port closed." << std::endl;
    }
}

// Read data from serial port
bool Serial_obj::read(char* buffer, DWORD bufferSize, DWORD& bytesRead) {
    if (!isConnected || hSerial == INVALID_HANDLE_VALUE) {
        return false;
    }

    return ReadFile(hSerial, buffer, bufferSize, &bytesRead, NULL) != 0;
}

// Write data to serial port
bool Serial_obj::write(const char* data, DWORD dataSize, DWORD& bytesWritten) {
    if (!isConnected || hSerial == INVALID_HANDLE_VALUE) {
        return false;
    }

    return WriteFile(hSerial, data, dataSize, &bytesWritten, NULL) != 0;
}

// Write string to serial port
bool Serial_obj::writeString(const std::string& data) {
    DWORD bytesWritten = 0;
    return write(data.c_str(), static_cast<DWORD>(data.length()), bytesWritten);
}

// Get number of bytes available to read
DWORD Serial_obj::getBytesAvailable() {
    if (!isConnected || hSerial == INVALID_HANDLE_VALUE) {
        return 0;
    }

    COMSTAT comStat;
    DWORD errors;

    if (ClearCommError(hSerial, &errors, &comStat)) {
        return comStat.cbInQue;
    }

    return 0;
}

// Check if connected
bool Serial_obj::connected() const {
    return isConnected;
}

// Get last error as string
std::string Serial_obj::getLastErrorString() {
    DWORD error = GetLastError();
    if (error == 0) {
        return "No error";
    }

    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&messageBuffer,
        0,
        NULL
    );

    std::string message(messageBuffer, size);
    LocalFree(messageBuffer);
    return message;
}

// Flush serial port buffers
bool Serial_obj::flush() {
    if (!isConnected || hSerial == INVALID_HANDLE_VALUE) {
        return false;
    }

    return PurgeComm(hSerial,
        PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT) != 0;
}