#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")

#define PORT 9  // Wake-on-LAN default port
#define MAC_SIZE 6
#define MAGIC_PACKET_SIZE 102
#define TIME_WINDOW 3  // 3 seconds window

void initialize_winsock() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        printf("WSAStartup failed: %d\n", result);
        exit(1);
    }
}

SOCKET create_socket() {
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        printf("Socket creation failed: %ld\n", WSAGetLastError());
        WSACleanup();
        exit(1);
    }
    return sock;
}

void bind_socket(SOCKET sock) {
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Bind failed: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        exit(1);
    }
}

int get_mac_address(unsigned char *mac_address) {
    IP_ADAPTER_INFO AdapterInfo[16];
    DWORD dwBufLen = sizeof(AdapterInfo);
    DWORD dwStatus = GetAdaptersInfo(AdapterInfo, &dwBufLen);
    if (dwStatus != ERROR_SUCCESS) {
        printf("GetAdaptersInfo failed: %d\n", dwStatus);
        return 0;
    }

    PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;
    while (pAdapterInfo) {
        if (pAdapterInfo->Type == MIB_IF_TYPE_ETHERNET) {
            memcpy(mac_address, pAdapterInfo->Address, MAC_SIZE);
            return 1;
        }
        pAdapterInfo = pAdapterInfo->Next;
    }
    return 0;
}

int verify_magic_packet(const unsigned char *buffer, size_t size, const unsigned char *mac_address) {
    if (size != MAGIC_PACKET_SIZE) return 0;

    // Check for 6 bytes of 0xFF
    for (int i = 0; i < 6; i++) {
        if (buffer[i] != 0xFF) return 0;
    }

    // Check for 16 repetitions of the MAC address
    for (int repetition = 0; repetition < 16; repetition++) {
        for (int i = 0; i < 6; i++) {
            if (buffer[6 + repetition * 6 + i] != mac_address[i]) return 0;
        }
    }

    return 1;
}

void shutdown_computer() {
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;

    // Get a token for this process.
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        printf("OpenProcessToken failed: %d\n", GetLastError());
        return;
    }

    // Get the LUID for the shutdown privilege.
    LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

    tkp.PrivilegeCount = 1;  // one privilege to set
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    // Get the shutdown privilege for this process.
    AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);

    // Cannot test the return value of AdjustTokenPrivileges.
    if (GetLastError() != ERROR_SUCCESS) {
        printf("AdjustTokenPrivileges failed: %d\n", GetLastError());
        return;
    }

    // Shut down the system and force all applications to close.
    if (!ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, SHTDN_REASON_MAJOR_OTHER)) {
        printf("ExitWindowsEx failed: %d\n", GetLastError());
    }
}

void receive_magic_packet(SOCKET sock, const unsigned char *mac_address) {
    struct sockaddr_in client_addr;
    int client_addr_size = sizeof(client_addr);
    unsigned char buffer[1024];
    int count = 0;
    time_t last_time = 0;

    while (1) {
        int bytes_received = recvfrom(sock, (char *)buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_addr_size);
        if (bytes_received == SOCKET_ERROR) {
            printf("recvfrom failed: %d\n", WSAGetLastError());
            closesocket(sock);
            WSACleanup();
            exit(1);
        }

        printf("Received packet from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        time_t current_time = time(NULL);
        if (verify_magic_packet(buffer, bytes_received, mac_address)) {
            if (difftime(current_time, last_time) <= TIME_WINDOW) {
                count++;
            } else {
                count = 1;
            }

            last_time = current_time;

            if (count == 3) {
                printf("3 magic packets received. Shutting down... \n");
                shutdown_computer();
                // count = 0;  // Reset count after printing "hello"
            }
        } else {
            count = 0;  // Reset count if an invalid packet is received
        }
    }
}

int main() {
    unsigned char mac_address[MAC_SIZE];

    if (!get_mac_address(mac_address)) {
        printf("Failed to retrieve MAC address.\n");
        return 1;
    }

    printf("MAC Address: ");
    for (int i = 0; i < MAC_SIZE; i++) {
        printf("%02X", mac_address[i]);
        if (i < MAC_SIZE - 1) {
            printf(":");
        }
    }
    printf("\n");

    initialize_winsock();
    SOCKET sock = create_socket();
    bind_socket(sock);
    printf("Listening for magic packets on port %d...\n", PORT);
    receive_magic_packet(sock, mac_address);

    closesocket(sock);
    WSACleanup();
    return 0;
}
