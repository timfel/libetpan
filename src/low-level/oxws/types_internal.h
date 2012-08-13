/*
 * libEtPan! -- a mail stuff library
 *
 * Copyright (C) 2012 Lysann Kessler
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

#ifndef OXWS_TYPES_INTERNAL_H
#define OXWS_TYPES_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif


#include <libetpan/oxws_types.h>
#include <libetpan/mmapstring.h>

#include <curl/curl.h>
#include <libxml/tree.h>


#define OXWS_INTERNAL(oxws) ((oxws_internal*)((oxws)->internal))


/*
  struct oxws

  A Exchange session object.
*/
struct oxws_internal {
  /* the CURL object used to perform HTTP requests to the service */
  CURL* curl;
  struct curl_slist *curl_headers;

  /* a buffer that typically stores the body of the last HTTP response */
  MMAPString* response_buffer;
  /* holds xml representation of most recent SOAP response */
  xmlParserCtxtPtr response_xml_parser;
};
typedef struct oxws_internal oxws_internal;

oxws_internal* oxws_internal_new();

void oxws_internal_free(oxws_internal* internal);


void oxws_internal_response_buffer_free(oxws_internal* internal);


#ifdef __cplusplus
}
#endif

#endif
