#include "TwitterClient.h"

#define tcPathToUserdata "tc_userdata.txt"

int main(int argc, char *argv[])
{
    char buf1[512];
    char buf2[512];

    tcContext *ctx = tcCreateContext();
    tcLoad(ctx, tcPathToUserdata);

    if (!tcIsAuthorized(ctx))
    {
        printf("username:\n");
        gets(buf1);
        printf("password:\n");
        gets(buf2);
        tcSetUsernameAndPassword(ctx, buf1, buf2);

        printf("consumer key:\n");
        gets(buf1);
        printf("consumer secret:\n");
        gets(buf2);
        tcSetConsumerKeyAndSecret(ctx, buf1, buf2);

        printf("authorize URL: %s\n", tcGetAuthorizeURL(ctx));

        printf("pin:\n");
        gets(buf1);
        tcEnterPin(ctx, buf1);
    }

    if (tcIsAuthorized(ctx))
    {
        printf("tweet:\n");
        gets(buf1);
        tcTweet(ctx, buf1);
    }

    tcSave(ctx, tcPathToUserdata);
    tcDestroyContext(ctx);
}
