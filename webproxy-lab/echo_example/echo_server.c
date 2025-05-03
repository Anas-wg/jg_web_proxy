#include "csapp.h" // CSAPP 라이브러리 헤더 포함

void echo(int connfd); // 클라이언트 통신 처리 함수의 원형 선언 (구현은 다른 곳에)

int main(int argc, char **argv)
{
  int listenfd, connfd; // 리스닝 소켓, 연결 소켓 디스크립터
  socklen_t clientlen;  // 클라이언트 주소 구조체 크기
  // sockaddr_storage: IPv4/IPv6 주소를 모두 담을 수 있는 충분한 크기의 구조체 (프로토콜 독립성)
  struct sockaddr_storage clientaddr;
  char client_hostname[MAXLINE], client_port[MAXLINE]; // 클라이언트 호스트명, 포트 저장 버퍼

  // 1. 명령줄 인자 확인 (프로그램 이름, 포트 - 총 2개)
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(0);
  }

  // 2. 리스닝 소켓 생성 및 준비 (CSAPP 헬퍼 함수 사용)
  // Open_listenfd는 내부적으로 getaddrinfo(AI_PASSIVE), socket, bind, listen 처리
  listenfd = Open_listenfd(argv[1]); // argv[1] 포트로 리스닝 소켓 열기

  // 3. 무한 루프: 클라이언트 연결 요청 대기 및 처리
  while (1)
  {
    // 3a. 클라이언트 연결 수락 대기
    clientlen = sizeof(struct sockaddr_storage); // clientaddr 크기 초기화
    // Accept 함수는 listenfd로 연결 요청이 올 때까지 기다림(block).
    // 연결되면, 새로운 연결 소켓 connfd를 반환하고, clientaddr에 클라이언트 주소 정보를 채움.
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

    // 3b. 연결된 클라이언트 정보 얻기 및 출력
    // clientaddr에 저장된 바이너리 주소를 호스트 이름과 포트 문자열로 변환
    Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE,
                client_port, MAXLINE, 0); // flags=0 이므로 호스트 이름 찾기 시도
    printf("Connected to (%s, %s)\n", client_hostname, client_port);

    // 3c. 클라이언트 요청 처리 함수 호출
    // 실제 에코 로직은 echo 함수에서 담당. connfd를 넘겨줌.
    echo(connfd);

    // 3d. 클라이언트와의 연결 소켓 닫기
    // echo 함수가 반환되면 (클라이언트와의 통신이 끝나면) 해당 연결 소켓을 닫음.
    Close(connfd);
    printf("Closed connection to (%s, %s)\n", client_hostname, client_port); // 연결 종료 메시지 (선택)

    // -> 루프의 처음으로 돌아가 다음 클라이언트 연결을 Accept에서 기다림
  }
  exit(0); // 실제로는 이 라인에 도달하지 않음 (무한 루프)
}