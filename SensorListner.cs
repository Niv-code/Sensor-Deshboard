using System;
using System.Net.Sockets;
using System.Threading.Tasks;
using System.Diagnostics;

namespace SensorDashboard
{
    class SensorListener
    {
        
        private UdpClient udpClient;
        private int port;
        private bool isRunnuing;
        public event Action<byte[]> OnPacketReceived;

        public SensorListener(int p)
        {
            port = p;
        }
        
        public async void start()
        {
            if (isRunnuing)
                return;
            try
            {
                udpClient = new UdpClient(port);
                isRunnuing = true;

                Debug.WriteLine($"[Listener] Started listening on port {port}");

                while (isRunnuing)
                {
                    var result = await udpClient.ReceiveAsync();

                    Debug.WriteLine($"[{DateTime.Now:HH:mm:ss.fff}] Packet received on port {port}");

                    OnPacketReceived?.Invoke(result.Buffer);
                }
            }
            catch (ObjectDisposedException)
            {
                Debug.WriteLine($"[Error] Stopped on port {port}");
            }
            catch (Exception ex)
            {
                Debug.WriteLine($"[Error] port {port}: {ex.Message}");
            }
            finally
            {
                isRunnuing = false;
            }
        }
        
        public void stop()
        {
            isRunnuing = false;

            udpClient?.Close();
            udpClient = null;
        }
      
    }
}
