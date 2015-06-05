#ifndef TwitterClient_h
#define TwitterClient_h

#include <cstdio>
#include <iostream>
#include <fstream>
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
    #define tcBreak() DebugBreak()
#else // tcWindows
    #define tcExport
    #define tcBreak() __builtin_trap()
#endif // tcWindows


class tcContext;

enum tcETweetStatus
{
    tcE_Unknown,
    tcE_Succeeded,
    tcE_Failed,
};

enum tcEMediaType
{
    tcE_JPEG,
    tcE_PNG,
    tcE_GIF,
    tcE_MP4,
};


tcCLinkage tcExport tcContext*      tcCreateContext();
tcCLinkage tcExport void            tcDestroyContext(tcContext *ctx);

tcCLinkage tcExport void            tcLoad(tcContext *ctx, const char *path);
tcCLinkage tcExport void            tcSave(tcContext *ctx, const char *path);
tcCLinkage tcExport void            tcSetUsernameAndPassword(tcContext *ctx, const char *username, const char *password);
tcCLinkage tcExport void            tcSetConsumerKeyAndSecret(tcContext *ctx, const char *consumer_key, const char *consumer_secret);
tcCLinkage tcExport void            tcSetAccessToken(tcContext *ctx, const char *token, const char *token_secret);

tcCLinkage tcExport bool            tcIsAuthorized(tcContext *ctx);
tcCLinkage tcExport const char*     tcGetAuthorizeURL(tcContext *ctx);
tcCLinkage tcExport bool            tcEnterPin(tcContext *ctx, const char *pin);

tcCLinkage tcExport bool            tcAddMedia(tcContext *ctx, const void *data, int data_size, tcEMediaType mtype);
tcCLinkage tcExport int             tcTweet(tcContext *ctx, const char *message);
tcCLinkage tcExport tcETweetStatus  tcGetTweetStatus(tcContext *ctx, int thandle);

#endif // TwitterClient_h
