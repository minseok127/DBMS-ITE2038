Buffer Manager   
==============
디스크 I/O를 최소화 하기위해 Index 계층과 Disk 계층 사이에 위치하게 되는 계층입니다.   
     
해당 계층의 역할은 다음과 같습니다.   
1. 메모리 상에 페이지를 저장하여 해당 페이지에 대한 read가 생길 시 디스크가 아니라 메모리에서 불러오는 역할을 해주는 caching   
2. Index 계층에서 특정 페이지에 대한 수정을 할 때 디스크에 바로 입력하지 않고 버퍼를 거침으로써 Index 계층은 다음 행동을 빠르게 수행할 수 있게 해줌
   
본 프로젝트에서는 c++언어를 사용하여 구현하였고 리눅스 환경에서 g++ 7.5.0로 컴파일되었습니다.
  
Features
========
1. Buffer Manager API
2. File Manager API modification
3. Index Manager Command modification