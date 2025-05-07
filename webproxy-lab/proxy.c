#include <stdio.h>
#include "csapp.h"

void doit(int clientfd);
void read_requesthdrs(rio_t *rp, void *buf, int serverfd, char *hostname, char *port);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void parse_uri(char *uri, char *hostname, char *port, char *path);

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

static const int is_local_test = 1; // 로컬이 아닌 외부에서 테스트하기 위한 상수 (0 할당 시 도메인과 포트가 고정되어 외부에서 접속 가능)

int main(int argc, char **argv)
{
  int listenfd, clientfd;
  char client_hostname[MAXLINE], client_port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]); // 전달받은 포트 번호를 사용해 수신 소켓 생성
  while (1)
  {
    clientlen = sizeof(clientaddr);
    clientfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // 클라이언트 연결 요청 수신
    Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", client_hostname, client_port);
    doit(clientfd);
    Close(clientfd);
  }
}

void doit(int clientfd)
{
  int serverfd;
  char request_buf[MAXLINE], response_buf[MAXLINE];
  char method[MAXLINE], uri[MAXLINE], path[MAXLINE], hostname[MAXLINE], port[MAXLINE];
  char *srcp, filename[MAXLINE], cgiargs[MAXLINE];
  rio_t request_rio, response_rio;

  /* Request 요청 라인 읽기  Client -> Proxy */
  Rio_readinitb(&request_rio, clientfd);             // 클라이언트의 요청을 읽기 위해 rio와 fd 연결
  Rio_readlineb(&request_rio, request_buf, MAXLINE); // 요청 라인 읽기
  printf("Request headers:\n %s\n", request_buf);
  sscanf(request_buf, "%s %s", method, uri); // 요청 라인에서 method, uri를 읽어서 지역 변수 `method` `uri`에 할당
  parse_uri(uri, hostname, port, path);
  sprintf(request_buf, "%s %s %s\r\n", method, path, "HTTP/1.0"); // end server에 전송 요청  & 라인 수정

  // 요청 메소드가 GET | HEAD가 아닌 경우 예외 처리
  if (strcasecmp(method, "GET") && strcasecmp(method, "HEAD"))
  {
    clienterror(clientfd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }

  // end server 소켓 생성
  serverfd = is_local_test ? Open_clientfd(hostname, port) : Open_clientfd("52.79.234.188", port);
  if (serverfd < 0)
  {
    clienterror(serverfd, method, "502", "Bad Gateway", "Failed to establish connection with the end server");
    return;
  }

  /* Request 요청 라인 전송 Proxy -> Server */
  Rio_writen(serverfd, request_buf, strlen(request_buf));

  /* Request 요청 헤더 읽기 & 전송  Client -> Proxy -> Server */
  read_requesthdrs(&request_rio, request_buf, serverfd, hostname, port);

  /* Response 라인 읽기 & 전송  Server -> Proxy -> Client */
  Rio_readinitb(&response_rio, serverfd);                   // 서버의 응답을 담을 버퍼 초기화
  Rio_readlineb(&response_rio, response_buf, MAXLINE);      // 응답 라인 읽기
  Rio_writen(clientfd, response_buf, strlen(response_buf)); // 클라이언트에 응답 라인 보내기
  /* Response  헤더 읽기 & 전송 Server -> Proxy -> Client */
  int content_length;
  while (strcmp(response_buf, "\r\n"))
  {
    Rio_readlineb(&response_rio, response_buf, MAXLINE);
    if (strstr(response_buf, "Content-length")) // 응답 바디 수신에 사용하기 위해 바디 사이즈 저장
      content_length = atoi(strchr(response_buf, ':') + 1);
    Rio_writen(clientfd, response_buf, strlen(response_buf));
  }
  /* Response 바디 읽기 & 전송 Server -> Proxy -> Client */
  if (content_length)
  {
    srcp = malloc(content_length);
    Rio_readnb(&response_rio, srcp, content_length);
    Rio_writen(clientfd, srcp, content_length);
    free(srcp);
  }
}

// 클라이언트에 에러를 전송하는 함수
// cause: 오류 원인, errnum: 오류 번호, shortmsg: 짧은 오류 메시지, longmsg: 긴 오류 메시지
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  /* 응답 본문 생성 */
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor="
                "ffffff"
                ">\r\n",
          body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  /* 응답 출력 */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf)); // 클라이언트에 전송 '버전 에러코드 에러메시지'
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));                              // 컨텐츠 타입
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body)); // \r\n: 헤더와 바디를 나누는 개행
  Rio_writen(fd, buf, strlen(buf));                              // 컨텐츠 크기
  Rio_writen(fd, body, strlen(body));                            // 응답 본문(HTML 형식)
}

// uri를 `hostname`, `port`, `path`로 파싱하는 함수
// uri 형태: `http://hostname:port/path` 혹은 `http://hostname/path` (port는 optional)
void parse_uri(char *uri, char *hostname, char *port, char *path)
{
  // host_name의 시작 위치 포인터: '//'가 있으면 //뒤(ptr+2)부터, 없으면 uri 처음부터
  char *hostname_ptr = strstr(uri, "//") != NULL ? strstr(uri, "//") + 2 : uri;
  char *port_ptr = strchr(hostname_ptr, ':'); // port 시작 위치 (없으면 NULL)
  char *path_ptr = strchr(hostname_ptr, '/'); // path 시작 위치 (없으면 NULL)
  // 경로가 없는 경우를 위한 처리 (path_ptr이 NULL일 때)
  if (path_ptr == NULL)
  {
    strcpy(path, "/"); // 기본 경로
  }
  else
  {
    strcpy(path, path_ptr); // path 구하기
    *path_ptr = '\0';       // hostname_ptr에서 경로 부분을 잘라냄
  }
  if (port_ptr != NULL && (path_ptr == NULL || port_ptr < path_ptr)) // port 있는 경우이고, ':'가 올바른 위치에 있을 때
  {
    // 포트 문자열 복사
    if (path_ptr != NULL)
    {
      char *actual_port_start = port_ptr + 1;
      strcpy(port, actual_port_start); // 실제로는 길이 제한 있는 strncpy 또는 다른 방식 필요
    }
    else
    { // 경로가 없는 경우 (http://host:port)
      strcpy(port, port_ptr + 1);
    }
    *port_ptr = '\0';               // hostname_ptr에서 포트 부분을 잘라냄
    strcpy(hostname, hostname_ptr); // hostname 구하기
  }
  else // port 없는 경우 또는 ':'가 경로 뒤에 오는 등 비정상적인 경우
  {
    strcpy(port, "80");
    strcpy(hostname, hostname_ptr); // hostname 구하기 (이미 경로가 분리된 상태)
  }
  return;
}

// Client의 요청을 읽고 Server에 전송하는 함수
// 요청 받은 헤더에 필수 헤더가 없는 경우 추가로 전송
void read_requesthdrs(rio_t *request_rio, void *request_buf, int serverfd, char *hostname, char *port)
{
  int is_host_exist = 0;
  int is_connection_exist = 0;
  int is_proxy_connection_exist = 0;
  int is_user_agent_exist = 0;

  Rio_readlineb(request_rio, request_buf, MAXLINE); // 요청 메시지의 첫번째 줄 읽기

  while (strcmp(request_buf, "\r\n")) // 버퍼에서 읽은 줄이 '\r\n'이 아닐 때까지 반복
  {
    if (strstr(request_buf, "Proxy-Connection") != NULL)
    {
      sprintf(request_buf, "Proxy-Connection: close\r\n");
      is_proxy_connection_exist = 1;
    }
    else if (strstr(request_buf, "Connection") != NULL)
    {
      sprintf(request_buf, "Connection: close\r\n");
      is_connection_exist = 1;
    }
    else if (strstr(request_buf, "User-Agent") != NULL)
    {
      sprintf(request_buf, user_agent_hdr);
      is_user_agent_exist = 1;
    }
    else if (strstr(request_buf, "Host") != NULL)
    {
      is_host_exist = 1;
    }
    Rio_writen(serverfd, request_buf, strlen(request_buf)); // Server에 전송
    Rio_readlineb(request_rio, request_buf, MAXLINE);       // 다음 줄 읽기
  }

  // 요청 헤더 종료문 전송
  sprintf(request_buf, "\r\n");
  Rio_writen(serverfd, request_buf, strlen(request_buf));
  return;
}