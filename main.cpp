#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <cmath>
#include <cstring>
#include <mutex>
#include <random>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")  // Link Winsock library

inline uint16_t crc16_ccitt(const uint8_t* data, size_t length)
{
    uint16_t crc = 0xFFFF;

    for (size_t i = 0; i < length; ++i)
    {
        crc ^= (uint16_t)data[i] << 8;

        for (int j = 0; j < 8; ++j)
        {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }

    return crc;
}
std::mutex print_mutex;

// ---------------------------
// UDP Helpers for Windows
// ---------------------------
SOCKET create_socket()
{
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET)
    {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        exit(1);
    }
    return sock;
}

void send_packet(SOCKET sock, int port, const std::vector<uint8_t>& packet)
{
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    sendto(sock, reinterpret_cast<const char*>(packet.data()), static_cast<int>(packet.size()), 0,
        reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
}

// ---------------------------
// VOLTAGE SENSOR (1ms) - Port 5001
// ---------------------------
void voltage_sensor()
{
    SOCKET sock = create_socket();

    uint32_t serial = 0x11223344;
    uint8_t msg_id = 0x10;

    std::default_random_engine eng(std::random_device{}());
    std::uniform_int_distribution<int> noise(-5, 5);
    std::uniform_int_distribution<int> rare_event(0, 5000);

    double base = 12000;
    bool spike = false;
    int spike_counter = 0;

    while (true)
    {
        if (rare_event(eng) == 1)
        {
            spike = true;
            spike_counter = 50; // 50ms spike
        }

        if (spike)
        {
            base = 15000; // 15V overvoltage
            spike_counter--;
            if (spike_counter <= 0)
            {
                spike = false;
                base = 12000;
            }
        }

        uint16_t voltage = static_cast<uint16_t>(base + noise(eng));
        uint16_t temp = 35 + noise(eng);

        std::vector<uint8_t> packet;

        packet.push_back(0xAA);
        packet.push_back(0x55);

        packet.insert(packet.end(),
            reinterpret_cast<uint8_t*>(&serial),
            reinterpret_cast<uint8_t*>(&serial) + 4);

        packet.push_back(msg_id);

        packet.insert(packet.end(),
            reinterpret_cast<uint8_t*>(&voltage),
            reinterpret_cast<uint8_t*>(&voltage) + 2);

        packet.insert(packet.end(),
            reinterpret_cast<uint8_t*>(&temp),
            reinterpret_cast<uint8_t*>(&temp) + 2);

        uint16_t crc = crc16_ccitt(packet.data(), packet.size());
        packet.insert(packet.end(),
            reinterpret_cast<uint8_t*>(&crc),
            reinterpret_cast<uint8_t*>(&crc) + 2);

        send_packet(sock, 5001, packet);

        {
            std::lock_guard<std::mutex> lock(print_mutex);
            std::cout << "[Voltage] " << voltage << " mV\n";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// ---------------------------
// GAS SENSOR (10ms) - Port 5002
// ---------------------------
void gas_sensor()
{
    SOCKET sock = create_socket();

    uint32_t serial = 0x55667788;
    uint8_t msg_id = 0x22;

    std::default_random_engine eng(std::random_device{}());
    std::uniform_int_distribution<int> noise(-3, 3);
    std::uniform_int_distribution<int> rare_event(0, 2000);

    int ppm = 50;
    bool leak = false;
    int leak_phase = 0;

    while (true)
    {
        if (rare_event(eng) == 1)
        {
            leak = true;
            leak_phase = 0;
        }

        if (leak)
        {
            ppm += 15; // ramp up leak
            leak_phase++;
            if (leak_phase > 30)
                leak = false;
        }
        else
        {
            ppm = 50 + noise(eng);
        }

        uint8_t alarm = (ppm > 200) ? 1 : 0;

        std::vector<uint8_t> packet;

        packet.push_back(0x7E);
        packet.push_back(msg_id);

        packet.insert(packet.end(),
            reinterpret_cast<uint8_t*>(&serial),
            reinterpret_cast<uint8_t*>(&serial) + 4);

        packet.insert(packet.end(),
            reinterpret_cast<uint8_t*>(&ppm),
            reinterpret_cast<uint8_t*>(&ppm) + 2);

        packet.push_back(alarm);

        uint16_t crc = crc16_ccitt(packet.data(), packet.size());
        packet.insert(packet.end(),
            reinterpret_cast<uint8_t*>(&crc),
            reinterpret_cast<uint8_t*>(&crc) + 2);

        send_packet(sock, 5002, packet);

        {
            std::lock_guard<std::mutex> lock(print_mutex);
            std::cout << "[Gas] " << ppm << " ppm  Alarm: " << (int)alarm << "\n";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// ---------------------------
// LIGHT SENSOR (100ms) - Port 5003
// ---------------------------
void light_sensor()
{
    SOCKET sock = create_socket();

    std::default_random_engine eng(std::random_device{}());
    std::uniform_int_distribution<int> noise(-20, 20);
    std::uniform_int_distribution<int> rare_event(0, 100);

    int lux = 1000;
    bool blocked = false;
    int block_counter = 0;

    while (true)
    {
        if (rare_event(eng) == 1)
        {
            blocked = true;
            block_counter = 20;
        }

        if (blocked)
        {
            lux = 50 + noise(eng);
            block_counter--;
            if (block_counter <= 0)
                blocked = false;
        }
        else
        {
            lux = 1000 + noise(eng);
        }

        uint8_t block_flag = (lux < 200) ? 1 : 0;

        std::vector<uint8_t> packet;

        packet.push_back('L');
        packet.push_back('I');
        packet.push_back('T');
        packet.push_back('E');

        packet.insert(packet.end(),
            reinterpret_cast<uint8_t*>(&lux),
            reinterpret_cast<uint8_t*>(&lux) + 2);

        packet.push_back(block_flag);

        uint16_t crc = crc16_ccitt(packet.data(), packet.size());
        packet.insert(packet.end(),
            reinterpret_cast<uint8_t*>(&crc),
            reinterpret_cast<uint8_t*>(&crc) + 2);

        send_packet(sock, 5003, packet);

        {
            std::lock_guard<std::mutex> lock(print_mutex);
            std::cout << "[Light] " << lux << " lux  Blocked: " << (int)block_flag << "\n";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// ---------------------------
// MAIN
// ---------------------------
int main()
{
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    std::cout << "Starting Windows Virtual Sensor Simulator...\n";

    std::thread v(voltage_sensor);
    std::thread g(gas_sensor);
    std::thread l(light_sensor);

    v.join();
    g.join();
    l.join();

    WSACleanup();
    return 0;
}