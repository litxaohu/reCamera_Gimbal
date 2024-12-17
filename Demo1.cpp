#include <windows.h>
#include <winusb.h>
#include <setupapi.h>
#include <iostream>

#pragma comment(lib, "winusb.lib")
#pragma comment(lib, "setupapi.lib")

// 示例设备的GUID
GUID USB_DEVICE_GUID = {0xa503e2d3, 0xa031, 0x49dc, {0xb6, 0x84, 0xc9, 0x90, 0x85, 0xdb, 0xfe, 0x92}};

bool InitializeDevice(WINUSB_INTERFACE_HANDLE &deviceHandle) {
    HDEVINFO deviceInfo;
    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
    SP_DEVICE_INTERFACE_DETAIL_DATA *deviceInterfaceDetailData = nullptr;
    ULONG length = 0;
    bool result = false;

    deviceInfo = SetupDiGetClassDevs(&USB_DEVICE_GUID, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (deviceInfo == INVALID_HANDLE_VALUE) {
        return false;
    }

    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    if (!SetupDiEnumDeviceInterfaces(deviceInfo, nullptr, &USB_DEVICE_GUID, 0, &deviceInterfaceData)) {
        SetupDiDestroyDeviceInfoList(deviceInfo);
        return false;
    }

    SetupDiGetDeviceInterfaceDetail(deviceInfo, &deviceInterfaceData, nullptr, 0, &length, nullptr);
    deviceInterfaceDetailData = (SP_DEVICE_INTERFACE_DETAIL_DATA *)malloc(length);
    if (!deviceInterfaceDetailData) {
        SetupDiDestroyDeviceInfoList(deviceInfo);
        return false;
    }

    deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
    if (!SetupDiGetDeviceInterfaceDetail(deviceInfo, &deviceInterfaceData, deviceInterfaceDetailData, length, &length, nullptr)) {
        free(deviceInterfaceDetailData);
        SetupDiDestroyDeviceInfoList(deviceInfo);
        return false;
    }

    HANDLE deviceHandleRaw = CreateFile(deviceInterfaceDetailData->DevicePath, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, nullptr);
    free(deviceInterfaceDetailData);
    SetupDiDestroyDeviceInfoList(deviceInfo);

    if (deviceHandleRaw == INVALID_HANDLE_VALUE) {
        return false;
    }

    if (!WinUsb_Initialize(deviceHandleRaw, &deviceHandle)) {
        CloseHandle(deviceHandleRaw);
        return false;
    }

    return true;
}

void SendCommand(WINUSB_INTERFACE_HANDLE deviceHandle, UCHAR *buffer, ULONG bufferLength) {
    ULONG bytesWritten;
    if (!WinUsb_WritePipe(deviceHandle, 0x01, buffer, bufferLength, &bytesWritten, nullptr)) {
        std::cerr << "Failed to send command" << std::endl;
    }
}

int main() {
    WINUSB_INTERFACE_HANDLE deviceHandle;
    if (!InitializeDevice(deviceHandle)) {
        std::cerr << "Failed to initialize device" << std::endl;
        return 1;
    }

    // 电机开启命令
    UCHAR motorOnCommand[] = {0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // ID: 141, DATA: 88 00 00 00 00 00 00 00

    // 发送电机开启命令
    SendCommand(deviceHandle, motorOnCommand, sizeof(motorOnCommand));

    // 示例命令：位置闭环2，速度120dps，角度+180
    UCHAR command[] = {0xA4, 0x00, 0x78, 0x00, 0x50, 0x46, 0x00, 0x00}; // ID: 141, DATA: A4 00 78 00 50 46 00 00

    // 发送控制命令
    SendCommand(deviceHandle, command, sizeof(command));

    // 清理
    WinUsb_Free(deviceHandle);

    return 0;
}