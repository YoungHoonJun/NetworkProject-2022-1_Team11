# NetworkProject-2022-1_Team11
This project is an Application using NS-3


## Installation
### Prerequisites
ns-3 (version 3.29),
## Subject
This project's main purpose is to improve the performance of video streaming. We implemented three functions as below for the performamce improvement.


### RTP
#### Routing Algorithm
1. Dijkstra Algorithm
* 링크 상태 정보를 모든 라우터에 전달하여 최단 경로 트리를 구성하는 라우팅 프로토콜 알고리즘 - 다익스트라(Dijkstra) 알고리즘 사용하여 구현

* Input(Topology 정보):
첫 줄에 node의 개수(nodenum)와 간선의 개수(bridgenum)를 의미하는 두 개의 정수가 주어진다.
그 다음 간선의 개수(bridgenum)만큼 간선에 대한 정보가 한 줄에 하나씩 주어진다.
각 줄은 시작점, 끝점, 간선의 cost를 의미하는 세 개의 정수가 주어진다.

* 다익스트라 알고리즘
다이나믹 프로그래밍을 활용한 대표적 최단 경로 탐색 알고리즘.
모든 간선의 cost가 양수여야 하는 제한이 있지만 본 프로그램에서는 문제가 되지 않음.
출발하는 노드(0, server)로부터 목적지 노드(n-1, client)에 이르기까지의 최단 경로를 탐색하며, 어떤 노드를 지나야 하는지를 기록해 둠

* Ouput(최단 거리 및 경로)
출발지로부터 목적지까지의 최단 거리와 경로를 출력

2. Connection (Wi-Fi)
-----

3. Connection (P2P)
txt 파일에 있는 값들을 받아
wifi와 거의 동일한 전처리 과정을 거칩니다.
대부분의 Helper 변수(PointToPointHelper, InternetStackHelper, Ipv4AddressHelper, VideoStreamServerHelper)는
하나씩만 선언하고 대부분의 Container 변수(NodeContainer, NetDeviceContainer, Ipv4InterfaceContainer, ApplicationContainer)는
노드의 개수( input.txt 2번째 숫자 +2 : 클라 1개 서버 1개를 가정하고 만들어서 서버 n개의 경우 코드 수정 필요)만큼 선언합니다.
선언된 Helper 및 Container다익스트라 알고리즘을 통해 사전에 구한 경로대로 설치합니다.
이때 111~118번째에서 설치할 node의 index를 n이라 하면 주소값을 "10.1.(n+1).0" 형태로 만들어줍니다.
wifi 코드와 비슷한 방식으로 클라이언트를 제외한 나머지 노드에 서버를, 서버를 제외한 나머지 노드에 클라이언트를 깔아
통신합니다.

#### simple_rtp rule
0. RTP header를 통해 sequence 번호 같이 전달. (seq 0은 non-request signal)
1. Client에서 빠진 packet Sequence 검출해 해당 sequence 서버에 요청
2. Server는 요청받은 sequence 포함 이전의 모든 패킷을 queue에서 꺼내고, 요청받은 패킷 전송.

#### Realiable Streaming
0. Framefile 에 사진들의 이름이 적혀있음.
1. 25(frameRate)개의 사진(frame)이 모여 1초의 동영상을 전달하는 것으로 취급됨.
2. 1 frame은 여러개의 Sequence로 이루어져 있음. 서버의 seq를 보낼 때 frame이 몇 sequence로 이루어졌는지도 보냄. (frameLastSeq)
3. 정기적으로 client 쪽에서 missing Seq을 서버로 보냄 (있다면)
4. frameLastSeq를 보고 client 쪽에서는 buffer로 저장했다가, frameRate만큼 모이면 25장을 output 파일로 저장.
