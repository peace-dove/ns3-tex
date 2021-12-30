#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/applications-module.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/uinteger.h"
#include "ns3/point-to-point-dumbbell.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/test.h"

using namespace ns3;

int main (int argc, char *argv[])
{

Time::SetResolution (Time::NS);
std::string queueType = "DropTail";

int nodes = 0; 
int window_size = 4000; 
int maxBytes = 1000000;
int qlen = 800;
double minimum_threshold = 50.0;
double maximum_threshold = 150.0;
double Wq = 1.0/100.0;
double maxP = 1.0/10.0;
double load = 0.75;

//修改命令行参数
CommandLine cmd;
cmd.AddValue ("window_size", "TCP Receiver window size", window_size);
cmd.AddValue ("maxBytes", "Max bytes that can be sent", maxBytes);
cmd.AddValue ("minTh", "Min threshold for prob drops", minimum_threshold);
cmd.AddValue ("maximum_threshold", "Max threshold for problitistic drops", maximum_threshold);
cmd.AddValue ("maxP", "Max probability of doing an early drop", maxP);
cmd.AddValue ("Wq", "Weighting factor for average queue length computation", Wq);
cmd.AddValue ("qlen", "Maximum number of bytes that can be enqueued", qlen);
cmd.AddValue ("load", "Load", load);
cmd.Parse(argc, argv);


//为DropTailQueue, RedQUeue and TCPSocket提供基本配置
Config::SetDefault ("ns3::DropTailQueue::Mode", EnumValue(DropTailQueue::QUEUE_MODE_BYTES));
Config::SetDefault ("ns3::DropTailQueue::MaxBytes", UintegerValue(maxBytes));
Config::SetDefault ("ns3::RedQueue::Mode", EnumValue (RedQueue::QUEUE_MODE_BYTES));
Config::SetDefault ("ns3::RedQueue::MinTh", DoubleValue (minimum_threshold));
Config::SetDefault ("ns3::RedQueue::MaxTh", DoubleValue (maximum_threshold));
Config::SetDefault ("ns3::RedQueue::QW", DoubleValue (Wq));
Config::SetDefault ("ns3::RedQueue::QueueLimit", UintegerValue (qlen));
Config::SetDefault ("ns3::RedQueue::LInterm", DoubleValue (maxP));
Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue(window_size));


std::string type;
type = "ns3::RedQueue"; //可以切换成DropTail

NodeContainer n;
nodes=12;
n.Create(nodes);

//创建网络拓扑图
NodeContainer bottleneck_link_1 = NodeContainer(n.Get(0), n.Get(1));
NodeContainer bottleneck_link_3 = NodeContainer(n.Get(1), n.Get(2));
NodeContainer bottleneck_link_2 = NodeContainer(n.Get(0), n.Get(3));
NodeContainer source_1 = NodeContainer(n.Get(3), n.Get(4));
NodeContainer source_2 = NodeContainer(n.Get(3), n.Get(5));
NodeContainer source_3 = NodeContainer(n.Get(3), n.Get(6));
NodeContainer sink_1 = NodeContainer(n.Get(2), n.Get(7));
NodeContainer sink_2 = NodeContainer(n.Get(2), n.Get(8));
NodeContainer sink_3 = NodeContainer(n.Get(2), n.Get(9));

//定义路由节点
NodeContainer routerNodes;
routerNodes.Add(n.Get(0));
routerNodes.Add(n.Get(1));
routerNodes.Add(n.Get(2));
routerNodes.Add(n.Get(3));


//源节点
NodeContainer leftNodes;
leftNodes.Add(n.Get(4));
leftNodes.Add(n.Get(5));
leftNodes.Add(n.Get(6));

//目的节点
NodeContainer rightNodes;
rightNodes.Add(n.Get(7));
rightNodes.Add(n.Get(8));
rightNodes.Add(n.Get(9));

//瓶颈容量
PointToPointHelper bottleneck1;
bottleneck1.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
bottleneck1.SetChannelAttribute ("Delay", StringValue ("10ms"));
bottleneck1.SetQueue (type);
NetDeviceContainer device_bottleneck_link_2 = bottleneck1.Install(bottleneck_link_2);

PointToPointHelper bottleneck2;
bottleneck2.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
bottleneck2.SetChannelAttribute ("Delay", StringValue ("10ms"));
bottleneck2.SetQueue (type);
NetDeviceContainer device_bottleneck_link_3 = bottleneck2.Install(bottleneck_link_3);

PointToPointHelper center;
center.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
center.SetChannelAttribute ("Delay", StringValue ("10ms"));
NetDeviceContainer device_bottleneck_link_1 = center.Install(bottleneck_link_1);

PointToPointHelper p2pLeft;
p2pLeft.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
p2pLeft.SetChannelAttribute ("Delay", StringValue ("6ms"));
NetDeviceContainer device_source_1 = p2pLeft.Install(source_1);

PointToPointHelper p2pLeft2;
p2pLeft2.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
p2pLeft2.SetChannelAttribute ("Delay", StringValue ("6ms"));
NetDeviceContainer device_source_2 = p2pLeft2.Install(source_2);

PointToPointHelper p2pLeft3;
p2pLeft3.SetDeviceAttribute ("DataRate", StringValue ("16Mbps"));
p2pLeft3.SetChannelAttribute ("Delay", StringValue ("4ms"));
NetDeviceContainer device_source_3 = p2pLeft3.Install(source_3);

PointToPointHelper p2pRight;
p2pRight.SetDeviceAttribute ("DataRate", StringValue ("16Mbps"));
p2pRight.SetChannelAttribute ("Delay", StringValue ("4ms"));
NetDeviceContainer device_sink_1 = p2pRight.Install(sink_1);

PointToPointHelper p2pRight2;
p2pRight2.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
p2pRight2.SetChannelAttribute ("Delay", StringValue ("6ms"));
NetDeviceContainer device_sink_2 = p2pRight2.Install(sink_2);

PointToPointHelper p2pRight3;
p2pRight3.SetDeviceAttribute ("DataRate", StringValue ("16Mbps"));
p2pRight3.SetChannelAttribute ("Delay", StringValue ("4ms"));
NetDeviceContainer device_sink_3 = p2pRight3.Install(sink_3);

NetDeviceContainer array_of_devices[] = {device_bottleneck_link_2, device_bottleneck_link_3, device_bottleneck_link_1, device_source_1, device_source_2, device_source_3, device_sink_1, device_sink_2, device_sink_3};
std::vector<NetDeviceContainer> devices(array_of_devices, array_of_devices + sizeof(array_of_devices) / sizeof(NetDeviceContainer));

//安装软件协议栈
InternetStackHelper stack;
stack.Install (n);

//为每个节点提供IP地址
Ipv4AddressHelper ipv4;
std::vector<Ipv4InterfaceContainer> ifaceLinks(nodes-1);
for(uint32_t i=0; i<devices.size(); ++i)
 {
std::ostringstream sub_net;
sub_net << "192.10." << i+1 << ".0";
ipv4.SetBase(sub_net.str().c_str(), "255.255.255.0");
ifaceLinks[i] = ipv4.Assign(devices[i]);
}

double min_bandwidth = 1000000;

double duty_cycle = 0.5;

uint64_t rate = (uint64_t)(load*min_bandwidth / 3 / duty_cycle); 


int port_number = 12;

//开关应用
OnOffHelper onOffUdp1("ns3::UdpSocketFactory", Address(InetSocketAddress(ifaceLinks[6].GetAddress(1), port_number)));
onOffUdp1.SetConstantRate(DataRate(rate));
onOffUdp1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=0.5]"));
onOffUdp1.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.5]"));


BulkSendHelper source1 ("ns3::TcpSocketFactory", InetSocketAddress(ifaceLinks[7].GetAddress(1), port_number));
source1.SetAttribute("MaxBytes", UintegerValue (maxBytes));

BulkSendHelper source2 ("ns3::TcpSocketFactory", InetSocketAddress(ifaceLinks[8].GetAddress(1), port_number));
source2.SetAttribute("MaxBytes", UintegerValue (maxBytes));

ApplicationContainer sourceApps;
sourceApps.Add(source1.Install(source_2.Get(1)));
sourceApps.Add(source2.Install(source_3.Get(1)));
sourceApps.Add(onOffUdp1.Install(source_1.Get(1)));
sourceApps.Start(Seconds(0.0));
sourceApps.Stop(Seconds(15.0));

//流量消耗
PacketSinkHelper sinkUdp1("ns3::UdpSocketFactory",
Address(InetSocketAddress(Ipv4Address::GetAny(), port_number)));

PacketSinkHelper sinkTcp1("ns3::TcpSocketFactory",
Address(InetSocketAddress(Ipv4Address::GetAny(), port_number)));

PacketSinkHelper sinkTcp2("ns3::TcpSocketFactory",
Address(InetSocketAddress(Ipv4Address::GetAny(), port_number)));

ApplicationContainer sinkApps;
sinkApps.Add(sinkTcp1.Install(sink_2.Get(1)));
sinkApps.Add(sinkTcp2.Install(sink_3.Get(1)));
sinkApps.Add(sinkUdp1.Install(sink_1.Get(1)));
sinkApps.Start(Seconds(0.0));
sinkApps.Stop(Seconds(15.0));

//为了方便，定义全局路由表
Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

Simulator::Stop (Seconds (15.0));
Simulator::Run ();

std::vector<int> goodputs;
int i = 0;
for(ApplicationContainer::Iterator ii = sinkApps.Begin(); ii != sinkApps.End(); ++ii) {
Ptr<PacketSink> sink = DynamicCast<PacketSink> (*ii);
int bytes_received = sink->GetTotalRx ();
goodputs.push_back(bytes_received / 15.0);
if(i==0)
{

std::cout << bytes_received << std::endl;
std::cout <<  goodputs.back() <<  std::endl;
}
if(i==1)
{
std::cout << bytes_received << std::endl;
std::cout << goodputs.back() << std::endl;
}
if(i==2)
{
std::cout <<  bytes_received << std::endl;
std::cout <<  goodputs.back() <<  std::endl;
}
++i;
}

Simulator::Destroy ();

return 0;
}
