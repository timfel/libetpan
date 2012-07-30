/*
 * libEtPan! -- a mail stuff library
 *
 * exhange support: Copyright (C) 2012 Lysann Kessler
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the libEtPan! project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <curl/curl.h>

#include <stdlib.h>
#include <string.h>

#include <libetpan/mailexch_helper.h>


#define MAILEXCH_AUTODISCOVER_REQUEST_FORMAT ( \
  "<Autodiscover xmlns=\"http://schemas.microsoft.com/exchange/autodiscover/outlook/requestschema/2006\">\n" \
  "  <Request>\n" \
  "    <EMailAddress>lysann.kessler@student.hpi.uni-potsdam.de</EMailAddress>\n" \
  "    <AcceptableResponseSchema>http://schemas.microsoft.com/exchange/autodiscover/outlook/responseschema/2006a</AcceptableResponseSchema>\n" \
  "  </Request>\n" \
  "</Autodiscover>")

#define MAILEXCH_AUTODISCOVER_MIN_RESPONSE_BUFFER_LENGTH 3000

#define MAILEXCH_AUTODISCOVER_STEP1_URL_FORMAT "https://%s/autodiscover/autodiscover.xml"
#define MAILEXCH_AUTODISCOVER_STEP2_URL_FORMAT "https://autodiscover.%s/autodiscover/autodiscover.xml"
#define MAILEXCH_AUTODISCOVER_URL_LENGTH strlen(MAILEXCH_AUTODISCOVER_STEP2_URL_FORMAT - 2) /* longer */


#define MAILEXCH_AUTODISCOVER_TRY_STEP_LONG(step, exch, url_buffer, host, result) \
  do { \
    sprintf(url_buffer, MAILEXCH_AUTODISCOVER_STEP##step##_URL_FORMAT, host); \
    result = mailexch_autodiscover_try_url(exch, url_buffer); \
  } while(0);

#define MAILEXCH_AUTODISCOVER_TRY_STEP(step) MAILEXCH_AUTODISCOVER_TRY_STEP_LONG(step, exch, url, host, result)


int mailexch_autodiscover_try_url(mailexch* exch, const char* url);
size_t mailexch_autodiscover_write_callback(char *ptr, size_t size, size_t nmemb, void *userdata);


int mailexch_autodiscover(mailexch* exch, const char* email_address, const char* host,
                          const char* username, const char* password, const char* domain) {
  /* http://msdn.microsoft.com/en-us/library/exchange/ee332364(v=exchg.140).aspx */

  /* get host name */
  if(host == NULL) {
    host = strstr(email_address, "@");
    if(host == NULL) return MAILEXCH_ERROR_INVALID_PARAMETER;
    host += 1;
    if(*host == 0) return MAILEXCH_ERROR_INVALID_PARAMETER; /* end of string after @ */
  }

  /* prepare curl: curl object + credentials */
  int result = mailexch_prepare_curl(exch, username, password, domain);
  if(result != MAILEXCH_NO_ERROR) return result;

  /* headers */
  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "Content-Type: text/xml");
  curl_easy_setopt(exch->curl, CURLOPT_HTTPHEADER, headers);

  /* Follow redirects, but only to HTTPS. */
  curl_easy_setopt(exch->curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(exch->curl, CURLOPT_MAXREDIRS, 10L);
  curl_easy_setopt(exch->curl, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTPS);
  curl_easy_setopt(exch->curl, CURLOPT_UNRESTRICTED_AUTH, 0L);

  /* content */
  char* request = (char*) malloc(
    strlen(MAILEXCH_AUTODISCOVER_REQUEST_FORMAT) - 2 + /* remove format characters */
    strlen(email_address) + 1);                        /* add email address and null terminator */
  if(!request) {
    curl_easy_setopt(exch->curl, CURLOPT_FOLLOWLOCATION, 0L);
    curl_slist_free_all(headers);
    return MAILEXCH_ERROR_INTERNAL;
  }
  sprintf(request, MAILEXCH_AUTODISCOVER_REQUEST_FORMAT, email_address);
  curl_easy_setopt(exch->curl, CURLOPT_POSTFIELDS, request);

  /* result */
  if(mailexch_write_response_to_buffer(exch, MAILEXCH_AUTODISCOVER_MIN_RESPONSE_BUFFER_LENGTH) != MAILEXCH_NO_ERROR) {
    curl_easy_setopt(exch->curl, CURLOPT_FOLLOWLOCATION, 0L);
    curl_slist_free_all(headers);
    free(request);
    return MAILEXCH_ERROR_INTERNAL;
  }

  /* try steps: */
  /*   allocate buffer */
  char* url = malloc(MAILEXCH_AUTODISCOVER_URL_LENGTH + strlen(host) + 1);
  if(!url) {
    mailexch_free_response_buffer(exch);
    curl_easy_setopt(exch->curl, CURLOPT_FOLLOWLOCATION, 0L);
    curl_slist_free_all(headers);
    free(request);
    return MAILEXCH_ERROR_INTERNAL;
  }
  /*   try */
  MAILEXCH_AUTODISCOVER_TRY_STEP(1);
  if(result != MAILEXCH_NO_ERROR)
    MAILEXCH_AUTODISCOVER_TRY_STEP(2);
  /*   set result */
  if(result != MAILEXCH_NO_ERROR)
    result = MAILEXCH_ERROR_AUTODISCOVER_UNAVAILABLE;

  /* clean up */
  free(url);
  mailexch_free_response_buffer(exch);
  curl_easy_setopt(exch->curl, CURLOPT_FOLLOWLOCATION, 0L);
  curl_slist_free_all(headers);
  curl_easy_setopt(exch->curl, CURLOPT_HTTPHEADER, NULL);
  free(request);
  return result;
}

int mailexch_autodiscover_try_url(mailexch* exch, const char* url) {
  curl_easy_setopt(exch->curl, CURLOPT_URL, url);
  CURLcode curl_code = curl_easy_perform(exch->curl);

  int result = MAILEXCH_ERROR_CONNECT;
  if(curl_code == CURLE_OK) {
    long http_response = 0;
    curl_easy_getinfo (exch->curl, CURLINFO_RESPONSE_CODE, &http_response);
    if(http_response == 200) {
      result = MAILEXCH_ERROR_AUTODISCOVER_UNAVAILABLE;

      /* parse ASUrl */
      char* as_url = strstr(exch->response_buffer, "<ASUrl>");
      if(as_url != NULL) {
        as_url += 7;
        char* as_url_end = strstr(as_url, "</ASUrl>");
        if(as_url_end != NULL) {

          /* copy ASUrl */
          size_t as_url_length = as_url_end - as_url;
          exch->connection_settings.as_url = (char*) malloc(as_url_length + 1);
          memcpy(exch->connection_settings.as_url, as_url, as_url_length);
          exch->connection_settings.as_url[as_url_length] = 0;

          result = MAILEXCH_NO_ERROR;
        }
      }
    }
  }

  return result;
}
