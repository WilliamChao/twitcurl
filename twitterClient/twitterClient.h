#ifndef TwitterClient_h
#define TwitterClient_h

#include <cstdio>
#include <iostream>
#include <fstream>
#include <memory>
#include <thread>
#include "twitcurl.h"

//#pragma comment(lib, "libeay32.lib")
//#pragma comment(lib, "ssleay32.lib")
#pragma comment(lib, "libcurl.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "wldap32.lib")
#pragma comment(lib, "normaliz.lib")


#ifdef _WIN32
#define tcWindows
#endif // _WIN32

#define tcCLinkage extern "C"
#ifdef tcWindows
    #define tcExport __declspec(dllexport)
#else // tcWindows
    #define tcExport
#endif // tcWindows


class tcContext;

enum tcEStatusCode
{
    tcEStatusCode_Unknown,
    tcEStatusCode_InProgress,
    tcEStatusCode_Failed,
    tcEStatusCode_Succeeded,
};


struct tcAuthState
{
    tcEStatusCode code;
    const char *error_message;
    const char *auth_url;
};


struct tcTweetState
{
    tcEStatusCode code;
    const char *error_message;
};


tcCLinkage tcExport tcContext*      tcCreateContext();
tcCLinkage tcExport void            tcDestroyContext(tcContext *ctx);

tcCLinkage tcExport void            tcLoadCredentials(tcContext *ctx, const char *path);
tcCLinkage tcExport void            tcSaveCredentials(tcContext *ctx, const char *path);

tcCLinkage tcExport tcAuthState     tcVerifyCredentials(tcContext *ctx);
tcCLinkage tcExport void            tcVerifyCredentialsAsync(tcContext *ctx);
tcCLinkage tcExport tcAuthState     tcGetVerifyCredentialsState(tcContext *ctx);

tcCLinkage tcExport tcAuthState     tcRequestAuthURL(tcContext *ctx, const char *consumer_key, const char *consumer_secret);
tcCLinkage tcExport void            tcRequestAuthURLAsync(tcContext *ctx, const char *consumer_key, const char *consumer_secret);
tcCLinkage tcExport tcAuthState     tcGetRequestAuthURLState(tcContext *ctx);

tcCLinkage tcExport tcAuthState     tcEnterPin(tcContext *ctx, const char *pin);
tcCLinkage tcExport void            tcEnterPinAsync(tcContext *ctx, const char *pin);
tcCLinkage tcExport tcAuthState     tcGetEnterPinState(tcContext *ctx);

tcCLinkage tcExport bool            tcAddMedia(tcContext *ctx, const void *data, int data_size, twitCurlTypes::eTwitCurlMediaType mtype);
tcCLinkage tcExport bool            tcAddMediaFile(tcContext *ctx, const char *path);
tcCLinkage tcExport int             tcTweet(tcContext *ctx, const char *message);
tcCLinkage tcExport int             tcTweetAsync(tcContext *ctx, const char *message);
tcCLinkage tcExport tcTweetState    tcGetTweetState(tcContext *ctx, int thandle);
tcCLinkage tcExport void            tcEraseTweetCache(tcContext *ctx, int thandle);


bool                                tcFileToString(std::string &o_buf, const char *path);
twitCurlTypes::eTwitCurlMediaType   tcGetMediaTypeByFilename(const char *path);

#endif // TwitterClient_h
