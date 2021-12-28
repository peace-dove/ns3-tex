#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/spectrum-module.h"
#include "ns3/wifi-module.h"
#include "ns3/propagation-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include <iostream>
#include <iomanip>
#include <fstream>

using namespace ns3;

std::ofstream g_fileApRx;

// 计算数据包个数
uint64_t g_packetCount;

bool ApRxTrace(Ptr<NetDevice> device, Ptr<const Packet> p, uint16_t protocol, const Address &from)
{
  g_fileApRx << Simulator::Now().GetSeconds() << " 0 " << Mac48Address::ConvertFrom(from) << " " << p->GetSize() << std::endl;
  g_packetCount++;
  return true;
}

int main(int argc, char *argv[])
{
  bool pcap = false;
  // 工作站点数量
  uint32_t numStas = 1;
  // 工作站点距离AP 单位米
  uint32_t radius = 25;
  // 利用是否打开 设置RTSCTS的阈值 这个命令用于设置数据包长度的极限值，当超过这个极限值时无线接入点需要先向接受端发rts信号，得到接受端反馈后才能发送数据
  bool enableRTSCTS = false;
  // 默认数据包大小
  uint32_t packetSize = 1900;
  // 争用窗口的设定
  uint32_t cwMin = 15;
  uint32_t cwMax = 1023;
  // 仿真时间为durtion+1
  double duration = 10;
  // 每一秒到达的数据包个数
  double packetArrivalRate = 1;
  std::string fileNameApRx = "wifi-dcf-ap-rx-trace.dat";
  std::string fileNameStaTx = "wifi-dcf-sta-tx-trace.dat";
  std::string fileNameRxOk = "wifi-dcf-rx-ok-trace.dat";
  std::string fileNameRxError = "wifi-dcf-rx-error-trace.dat";
  std::string fileNamePhyTx = "wifi-dcf-phy-tx.dat";
  std::string fileNameState = "wifi-dcf-state-trace.dat";

  CommandLine cmd;
  cmd.AddValue("pcap", "Print pcap trace information if true", pcap);
  cmd.AddValue("packetSize", "Set packet size (bytes)", packetSize);
  cmd.AddValue("packetArrivalRate", "Packet arrival rate per second", packetArrivalRate);
  cmd.AddValue("numStas", "Number of STA devices", numStas);
  cmd.AddValue("cwMin", "CwMin parameter of DCF", cwMin);
  cmd.AddValue("cwMax", "CwMax parameter of DCF", cwMax);
  cmd.AddValue("duration", "Duration of data logging phase (s)", duration);
  cmd.AddValue("radius", "Radius for node dropping around AP (m)", radius);
  cmd.AddValue("enableRTSCTS", "Turn on or turn off the RTS/CTS model", enableRTSCTS);
  cmd.Parse(argc, argv);

  g_fileApRx.open(fileNameApRx.c_str(), std::ofstream::out);

  g_packetCount = 0;

  Packet::EnablePrinting();

  NodeContainer ap;   //接入点AP
  NodeContainer stas; //工作站

  ap.Create(1);         //创建一个AP
  stas.Create(numStas); //创建numStats个工作点

  // 物理层使用spectrum channel模型
  Ptr<SingleModelSpectrumChannel> spectrumChannel = CreateObject<SingleModelSpectrumChannel>();
  Ptr<FriisPropagationLossModel> lossModel = CreateObject<FriisPropagationLossModel>();
  spectrumChannel->AddPropagationLossModel(lossModel);
  Ptr<ConstantSpeedPropagationDelayModel> delayModel = CreateObject<ConstantSpeedPropagationDelayModel>();
  spectrumChannel->SetPropagationDelayModel(delayModel);

  // 参考类似YansWifiPhyHelper
  SpectrumWifiPhyHelper wifiPhy = SpectrumWifiPhyHelper::Default(); //和SpectrumChannel固定搭配
  wifiPhy.SetChannel(spectrumChannel);

  wifiPhy.SetErrorRateModel("ns3::YansErrorRateModel"); // 错误率模型
  wifiPhy.Set("Frequency", UintegerValue(5180));        // 信道编号36 宽度20MHz
  wifiPhy.Set("TxPowerStart", DoubleValue(16));         // 40 mW at 5 GHz
  wifiPhy.Set("TxPowerEnd", DoubleValue(16));           // 40 mW dBm

  // MAC
  WifiHelper wifi;
  WifiMacHelper wifiMac;
  Ssid ssid = Ssid("wifi-dcf");
  wifi.SetStandard(WIFI_PHY_STANDARD_80211a); // wifi的标准
  wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                               "DataMode", StringValue("OfdmRate54Mbps"),
                               "ControlMode", StringValue("OfdmRate24Mbps"),
                               "RtsCtsThreshold", enableRTSCTS ? UintegerValue(0) : UintegerValue(999999));

  // container
  NetDeviceContainer apDev;
  NetDeviceContainer staDevs;

  // AP
  wifiMac.SetType("ns3::ApWifiMac",
                  "Ssid", SsidValue(ssid));
  apDev = wifi.Install(wifiPhy, wifiMac, ap);

  // 工作站
  wifiMac.SetType("ns3::StaWifiMac",
                  "Ssid", SsidValue(ssid));
  staDevs = wifi.Install(wifiPhy, wifiMac, stas);

  // 设置争用窗口大小 使用全局变量设计
  Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/Txop/MinCw", UintegerValue(cwMin));
  Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/Txop/MaxCw", UintegerValue(cwMax));

  // 设置AP和工作点的位置，将AP放在中心点 模型是一个圆形
  MobilityHelper mobility;
  Ptr<UniformDiscPositionAllocator> positionAllocator = CreateObject<UniformDiscPositionAllocator>();
  positionAllocator->SetRho(radius);
  mobility.SetPositionAllocator(positionAllocator);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(stas);

  Ptr<ListPositionAllocator> apPositionAllocator = CreateObject<ListPositionAllocator>();
  apPositionAllocator->Add(Vector(0.0, 0.0, 0.0)); // 中心点的位置
  mobility.SetPositionAllocator(apPositionAllocator);
  mobility.Install(ap);

  // 为每个容器设置套接字 为了协助数据包处理
  PacketSocketHelper packetSocket;
  packetSocket.Install(stas);
  packetSocket.Install(ap);

  PacketSocketAddress socket;
  // 在开始的时候设置一个微小抖动
  Ptr<UniformRandomVariable> startTimeVariable = CreateObject<UniformRandomVariable>();
  startTimeVariable->SetAttribute("Max", DoubleValue(0.1));
  // 数据率等于每秒到达的数据包*数据包数*8 转化为bps 这也是传输率的设置
  uint64_t bps = static_cast<uint64_t>(packetArrivalRate * packetSize * 8);
  // 对每一个sta设置socket和应用层
  for (uint32_t i = 0; i < staDevs.GetN(); i++)
  {
    socket.SetSingleDevice(staDevs.Get(i)->GetIfIndex());
    socket.SetPhysicalAddress(apDev.Get(0)->GetAddress());
    socket.SetProtocol(1);
    OnOffHelper onoff("ns3::PacketSocketFactory", Address(socket)); //产生数据包factory 地址
    onoff.SetConstantRate(DataRate(bps), packetSize);               //设置恒定的速率 bps就是速率 datarate packetSize

    ApplicationContainer apps = onoff.Install(stas.Get(i));
    apps.StartWithJitter(Seconds(1.0), startTimeVariable);
    // 设置仿真时间
    apps.Stop(Seconds(duration + 1) + MilliSeconds(100));
  }

  // 启动时间高达100ms的抖动
  // 仿真结束的时间在duration + 1秒
  Simulator::Stop(Seconds(duration + 1) + MilliSeconds(100));

  // Set traces
  Ptr<WifiNetDevice> apNetDevice = DynamicCast<WifiNetDevice>(apDev.Get(0));
  // 为AP设置了回调函数
  apNetDevice->SetReceiveCallback(MakeCallback(&ApRxTrace));

  if (pcap)
  {
    //生成pcap文件
    wifiPhy.EnablePcap("wifi-dcf", apDev.Get(0));
  }

  Simulator::Run();

  std::cout << "Throughput observed at AP: " << ((g_packetCount * packetSize * 8) / duration) / 1e6 << " Mb/s" << std::endl;

  Simulator::Destroy();

  g_fileApRx.close();
  return 0;
}
