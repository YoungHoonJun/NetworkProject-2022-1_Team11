# NetworkProject-2022-1_Team11
This project is an Application using NS-3, refering many points to [this](https://github.com/guoxiliu/VideoStream-NS3).


### Prerequisites
ns-3 (version 3.29), Python (`python 2.7+` or `python 3.5+`), C++ compiler (`clang++` or `g++`)

### How to install
1. Download and build `ns-3` following the official document [here](https://www.nsnam.org/docs/release/3.29/tutorial/singlehtml/index.html#getting-started).
2. Copy the files **exactly** into the folders of the `ns-3`. (Be aware of the `wscript` in `src->applications`, otherwise the video streaming application will not be installed!)
3. Run `./waf` or `./waf build` to build the new application.
4. Run `./waf --run videoStreamer` for the testing program (you can change `CASE` in `videoStreamTest.cc` for different network environments).

### Subject
This project's main purpose is by sending a lot of pictures (or -videos) with differnet quality depending on buffering.  
Also we considered routing algorithm like Dijkstra.


### Model

#### RTP Rule

0. our RTP Header
![image](https://user-images.githubusercontent.com/49546550/171166302-93026288-0413-4b89-b039-c756de324d45.png)
1. 서버에서 패킷을 전송할 때, RTP header를 추가해 sequence number와 해당 frame의 마지막 sequence number를 같이 전달합니다. (sequence = 0은 retransmit이 필요 없음을 의미.)
2. 서버는 보낸 패킷들을 queue에 넣어 저장합니다.
3. 클라이언트는 받은 패킷을 sequence 순서대로 읽어 빠진 sequence를 검출하여 해당 sequence를 서버에 요청합니다.
4. 서버는 클라이언트로부터 요청받은 sequence 이전의 모든 패킷을 queue에서 꺼내고, 요청받은 패킷을 전송합니다.

#### Realiable Streaming
0. Framefile에는 사진들의 이름이 적혀있습니다.
1. 25(frameRate)개의 사진(frame)이 모여 1초의 동영상을 전달하는 것으로 간주됩니다.
2. 1 frame은 여러 개의 sequence로 이루어져 있습니다. 서버가 sequence를 보낼 때 frame이 몇 sequence로 이루어졌는지도 RTP header를 통해 같이 전송합니다. (frameLastSeq)
3. 클라이언트가 받은 패킷 중 만약 miss가 존재한다면, 정기적으로 클라이언트 쪽에서 missing sequence를 서버로 보냅니다.
4. frameLastSeq를 보고 클라이언트 쪽에서는 buffer로 저장했다가, frameRate만큼 모이면 25장을 output 파일로 저장합니다.

### Testing Scenario

#### Scenario with Routing Algorithm
1. Dijkstra Algorithm

![image](https://user-images.githubusercontent.com/34998542/171118417-9b3610f5-0543-41bd-9053-06dead9ef5e7.png)

* 링크 상태 정보를 모든 라우터에 전달하여 최단 경로 트리를 구성하는 라우팅 프로토콜 알고리즘 - 다익스트라(Dijkstra) 알고리즘을 사용하여 구현하였습니다.

* Input(Topology 정보):
첫 줄에 node의 개수(nodenum)와 간선의 개수(bridgenum)를 의미하는 두 개의 정수가 주어집니다.
그 다음 간선의 개수(bridgenum)만큼 간선에 대한 정보가 한 줄에 하나씩 주어집니다.
각 줄에는 시작점, 끝점, 간선의 cost를 의미하는 세 개의 정수가 주어집니다.

* 다익스트라 알고리즘
다이나믹 프로그래밍을 활용한 대표적 최단 경로 탐색 알고리즘입니다.
모든 간선의 cost가 양수여야 하는 제한이 있지만 본 프로그램에서는 문제가 되지 않습니다.
출발하는 노드(0, server)로부터 목적지 노드(n-1, client)에 이르기까지의 최단 경로를 탐색하며, 어떤 노드를 지나야 하는지를 기록합니다.

* Ouput(최단 거리 및 경로)
출발지로부터 목적지까지의 최단 거리와 경로를 2차원 vector 형식으로 저장합니다.

2. Connection (Wi-Fi)

다익스트라 알고리즘을 통해 저장된 경로를 이용합니다.
Wi-Fi 설정에 필요한 Station Node와 AP Node를 만들고 Helper를 통해 설정하고 연결해주는 과정을 거칩니다.
(YansWifiChannelHelper, YansWifiPhyHelper, WifiHelper, WifiMacHelper, MobilityHelper, InternetStackHelper, Ipv4AddressHelper
  , VideoStreamServerHelper, VideoStreamClientHelper, Ipv4GlobalRoutingHelper)를 이용합니다.
Routing을 구현하기 위해 router의 역할을 하는 node에 client와 server를 모두 설치하여 send와 receive의 기능이 모두 작동하도록 합니다.

3. Connection (P2P)

input.txt 파일에 있는 값들을 받아 wifi와 거의 동일한 전처리 과정을 거칩니다.
대부분의 Helper 변수(PointToPointHelper, InternetStackHelper, Ipv4AddressHelper, VideoStreamServerHelper)는
하나씩만 선언하고 대부분의 Container 변수(NodeContainer, NetDeviceContainer, Ipv4InterfaceContainer, ApplicationContainer)는
노드의 개수( input.txt 2번째 숫자 +2 : 클라 1개 서버 1개를 가정하고 만들어서 서버 n개의 경우 코드 수정 필요)만큼 선언합니다.
선언된 Helper 및 Container다익스트라 알고리즘을 통해 사전에 구한 경로대로 설치합니다.
이 때, 설치할 node의 index를 n이라 하면 주소값을 "10.1.(n+1).0" 형태로 만들어줍니다.
wifi 코드와 유사한 방식으로 클라이언트를 제외한 나머지 노드에 서버를, 서버를 제외한 나머지 노드에 클라이언트를 깔아 통신합니다.

다음 사진은 이번 project에서 구성한 Network Topology를 간단하게 나타낸 것입니다.

![image](https://user-images.githubusercontent.com/30406090/171147934-be619c49-bf42-46d2-b74e-83b3a93210eb.JPG)

---

## References

1. The ns-3 development team, “ns-3 network simulator”, https://www.nsnam.org/docs/release/3.29/tutorial/html 
2. Guoxi and L.Kong, "VideoStream-NS3", https://github.com/guoxiliu/VideoStream-NS3
3. Wikipedia, "Real-time Transport Protocol", https://en.wikipedia.org/wiki/Real-time_Transport_Protocol Accessed: 2022-05-17.


## License

본 프로젝트는 `GPLv2` 라이선스 하에 공개되어 있습니다. 모델 및 코드를 사용할 경우 라이선스 내용을 준수해주세요. 라이선스 전문은 [LICENSE](https://github.com/nsnam/ns-3-dev-git/blob/master/LICENSE)에서 확인하실 수 있습니다.