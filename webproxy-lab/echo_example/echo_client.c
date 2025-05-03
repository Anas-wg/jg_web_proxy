#include "csapp.h" // CSAPP 라이브러리 헤더 포함 (표준 헤더 및 래퍼 함수 포함)

int main(int argc, char **argv)
{
  int clientfd;                    // 소켓 디스크립터
  char *host, *port, buf[MAXLINE]; // 호스트, 포트 문자열, 입출력 버퍼
  rio_t rio;                       // RIO 버퍼 구조체

  // 1. 명령줄 인자 확인 (프로그램 이름, 호스트, 포트 - 총 3개)
  if (argc != 3)
  {
    fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
    exit(0); // 사용법 출력 후 종료
  }
  host = argv[1]; // 두 번째 인자를 호스트 이름으로 사용
  port = argv[2]; // 세 번째 인자를 포트 번호 문자열로 사용

  // 2. 서버에 연결 시도 (CSAPP 헬퍼 함수 사용)
  // Open_clientfd는 내부적으로 getaddrinfo, socket, connect를 처리함
  clientfd = Open_clientfd(host, port);

  // 3. RIO (Robust I/O) 읽기 버퍼 초기화
  // clientfd 소켓에서 데이터를 읽기 위한 rio 구조체 설정
  Rio_readinitb(&rio, clientfd); // rio 초기화

  // 4. 메인 루프: 표준 입력 -> 서버 전송 -> 서버 응답 -> 표준 출력
  printf("Connected to echo server on %s:%s\n", host, port); // 연결 메시지 (선택)
  printf("Type input: ");                                    // 프롬프트 (선택)
  while (Fgets(buf, MAXLINE, stdin) != NULL)
  { // 표준 입력에서 한 줄 읽기 (EOF나 에러 시 NULL 반환)
    // 4a. 읽은 라인을 서버로 전송
    Rio_writen(clientfd, buf, strlen(buf)); // buf의 내용을 서버로 보냄

    // 4b. 서버로부터 에코(메아리) 라인 읽기
    if (Rio_readlineb(&rio, buf, MAXLINE) == 0)
    { // 서버가 연결을 닫으면 0 반환
      printf("Server closed connection\n");
      break; // 루프 탈출
    }

    // 4c. 받은 에코 라인을 표준 출력으로 인쇄
    Fputs(buf, stdout);
    printf("Type input: "); // 다음 입력을 위한 프롬프트 (선택)
  }

  // 5. 루프 종료 후 연결 닫기
  Close(clientfd);                // 소켓 디스크립터 닫기
  printf("Connection closed.\n"); // 종료 메시지 (선택)
  exit(0);                        // 정상 종료
}