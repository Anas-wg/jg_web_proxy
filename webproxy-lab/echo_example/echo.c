#include "csapp.h" // CSAPP 라이브러리 헤더 포함

void echo(int connfd) // 연결된 클라이언트 소켓 디스크립터를 인자로 받음
{
  size_t n;          // 읽은 바이트 수를 저장할 변수
  char buf[MAXLINE]; // 데이터를 읽고 쓸 버퍼
  rio_t rio;         // RIO 읽기 버퍼 구조체

  // 1. RIO 읽기 버퍼 초기화
  // connfd로부터 데이터를 읽기 위해 rio 구조체 초기화
  Rio_readinitb(&rio, connfd);

  // 2. 데이터 읽기 및 쓰기 루프
  // Rio_readlineb가 0을 반환할 때까지 (클라이언트가 연결을 끊을 때까지) 반복
  while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0)
  {
    // 2a. 서버가 받은 바이트 수 출력 (디버깅/로깅용)
    printf("server received %d bytes\n", (int)n);

    // 2b. 읽은 데이터를 클라이언트에게 그대로 다시 전송 (에코)
    // Rio_writen은 n 바이트를 모두 전송하는 것을 보장
    Rio_writen(connfd, buf, n);
  }
  // 3. 루프 종료 (클라이언트가 연결을 닫으면 Rio_readlineb가 0 반환)
  // 함수는 여기서 반환됨. connfd 닫는 것은 main 루프에서 처리.
}