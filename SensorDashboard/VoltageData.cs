using System;
using System.Collections.Generic;
using System.Text;

namespace SensorDashboard
{
    public class VoltageData
    {
        public uint SerialNumber { get; set; }
        public ushort Voltage { get; set; }
        public ushort Temp { get; set; }
    }
    public class GasData
    {
        public uint SerialNumber { get; set; }
        public ushort PPM { get; set; }
        //True if alarm byte is 1
        public bool IsAlarm { get; set; }
    }
    public class LightData
    {
        public ushort Lux { get; set; }
        public bool IsBlocked { get; set; }
    }
    // A helper class to convert bytes into  numbers and validate integrity
    public static class PacketParser
    {
        private static ushort CalculateCRC16(byte[] data, int length)//Function
        {
            ushort crc = 0xFFFF;

            for (int i = 0; i < length; i++)
            {
                crc ^= (ushort)(data[i] << 8); 

                for (int j = 0; j < 8; j++) 
                {
                    if ((crc & 0x8000) != 0)
                    {
                        crc = (ushort)((crc << 1) ^ 0x1021); 
                    }
                    else
                    {
                        crc <<= 1;
                    }
                }
            }
            return crc;
        }
        /*Calculates the CRC16-CCITT of the packet (excluding the last 2 bytes) 
         *and compares it to the received CRC (the last 2 bytes). Returns true if they match.*/

        public static bool CheckCRC(Byte[] packet)
        {
            if (packet == null || packet.Length < 3)
                    return false;

            ushort receivedCRC = BitConverter.ToUInt16(packet, packet.Length - 2);
            ushort calculatedCRC = CalculateCRC16(packet, packet.Length - 2);
            return receivedCRC == calculatedCRC;
        }
        //Extracts bytes [7-8] as Voltage  and [9-10] as Temp 

        public static VoltageData ParseVoltage(byte[] data)
        {
            if (data.Length != 13)
                return null;
            return new VoltageData
            {
                SerialNumber = BitConverter.ToUInt32(data, 2),
                Voltage = BitConverter.ToUInt16(data, 7),
                Temp = BitConverter.ToUInt16(data,9)
            };

        }
        // Extracts bytes [6-7] as PPM  and byte [8] as Alarm 

        public static GasData ParseGas(byte[] data)
        {
            if (data.Length != 11) return null;

            return new GasData
            {
                SerialNumber = BitConverter.ToUInt32(data, 2),
                PPM = BitConverter.ToUInt16(data, 6),
                IsAlarm = data[8] == 1 
            };
        }
        //Extracts bytes [4-5] 

        public static LightData ParseLight(Byte[] data)
        {
            if (data.Length != 9) return null;

            return new LightData
            {
                Lux = BitConverter.ToUInt16(data, 4),
                IsBlocked = data[6] == 1
            };
        }
    }
}
   