/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ltmodule.h"
#include "ns3/udp-client.h"

using namespace ns3;

#ifndef PACK_INF
#define Pks_Payload_Len 1000
#define Pks_Header_Len 10
#define Pks_Len 1010
#define Data_Pks_Header  0x47 //indicate the data packets
#endif

#define Receive_Enough 0x77 // indicate the receiver has got enough data

NS_LOG_COMPONENT_DEFINE("LT code application in UDP file transmission");

// ===========================================================================
//
//         node 0                 node 1
//   +----------------+    +----------------+
//   |    ns-3 UDP   |    |    ns-3 UDP    |
//   +----------------+    +----------------+
//   |    10.1.1.1    |    |    10.1.1.2    |
//   +----------------+    +----------------+
//   | point-to-point |    | point-to-point |
//   +----------------+    +----------------+
//           |                     |
//           +---------------------+
//                5 Mbps, 2 ms}

//We want to simulate the UDP file transfer while using LT code in the application
// layer. In this program, there are two kinds of applications. One is MyAPPC, which
// is the client application. The other is the MyAPPS, which is the server application.
// The client will send a video file to the server, using a packet loss channel.
// The receiver drop the packet according to some probability assigned in the main function. Note in a real
// networks, in most cases, it is the server who send  large files to the users. But, for the simulations
// and performance evaluation, it is OK to view the server and client as peers.
//
// The program uses the LT_module which includes the encoding module and decoding module.
//
// The transmission time is recorded and we run the program under different Error Rate to see the changes
// in the transmission time.
//
// The packet segment structure in the application layer is
//   -----2 bytes----|-----4 bytes------|------4 bytes------|-----1000 bytes-------
//   ---Identifier---|---file length----|---random seed-----|-------data-------
// the total length for a packet is 1010 bytes
// the sender will send one packet in one UDP send
// ===========================================================================
//

// this is the receiver buffer size
#define Buffer_Size 10000000 //10MB


//Client Application****************************************************************************************************************Client Application*/
class MyAPPC: public Application {
public:

	MyAPPC();
	virtual ~MyAPPC();

	void Setup(Ptr<Socket> socket, Address address, uint32_t packetSize,
			DataRate dataRate);

private:
	virtual void StartApplication(void);
	virtual void StopApplication(void);

	void ReceivePacket(Ptr<Socket>);

	void ScheduleTx(void);
	void SendPacket(void);

	Ptr<Socket> m_socket;
	Address m_peer;
	uint32_t m_packetSize;
	uint32_t m_nPackets;
	DataRate m_dataRate;
	EventId m_sendEvent;
	bool m_running;
	uint32_t m_packetsSent;

	char * m_sendBuffer;
	long m_sendPos;
	long m_sendSize;
};

MyAPPC::MyAPPC() :
		m_socket(0), m_peer(), m_packetSize(0), m_nPackets(0), m_dataRate(0), m_sendEvent(), m_running(
				false), m_packetsSent(0) {
}

MyAPPC::~MyAPPC() {
	m_socket = 0;
}

void MyAPPC::Setup(Ptr<Socket> socket, Address address, uint32_t packetSize,
		 DataRate dataRate) {
	m_socket = socket;
	m_peer = address;
	m_packetSize = packetSize;
	m_dataRate = dataRate;
	m_sendPos = 0;
}

void MyAPPC::StartApplication(void) {
	// send a file of size about 5MB to the receiver
	// using LT code to generate encoded packets. Here we will generate 5 times original data to send, but the receiver may use only a little more than the
	m_running = true;
	m_packetsSent = 0;
	m_socket->Bind();
	m_socket->Connect(m_peer);

	std::ifstream fsend("Freshman2Davis240p.3gp", std::ifstream::binary);
	fsend.seekg(0, std::ifstream::end);
	uint32_t flen = fsend.tellg();
	char * filereadbuff = new char[flen];


	fsend.seekg(0, std::ifstream::beg);
	fsend.read(filereadbuff, flen);
	fsend.close();
	m_sendSize = 5 * flen;// this is the maximal data that will be sent
	m_sendBuffer = new char[m_sendSize];
	memset(m_sendBuffer, 0, m_sendSize);

	//void LT_Encode(char* buf_send,char* buf,DWORD dwFileSize,DWORD LTSendSize);
	LT_Encode(m_sendBuffer, filereadbuff, flen, m_sendSize);


	m_sendPos = 0;

	m_socket->SetRecvCallback(MakeCallback(&MyAPPC::ReceivePacket, this));//receive data from receiver

	std::cout<<"Sender starts sending at: "<< Simulator::Now ().GetSeconds()<<std::endl;

	SendPacket();//start to send data
}

void MyAPPC::StopApplication(void) {
	m_running = false;

	if (m_sendEvent.IsRunning()) {
		Simulator::Cancel(m_sendEvent);
	}

	if (m_socket) {
		m_socket->Close();
	}
}

void MyAPPC::SendPacket(void) {
	SeqTsHeader seqTs;
	seqTs.SetSeq(m_packetsSent);
	Ptr<Packet> packet = Create<Packet>(
			(const uint8_t *) m_sendBuffer + m_sendPos, m_packetSize);
	packet->AddHeader(seqTs);
	m_socket->Send(packet);

	if (m_sendPos < m_sendSize) {

		ScheduleTx();
		m_sendPos += m_packetSize;
	}
}

void MyAPPC::ScheduleTx(void) {
	if (m_running) {
		Time tNext(
				Seconds(
						m_packetSize * 8
								/ static_cast<double>(m_dataRate.GetBitRate())));
		m_sendEvent = Simulator::Schedule(tNext, &MyAPPC::SendPacket, this);
	}
}

void MyAPPC::ReceivePacket(Ptr<Socket> socket) {
	NS_LOG_FUNCTION (this << socket);
	Ptr<Packet> packet;
	Address from;

	while ((packet = socket->RecvFrom(from)))
	{

		if (packet->GetSize() > 0) {
				SeqTsHeader seqTs;
				packet->RemoveHeader(seqTs);
				uint16_t* header = new uint16_t;
				packet->CopyData((uint8_t*) header,2);
				if ( (* header) == Receive_Enough)//if the receiver has got sufficient data, the sender should stop sending
				{
					m_running = false;
					std::cout<<"Sender stops sending at: "<< Simulator::Now ().GetSeconds()<<std::endl;
				}
		}

	}
}
//Sever Application****************************************************************************************************************Server Application*/
class MyAPPS: public Application {
public:
	static TypeId GetTypeId (void);
	MyAPPS();
	virtual ~MyAPPS();
	void Setup(Address adrr, uint16_t port, uint32_t packetSize, DataRate dataRate);

	/**
	 * returns the number of lost packets
	 * \return the number of lost packets
	 */
	uint32_t GetLost(void) const;

	/**
	 * \brief returns the number of received packets
	 * \return the number of received packets
	 */
	uint32_t GetReceived(void) const;

	/**
	 * \return the size of the window used for checking loss.
	 */
	uint16_t GetPacketWindowSize() const;

	/**
	 * \brief Set the size of the window used for checking loss. This value should
	 *  be a multiple of 8
	 * \param size the size of the window used for checking loss. This value should
	 *  be a multiple of 8
	 */
	void SetPacketWindowSize(uint16_t size);


protected:
	virtual void DoDispose(void);

private:
	virtual void StartApplication(void);
	virtual void StopApplicatioin(void);
	void SendPacket(Address);
	void ReceivePacket(Ptr<Socket>);

	Ptr<Socket> m_socket;
	Ptr<Socket> m_socket6;
	uint16_t m_port;
	Address m_localadrr;
	uint32_t m_packetSize;
	uint32_t m_packetsSent;
	uint32_t m_nPackets;
	DataRate m_dataRate;
	EventId m_receiveEvent;

	bool m_OK;

	PacketLossCounter m_lossCounter;

	bool m_running;
	uint32_t m_packetsReceive;
	char * m_sendBuffer;
	long m_sendPos;
	long m_sendSize;
	char * m_receiveBuffer;//we will initialize the size to be 10MB
	long m_receivePos;
	long m_receiveSize;
	long m_decodethreshold;// when the received data is above this value, start to try decoding

};

	TypeId MyAPPS::GetTypeId(void) {
static TypeId tid =
			TypeId("ns3::MyAPPS").SetParent<Application>().AddConstructor<
					MyAPPS>().AddAttribute("Port",
					"Port on which we listen for incoming packets.",
					UintegerValue(100),
					MakeUintegerAccessor(&MyAPPS::m_port),
					MakeUintegerChecker<uint16_t>()).AddAttribute(
					"PacketWindowSize",
					"The size of the window used to compute the packet loss. This value should be a multiple of 8.",
					UintegerValue(32),
					MakeUintegerAccessor(&MyAPPS::GetPacketWindowSize,
							&MyAPPS::SetPacketWindowSize),
					MakeUintegerChecker<uint16_t>(8, 256));
	return tid;
}

MyAPPS::MyAPPS() :
		m_socket(0), m_packetSize(0), m_nPackets(0), m_dataRate(0), m_lossCounter(
				0), m_running(false), m_packetsReceive(0) {
}

MyAPPS::~MyAPPS() {
	m_socket = 0;
}

void MyAPPS::Setup(Address adrr, uint16_t port, uint32_t packetSize, DataRate dataRate) {
	m_localadrr = adrr;
	m_port = port;
	m_packetSize = packetSize;
	m_dataRate = dataRate;
	m_receiveBuffer = new char[Buffer_Size];
	m_OK = false;
	m_receivePos = 0;
}

uint16_t MyAPPS::GetPacketWindowSize() const {
	NS_LOG_FUNCTION (this);
	return m_lossCounter.GetBitMapSize();
}

void MyAPPS::SetPacketWindowSize(uint16_t size) {
	NS_LOG_FUNCTION (this << size);
	m_lossCounter.SetBitMapSize(size);
}

uint32_t MyAPPS::GetLost(void) const {
	NS_LOG_FUNCTION (this);
	return m_lossCounter.GetLost();
}

uint32_t MyAPPS::GetReceived(void) const {
	NS_LOG_FUNCTION (this);
	return m_packetsReceive;
}

void MyAPPS::DoDispose(void) {
	NS_LOG_FUNCTION (this);
	Application::DoDispose();
}



void MyAPPS::StartApplication() {
	NS_LOG_FUNCTION (this);

	if (m_socket == 0) {
		TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
		m_socket = Socket::CreateSocket(GetNode(), tid);
		m_socket->Bind(m_localadrr);
	}
	m_socket->SetRecvCallback(MakeCallback(&MyAPPS::ReceivePacket, this));//receive data call back


}

void MyAPPS::StopApplicatioin() {
	NS_LOG_FUNCTION (this);

	if (m_socket != 0) {
		m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket> >());
	}


}

void MyAPPS::ReceivePacket(Ptr<Socket> socket) {
	NS_LOG_FUNCTION (this << socket);
	Ptr<Packet> packet;
	Address from;

	while ((packet = socket->RecvFrom(from))) {
		if (packet->GetSize() > 0) {
			SeqTsHeader seqTs;

			packet->RemoveHeader(seqTs);
			uint8_t* header = new uint8_t [6];
			uint32_t filelen = 0;
			uint16_t type = 0;
			packet->CopyData((uint8_t*) header,6);
			type = * (uint16_t*)header;//identifier
			filelen = * (uint32_t*)(header + 2);//get the file length
			m_decodethreshold = filelen*1.02;
			if (type == Data_Pks_Header && !m_OK)// check if it is data packet and if we have decoded the original file
			{
				packet->CopyData((uint8_t*) m_receiveBuffer + m_receivePos,
						m_packetSize);

     			m_receivePos += m_packetSize;
				//std::cout<<"Receive:  "<< m_packetsReceive << " at: " << Simulator::Now ().GetSeconds() <<std::endl;
				if (m_receivePos > m_decodethreshold && m_packetsReceive % 100 == 0 )
				{
					// when the received data is more than the decoding threshold, try decoding every 100 packets
					char * buf = new char [filelen];
				    if (LT_Decode(buf,m_receiveBuffer,filelen,m_receivePos))
				    {
				    	std::ofstream freceived("Recovered_UDP_Freshman2Davis240p.3gp",std::ofstream::binary);
				    	freceived.write(buf, filelen);
				    	freceived.close();
				    	std::cout<< "Receiver's Decoding succeeds at: "<< Simulator::Now ().GetSeconds()<<std::endl;
				    	m_OK = true;
				    	m_sendBuffer = new char [2];
				    	*(uint16_t *) m_sendBuffer = Receive_Enough;//stop message identifier
				    	m_sendPos = 0;
				    	m_packetSize = 2;
				    	SendPacket(from);//send the stop message to the sender
				    	m_packetsSent ++;
				    }
				    else
				    {
				    	std::cout<<"Receiver's Decoding fails Once!"<<std::endl;
				    }

			}
			}
			m_packetsReceive++;

			/*
 			uint32_t currentSequenceNumber = seqTs.GetSeq();if (InetSocketAddress::IsMatchingType(from)) {
				NS_LOG_UNCOND("TraceDelay: RX " << packet->GetSize () <<
						" bytes from "<< InetSocketAddress::ConvertFrom (from).GetIpv4 () <<
						" Sequence Number: " << currentSequenceNumber <<
						" Uid: " << packet->GetUid () <<
						" TXtime: " << seqTs.GetTs () <<
						" RXtime: " << Simulator::Now () <<
						" Delay: " << Simulator::Now () - seqTs.GetTs ());

				NS_LOG_INFO ("TraceDelay: RX " << packet->GetSize () <<
						" bytes from "<< InetSocketAddress::ConvertFrom (from).GetIpv4 () <<
						" Sequence Number: " << currentSequenceNumber <<
						" Uid: " << packet->GetUid () <<
						" TXtime: " << seqTs.GetTs () <<
						" RXtime: " << Simulator::Now () <<
						" Delay: " << Simulator::Now () - seqTs.GetTs ());
			} else if (Inet6SocketAddress::IsMatchingType(from)) {
				NS_LOG_INFO ("TraceDelay: RX " << packet->GetSize () <<
						" bytes from "<< Inet6SocketAddress::ConvertFrom (from).GetIpv6 () <<
						" Sequence Number: " << currentSequenceNumber <<
						" Uid: " << packet->GetUid () <<
						" TXtime: " << seqTs.GetTs () <<
						" RXtime: " << Simulator::Now () <<
						" Delay: " << Simulator::Now () - seqTs.GetTs ());
			}*/

			//m_lossCounter.NotifyReceived(currentSequenceNumber);

		}
	}
}
void MyAPPS::SendPacket(Address dest) {
	SeqTsHeader seqTs;
	seqTs.SetSeq(m_packetsSent);
	Ptr<Packet> packet = Create<Packet>(
			(const uint8_t *) m_sendBuffer + m_sendPos, m_packetSize);
	packet->AddHeader(seqTs);
	m_socket->SendTo(packet,0,dest);
}
/*
static void CwndChange(uint32_t oldCwnd, uint32_t newCwnd) {
	NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << newCwnd);
}

static void ReceivePkt(Ptr<const Packet> p){
	NS_LOG_UNCOND("Receive at "<<Simulator::Now().GetSeconds());
}

static void RxDrop(Ptr<const Packet> p) {
	NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());
}
*/
int main(int argc, char *argv[]) {
	NodeContainer nodes;
	nodes.Create(2);

	PointToPointHelper pointToPoint;
	pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
	pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

	NetDeviceContainer devices;
	devices = pointToPoint.Install(nodes);

	Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
	em->SetAttribute("ErrorRate", DoubleValue(0.001));// the error rate should be changed to see its effects
	devices.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(em));

	InternetStackHelper stack;
	stack.Install(nodes);

	Ipv4AddressHelper address;
	address.SetBase("10.1.1.0", "255.255.255.252");
	Ipv4InterfaceContainer interfaces = address.Assign(devices);

	uint16_t sinkPort = 8080;

	// the sever part
	Address sinkAddress(InetSocketAddress(interfaces.GetAddress(1), sinkPort));
	Ptr<MyAPPS> appS = CreateObject<MyAPPS>();
	appS->Setup(sinkAddress, sinkPort, Pks_Len, DataRate("5Mbps"));
	nodes.Get(1)->AddApplication(appS);
	appS->SetStartTime(Seconds(0.));
	appS->SetStopTime(Seconds(100.));


	// the client part
	Ptr<Socket> ns3UdpSocket = Socket::CreateSocket(nodes.Get(0),UdpSocketFactory::GetTypeId());
//	ns3UdpSocket->TraceConnectWithoutContext("CongestionWindow",MakeCallback(&CwndChange));
	Ptr<MyAPPC> app = CreateObject<MyAPPC>();
	app->Setup(ns3UdpSocket, sinkAddress, Pks_Len, DataRate("5Mbps"));
	nodes.Get(0)->AddApplication(app);
	app->SetStartTime(Seconds(1.));
	app->SetStopTime(Seconds(100.));


	//devices.Get(1)->TraceConnectWithoutContext("PhyRxDrop",MakeCallback(&RxDrop));

//	devices.Get(1)->TraceConnectWithoutContext("ReceivePkt", MakeCallback(&ReceivePkt));

	Simulator::Stop(Seconds(100));
	Simulator::Run();
	Simulator::Destroy();
	std::cout<<"Finished"<<std::endl;
	return 0;
}

