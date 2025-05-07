/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(void)
{
  char *buf, *p;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
  int n1 = 0, n2 = 0;
  char *image_filename = "../jisell.jpeg";

  /* Extract the two arguments */
  if ((buf = getenv("QUERY_STRING")) != NULL)
  {
    p = strchr(buf, '&');
    *p = '\0';
    strcpy(arg1, buf);
    strcpy(arg2, p + 1);
    n1 = atoi(strchr(arg1, '=') + 1);
    n2 = atoi(strchr(arg2, '=') + 1);
  }

  /* Make the response body */
  sprintf(content, "buf before arg1=%s\r\n<p>", buf);
  sprintf(content + strlen(content), "QUERY_STRING=%s\r\n<p>", arg1);
  sprintf(content + strlen(content), "QUERY_STRING=%s\r\n<p>", arg2);
  sprintf(content + strlen(content), "buf before arg2=%s\r\n<p>", p + 1);

  sprintf(content + strlen(content),
          "<img src=\"%s\" alt=\"Calculation Icon\" width=\"400\"> \r\n<p>", // width 등 속성 추가 가능
          image_filename);                                                   // 위에서 정의한 이미지 경로 사용
  sprintf(content + strlen(content), "Welcome to add.com: ");
  sprintf(content + strlen(content), "THE Internet addition portal.\r\n<p>");
  sprintf(content + strlen(content), "The answer is: %d + %d = %d\r\n<p>",
          n1, n2, n1 + n2);
  sprintf(content + strlen(content), "Thanks for visiting!\r\n");

  /* Generate the HTTP response */
  printf("Content-type: text/html\r\n");
  printf("Content-length: %d\r\n", (int)strlen(content));
  printf("\r\n");
  printf("%s", content);
  fflush(stdout);

  exit(0);
}
/* $end adder */
