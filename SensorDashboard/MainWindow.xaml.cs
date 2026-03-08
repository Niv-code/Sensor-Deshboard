using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Threading;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Diagnostics;

namespace SensorDashboard
{
    public partial class MainWindow : Window
    {
        private SensorListener voltageListener;
        private SensorListener gasListener;
        private SensorListener lightListener;
        private ushort lastPpm = 0;
        private int gasRiseCount = 0;

        public MainWindow()
        {
            InitializeComponent();
            StartSensors();
        }
        public void StartSensors()
        {

            voltageListener = new SensorListener(5001);
            gasListener = new SensorListener(5002);
            lightListener = new SensorListener(5003);


            voltageListener.OnPacketReceived += ProcessVoltage;
            gasListener.OnPacketReceived += ProcessGas;
            lightListener.OnPacketReceived += ProcessLight;


            voltageListener.start();
            gasListener.start();
            lightListener.start();
        }
        /*Calls PacketParser.CheckCRC If val calls PacketParser.ParseVoltage 
         * then uses Dispatcher.Invoke to update the UI labels Checks for Surge (>14000)*/

        private void ProcessVoltage(byte[] rData)
        {
            if (!PacketParser.CheckCRC(rData))
                return;
            var data = PacketParser.ParseVoltage(rData);
            if (data == null)
                return;

            Application.Current.Dispatcher.Invoke(() =>
            {
                txtVoltage.Text = $"{data.Voltage} mV";
                txtTemp.Text = $"{data.Temp} °C";
                voltageBorder.Background = data.Voltage > 14000 ? Brushes.Red : Brushes.Green;
            });  
        }
        private void ProcessGas(byte[] rawData)
        {
            if (!PacketParser.CheckCRC(rawData)) return;

            var data = PacketParser.ParseGas(rawData);
            if (data == null) return;

            Dispatcher.Invoke(() =>
            {
                txtPpm.Text = $"{data.PPM} PPM";

                
                if (data.PPM > lastPpm) gasRiseCount++;
                else gasRiseCount = 0;

                lastPpm = data.PPM;

                
                gasBorder.Background = (gasRiseCount >= 10 || data.IsAlarm) ? Brushes.Red : Brushes.Green;
            });
        }

        private void ProcessLight(byte[] rawData)
        {
            if (!PacketParser.CheckCRC(rawData)) return;

            var data = PacketParser.ParseLight(rawData);
            if (data == null) return;

            Dispatcher.Invoke(() =>
            {
                txtLux.Text = $"{data.Lux} Lux";

                
                lightBorder.Background = data.Lux < 200 ? Brushes.Black : Brushes.Yellow;
                txtLux.Foreground = data.Lux < 200 ? Brushes.White : Brushes.Black;
            });
        }
    }
}