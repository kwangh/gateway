#include <stdio.h>
#include <string.h> //strlen
#include <curl/curl.h>

int main(int argc, char *argv[])
{
  CURL *curl;
  CURLcode res;
  curl_slist *headers = NULL;

  char buffer[100];
  int cx;

  curl = curl_easy_init();
  if (curl)
  {
    curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.6.116:8080/test2");
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    cx = snprintf(buffer, 100, "{\"container_name\":\"%s\",\"container_state\":\"%s\"}","c1","START");
    printf("buffer: %s\n",buffer);
    printf("snprintf return: %d\n",cx);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, buffer);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(buffer));

    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
  }
  return 0;
}
