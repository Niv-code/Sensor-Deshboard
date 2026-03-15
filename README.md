# Real-Time Sensor Dashboard 

## About The Project
An industrial grade real time sensor monitoring system. This project demonstrates a classic hardware software architecture, featuring a sensor simulator that broadcasts data over the network, and a graphical dashboard that receives, parses, and displays it instantly.

## Technologies Built With
* **Hardware Simulator:** `C++` (using the `Winsock` library for networking).
* **Dashboard (UI):** `C#` and `WPF` (.NET).
* **Network Protocol:** `UDP` (Ports: 5001, 5002, 5003).

## Key Features
*  **Asynchronous Communication:** Utilizes `async/await` in C# to listen for network packets in the background without blocking the UI thread.
*  **Data Integrity:** Implements the `CRC16-CCITT` algorithm in both languages to ensure no bytes are corrupted or altered during network transmission.
*  **Anomaly Detection & Dynamic UI:** The interface updates in real-time based on incoming data (via `Dispatcher.Invoke`):
  * **Voltage Sensor:** Triggers a red overload warning when the voltage exceeds 14,000mV.
  * **Gas Sensor:** Triggers a leak warning upon detecting a continuous rise in PPM over time or when an alarm flag is received.
  * **Light Sensor:** Changes the background to black when the sensor is blocked (below 200 Lux).
*  **Clean Architecture:** The C# code is modularly structured into network listeners (`SensorListener`), a static parsing utility (`PacketParser`), and the UI logic (`MainWindow`).

## Packet Structure
Data is transmitted from C++ as raw bytes and parsed in C#. Example of the Voltage packet structure:
* `[0-1]` Header
* `[2-5]` Serial Number (uint32)
* `[6]` Message ID
* `[7-8]` Voltage in mV (ushort)
* `[9-10]` Temperature in Celsius (ushort)
* `[11-12]` CRC16 Checksum
