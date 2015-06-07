#include "TwitterClient.h"

#define CredentialsPath "credentials.txt"




class TestBase
{
public:
    TestBase()
    {
        m_ctx = tcCreateContext();
    }

    virtual ~TestBase()
    {
        tcDestroyContext(m_ctx);
    }

    void saveCredentials()
    {
        tcSaveCredentials(m_ctx, CredentialsPath);
    }

    void loadCredentials()
    {
        tcLoadCredentials(m_ctx, CredentialsPath);
    }

    void addMedia(std::string &bin, twitCurlTypes::eTwitCurlMediaType mtype)
    {
        tcAddMedia(m_ctx, &bin[0], bin.size(), mtype);
    }

    void addMediaFile(const char *path)
    {
        tcAddMediaFile(m_ctx, path);
    }

    virtual tcAuthState verifyCredentials() = 0;
    virtual tcAuthState requestAuthURL(const char *consumer_key, const char *consumer_secret) = 0;
    virtual tcAuthState enterPin(const char *pin) = 0;
    virtual tcTweetState tweet(const char *message) = 0;

protected:
    tcContext *m_ctx;
};


class TestSync : public TestBase
{
public:
    tcAuthState verifyCredentials() override
    {
        return tcVerifyCredentials(m_ctx);
    }

    tcAuthState requestAuthURL(const char *consumer_key, const char *consumer_secret) override
    {
        return tcRequestAuthURL(m_ctx, consumer_key, consumer_secret);
    }

    tcAuthState enterPin(const char *pin) override
    {
        return tcEnterPin(m_ctx, pin);
    }

    tcTweetState tweet(const char *message) override
    {
        int th = tcTweet(m_ctx, message);
        return tcGetTweetState(m_ctx, th);
    }
};


class TestAsync : public TestBase
{
public:
    tcAuthState verifyCredentials() override
    {
        tcAuthState r;
        tcVerifyCredentialsAsync(m_ctx);
        waitFor([&](){
            r = tcGetVerifyCredentialsState(m_ctx);
            return r.code != tcEStatusCode_InProgress;
        });
        return r;
    }

    tcAuthState requestAuthURL(const char *consumer_key, const char *consumer_secret) override
    {
        tcAuthState r;
        tcRequestAuthURLAsync(m_ctx, consumer_key, consumer_secret);
        waitFor([&](){
            r = tcGetRequestAuthURLState(m_ctx);
            return r.code != tcEStatusCode_InProgress;
        });
        return r;
    }

    tcAuthState enterPin(const char *pin) override
    {
        tcAuthState r;
        tcEnterPinAsync(m_ctx, pin);
        waitFor([&](){
            r = tcGetEnterPinState(m_ctx);
            return r.code != tcEStatusCode_InProgress;
        });
        return r;
    }

    tcTweetState tweet(const char *message) override
    {
        tcTweetState r;
        int th = tcTweetAsync(m_ctx, message);
        waitFor([&](){
            r = tcGetTweetState(m_ctx, th);
            bool ret = r.code != tcEStatusCode_InProgress;
            if (!ret) { printf("in progress...\n"); }
            return ret;
        }, 2000);
        return r;
    }

private:
    template<class F>
    void waitFor(const F& f, int millisec=100)
    {
        while (!f()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(millisec));
        }
    }
};




void DoTest(TestBase &test)
{
    char buf1[512];
    char buf2[512];

    test.loadCredentials();

    bool auth = false;
    auth = test.verifyCredentials().code == tcEStatusCode_Succeeded;
    if (!auth)
    {
        printf("consumer key:\n");
        gets(buf1);
        printf("consumer secret:\n");
        gets(buf2);

        auto stat = test.requestAuthURL(buf1, buf2);
        if (stat.code != tcEStatusCode_Succeeded) {
            printf("error: %s\n", stat.error_message);
            return;
        }

        printf("authorize URL: %s\n", stat.auth_url);
        printf("pin:\n");
        gets(buf1);

        stat = test.enterPin(buf1);
        if (stat.code != tcEStatusCode_Succeeded) {
            printf("error: %s\n", stat.error_message);
            return;
        }
        test.saveCredentials();
        auth = true;
    }

    if (auth)
    {
        std::string stat;

        printf("media:\n");
        gets(buf1);
        if (isprint(buf1[0])) {
            printf("on memory? [y][n]:\n");
            gets(buf2);
            if (buf2[0]=='y') {
                std::string media;
                if (tcFileToString(media, buf1)) {
                    test.addMedia(media, tcGetMediaTypeByFilename(buf1));
                }
            }
            else {
                test.addMediaFile(buf1);
            }
        }

        //tcFileToString(stat, "stat.txt");
        printf("tweet:\n");
        gets(buf1);
        stat = buf1;

        auto st = test.tweet(stat.c_str());
        if (st.code == tcEStatusCode_Succeeded) {
            printf("succeeded.\n");
        }
        else if (st.code == tcEStatusCode_Failed) {
            printf("failed. %s\n", st.error_message);
        }
    }
}


int main(int argc, char *argv[])
{
    char buf1[512];
    printf("[s] to sync [a] to async:\n");
    gets(buf1);

    if (buf1[0]=='a') {
        printf("TestAsync\n");
        TestAsync test;
        DoTest(test);
    }
    else {
        printf("TestSync\n");
        TestSync test;
        DoTest(test);
    }

    printf("done.\n");
    gets(buf1);
}
