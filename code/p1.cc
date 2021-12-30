#include <string>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/error-model.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"
#include "ns3/enum.h"
#include "ns3/event-id.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpComparision");

AsciiTraceHelper ascii;
Ptr<PacketSink> cbrSinks[5], tcpSink;

int totalVal;
int total_drops = 0;
bool first_drop = true;

// 统计丢失的数据包
static void
RxDrop(Ptr<OutputStreamWrapper> stream, Ptr<const Packet> p)
{
    if (first_drop)
    {
        first_drop = false;
        *stream->GetStream() << 0 << " " << 0 << std::endl;
    }
    *stream->GetStream() << Simulator::Now().GetSeconds() << " " << ++total_drops << std::endl;
}

// 统计所有获取的数据包
static void
TotalRx(Ptr<OutputStreamWrapper> stream)
{
    totalVal = tcpSink->GetTotalRx();

    for (int i = 0; i < 5; i++)
    {
        totalVal += cbrSinks[i]->GetTotalRx();
    }

    *stream->GetStream() << Simulator::Now().GetSeconds() << " " << totalVal << std::endl;

    Simulator::Schedule(Seconds(0.0001), &TotalRx, stream);
}

// 显示拥塞窗口变化函数
static void
CwndChange(Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
    *stream->GetStream() << Simulator::Now().GetSeconds() << " " << newCwnd << std::endl;
}

static void
TraceCwnd(Ptr<OutputStreamWrapper> stream)
{
    // 追踪拥塞窗口的变化
    Config::ConnectWithoutContext("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeBoundCallback(&CwndChange, stream));
}

int main(int argc, char *argv[])
{

    bool tracing = false;               // 默认关闭追踪信息
    uint32_t maxBytes = 0;              // 最大字节限制
    std::string prot = "TcpWestwood";   // 默认拥塞算法
    double error = 0.000001;

    CommandLine cmd;
    cmd.AddValue("tracing", "Flag to enable/disable tracing", tracing);
    cmd.AddValue("maxBytes",
                 "Total number of bytes for application to send", maxBytes);
    cmd.AddValue("prot", "Transport protocol to use: TcpNewReno, "
                         "TcpHybla, TcpHighSpeed, TcpHtcp, TcpVegas, TcpScalable, TcpVeno, "
                         "TcpBic, TcpYeah, TcpIllinois, TcpWestwood, TcpWestwoodPlus ",
                 prot);
    cmd.AddValue("error", "Packet error rate", error);
    cmd.Parse(argc, argv);

    // 拥塞算法选择 使用字符串匹配
    if (prot.compare("TcpNewReno") == 0)
    {
        Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpNewReno::GetTypeId()));
    }
    else if (prot.compare("TcpHybla") == 0)
    {
        Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpHybla::GetTypeId()));
    }
    else if (prot.compare("TcpHighSpeed") == 0)
    {
        Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpHighSpeed::GetTypeId()));
    }
    else if (prot.compare("TcpVegas") == 0)
    {
        Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpVegas::GetTypeId()));
    }
    else if (prot.compare("TcpScalable") == 0)
    {
        Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpScalable::GetTypeId()));
    }
    else if (prot.compare("TcpHtcp") == 0)
    {
        Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpHtcp::GetTypeId()));
    }
    else if (prot.compare("TcpVeno") == 0)
    {
        Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpVeno::GetTypeId()));
    }
    else if (prot.compare("TcpBic") == 0)
    {
        Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpBic::GetTypeId()));
    }
    else if (prot.compare("TcpYeah") == 0)
    {
        Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpYeah::GetTypeId()));
    }
    else if (prot.compare("TcpIllinois") == 0)
    {
        Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpIllinois::GetTypeId()));
    }
    else if (prot.compare("TcpWestwood") == 0)
    {
        // 此处是默认拥塞算法
        Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpWestwood::GetTypeId()));
        Config::SetDefault("ns3::TcpWestwood::FilterType", EnumValue(TcpWestwood::TUSTIN));
    }
    else if (prot.compare("TcpWestwoodPlus") == 0)
    {
        Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpWestwood::GetTypeId()));
        Config::SetDefault("ns3::TcpWestwood::ProtocolType", EnumValue(TcpWestwood::WESTWOODPLUS));
        Config::SetDefault("ns3::TcpWestwood::FilterType", EnumValue(TcpWestwood::TUSTIN));
    }
    else
    {
        NS_LOG_DEBUG("Invalid TCP version");
        exit(1);
    }

    // 文件名拼接
    std::string a_s = "bytes_" + prot + ".dat";
    std::string b_s = "drop_" + prot + ".dat";
    std::string c_s = "cw_" + prot + ".dat";

    // 创建文件流保存数据信息
    Ptr<OutputStreamWrapper> total_bytes_data = ascii.CreateFileStream(a_s);
    Ptr<OutputStreamWrapper> dropped_packets_data = ascii.CreateFileStream(b_s);
    Ptr<OutputStreamWrapper> cw_data = ascii.CreateFileStream(c_s);

    // 创建节点
    NS_LOG_INFO("Create nodes.");
    NodeContainer nodes;
    nodes.Create(2);

    NS_LOG_INFO("Create channels.");

    // 使用点对点链路
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("1Mbps"));//设置传输数据率
    pointToPoint.SetChannelAttribute("Delay", StringValue("10ms"));//设置传输延迟
    pointToPoint.SetQueue("ns3::DropTailQueue");//设置队尾丢弃

    NetDeviceContainer devices;
    devices = pointToPoint.Install(nodes);

    // 创建误差模型
    Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
    em->SetAttribute("ErrorRate", DoubleValue(error));
    devices.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(em));

    InternetStackHelper internet;
    internet.Install(nodes);

    // 设置IP地址
    NS_LOG_INFO("Assign IP Addresses.");
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ipv4Container = ipv4.Assign(devices);

    NS_LOG_INFO("Create Applications.");

    uint16_t port = 12344;
    BulkSendHelper source("ns3::TcpSocketFactory", InetSocketAddress(ipv4Container.GetAddress(1), port));
    // 设置传输的字节数
    source.SetAttribute("MaxBytes", UintegerValue(maxBytes));
    ApplicationContainer sourceApps = source.Install(nodes.Get(0));
    sourceApps.Start(Seconds(0.0));
    sourceApps.Stop(Seconds(1.80));

    // 在节点1上设置TCP
    PacketSinkHelper sink("ns3::TcpSocketFactory",
                          InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApps = sink.Install(nodes.Get(1));

    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(1.80));

    tcpSink = DynamicCast<PacketSink>(sinkApps.Get(0));

    uint16_t cbrPort = 12345;

    double startTimes[5] = {0.2, 0.4, 0.6, 0.8, 1.0};
    double endTimes[5] = {1.8, 1.8, 1.2, 1.4, 1.6};

    for (int i = 0; i < 5; i++)
    {
        // 对每一个节点设置应用层协议
        ApplicationContainer cbrApps;
        ApplicationContainer cbrSinkApps;
        OnOffHelper onOffHelper("ns3::UdpSocketFactory", InetSocketAddress(ipv4Container.GetAddress(1), cbrPort + i));
        onOffHelper.SetAttribute("PacketSize", UintegerValue(1024));
        onOffHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        onOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

        onOffHelper.SetAttribute("DataRate", StringValue("300Kbps"));
        onOffHelper.SetAttribute("StartTime", TimeValue(Seconds(startTimes[i])));
        onOffHelper.SetAttribute("StopTime", TimeValue(Seconds(endTimes[i])));
        cbrApps.Add(onOffHelper.Install(nodes.Get(0)));

        PacketSinkHelper sink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), cbrPort + i));
        cbrSinkApps = sink.Install(nodes.Get(1));
        cbrSinkApps.Start(Seconds(0.0));
        cbrSinkApps.Stop(Seconds(1.8));
        cbrSinks[i] = DynamicCast<PacketSink>(cbrSinkApps.Get(0));
    }

    devices.Get(1)->TraceConnectWithoutContext("PhyRxDrop", MakeBoundCallback(&RxDrop, dropped_packets_data));

    if (tracing)
    {
        AsciiTraceHelper ascii;
        pointToPoint.EnableAsciiAll(ascii.CreateFileStream("tcp-comparision.tr"));
        pointToPoint.EnablePcapAll("tcp-comparision", true);
    }

    NS_LOG_INFO("Run Simulation.");

    Simulator::Schedule(Seconds(0.00001), &TotalRx, total_bytes_data);
    Simulator::Schedule(Seconds(0.00001), &TraceCwnd, cw_data);

    // 流量监控
    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();

    Simulator::Stop(Seconds(1.80));
    Simulator::Run();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats();
    std::cout << std::endl
              << "*** Flow monitor statistics ***" << std::endl;
    std::cout << "  Tx Packets:   " << stats[1].txPackets << std::endl;
    std::cout << "  Tx Bytes:   " << stats[1].txBytes << std::endl;
    std::cout << "  Offered Load: " << stats[1].txBytes * 8.0 / (stats[1].timeLastTxPacket.GetSeconds() - stats[1].timeFirstTxPacket.GetSeconds()) / 1000000 << " Mbps" << std::endl;
    std::cout << "  Rx Packets:   " << stats[1].rxPackets << std::endl;
    std::cout << "  Rx Bytes:   " << stats[1].rxBytes << std::endl;
    std::cout << "  Throughput: " << stats[1].rxBytes * 8.0 / (stats[1].timeLastRxPacket.GetSeconds() - stats[1].timeFirstRxPacket.GetSeconds()) / 1000000 << " Mbps" << std::endl;
    std::cout << "  Mean delay:   " << stats[1].delaySum.GetSeconds() / stats[1].rxPackets << std::endl;
    std::cout << "  Mean jitter:   " << stats[1].jitterSum.GetSeconds() / (stats[1].rxPackets - 1) << std::endl;
    flowMonitor->SerializeToXmlFile("data.flowmon", true, true);

    Simulator::Destroy();
    NS_LOG_INFO("Done.");
}
