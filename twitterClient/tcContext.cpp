#include "TwitterClient.h"


struct tcMediaData
{
    tcEMediaType type;
    std::string data;
};


class tcContext
{
public:
    tcContext();
    ~tcContext();

    void load(const char *path);
    void save(const char *path);
    void setUsernameAndPassword(const char *username, const char *password);
    void setConsumerKeyAndSecret(const char *consumer_key, const char *consumer_secret);
    void setAccessToken(const char *token, const char *token_secret);

    bool            isAuthorized();
    std::string&    getAuthoosizeURL();
    bool            enterPin(const char *pin);

    bool            addMedia(const void *data, int data_size, tcEMediaType mtype);
    int             tweet(const char *message);
    tcETweetStatus  getTweetStatus(int thandle);

private:
    twitCurl m_twitter;

    std::string m_username;
    std::string m_password;
    std::string m_consumer_key;
    std::string m_consumer_secret;
    std::string m_token;
    std::string m_token_secret;

    std::string m_authorize_url;
    std::vector<tcMediaData> m_media;
    std::vector<tcETweetStatus> m_status;
};



tcContext::tcContext()
{
}

tcContext::~tcContext()
{
}


void tcContext::load(const char *path)
{
    std::ifstream f(path, std::ios::binary);
    f >> m_username;
    f >> m_password;
    f >> m_consumer_key;
    f >> m_consumer_secret;
    f >> m_token;
    f >> m_token_secret;
    m_twitter.setTwitterUsername(m_username);
    m_twitter.setTwitterPassword(m_password);
    m_twitter.getOAuth().setConsumerKey(m_consumer_key);
    m_twitter.getOAuth().setConsumerSecret(m_consumer_secret);
    m_twitter.getOAuth().setOAuthTokenKey(m_token);
    m_twitter.getOAuth().setOAuthTokenSecret(m_token_secret);
}

void tcContext::save(const char *path)
{
    std::ofstream f(path, std::ios::binary);
    f << m_username << std::endl;
    f << m_password << std::endl;
    f << m_consumer_key << std::endl;
    f << m_consumer_secret << std::endl;
    f << m_token << std::endl;
    f << m_token_secret << std::endl;
}


void tcContext::setUsernameAndPassword(const char *username, const char *password)
{
    m_username = username;
    m_password = password;
    m_twitter.setTwitterUsername(m_username);
    m_twitter.setTwitterPassword(m_password);
}

void tcContext::setConsumerKeyAndSecret(const char *consumer_key, const char *consumer_secret)
{
    m_consumer_key = consumer_key;
    m_consumer_secret = consumer_secret;
    m_twitter.getOAuth().setConsumerKey(m_consumer_key);
    m_twitter.getOAuth().setConsumerSecret(m_consumer_secret);
}

void tcContext::setAccessToken(const char *token, const char *token_secret)
{
    m_token = token;
    m_token_secret = token_secret;
    m_twitter.getOAuth().setOAuthTokenKey(m_token);
    m_twitter.getOAuth().setOAuthTokenSecret(m_token_secret);
}


bool tcContext::isAuthorized()
{
    return !m_token.empty();
}

std::string& tcContext::getAuthoosizeURL()
{
    m_twitter.oAuthRequestToken(m_authorize_url);
    return m_authorize_url;
}

bool tcContext::enterPin(const char *pin)
{
    m_twitter.getOAuth().setOAuthPin(pin);
    m_twitter.oAuthAccessToken();
    m_twitter.getOAuth().getOAuthTokenKey(m_token);
    m_twitter.getOAuth().getOAuthTokenSecret(m_token_secret);
    return true;
}

bool tcContext::addMedia(const void *data, int data_size, tcEMediaType mtype)
{
    // 画像は 4 枚まで。動画は 1枚まで。
    if (m_media.size() >= 4 ||
        (!m_media.empty() && (m_media.front().type == tcE_GIF || m_media.front().type == tcE_MP4)))
    {
        return false;
    }

    m_media.push_back(tcMediaData());
    tcMediaData &md = m_media.back();
    md.type = mtype;
    md.data.assign((const char*)data, data_size);
    return true;
}

int tcContext::tweet(const char *message)
{
    m_twitter.statusUpdate(message);
    m_media.clear();
    return 0;
}

tcETweetStatus tcContext::getTweetStatus(int thandle)
{
    return tcE_Unknown;
}



tcCLinkage tcExport tcContext* tcCreateContext()
{
    return new tcContext();
}

tcCLinkage tcExport void tcDestroyContext(tcContext *ctx)
{
    delete ctx;
}

tcCLinkage tcExport void tcLoad(tcContext *ctx, const char *path)
{
    ctx->load(path);
}
tcCLinkage tcExport void tcSave(tcContext *ctx, const char *path)
{
    ctx->save(path);
}
tcCLinkage tcExport void tcSetUsernameAndPassword(tcContext *ctx, const char *username, const char *password)
{
    ctx->setUsernameAndPassword(username, password);
}
tcCLinkage tcExport void tcSetConsumerKeyAndSecret(tcContext *ctx, const char *consumer_key, const char *consumer_secret)
{
    ctx->setConsumerKeyAndSecret(consumer_key, consumer_secret);
}
tcCLinkage tcExport void tcSetAccessToken(tcContext *ctx, const char *token, const char *token_secret)
{
    ctx->setAccessToken(token, token_secret);
}

tcCLinkage tcExport bool tcIsAuthorized(tcContext *ctx)
{
    return ctx->isAuthorized();
}
tcCLinkage tcExport const char* tcGetAuthorizeURL(tcContext *ctx)
{
    return ctx->getAuthoosizeURL().c_str();
}
tcCLinkage tcExport bool tcEnterPin(tcContext *ctx, const char *pin)
{
    return ctx->enterPin(pin);
}


tcCLinkage tcExport bool tcAddMedia(tcContext *ctx, const void *data, int data_size, tcEMediaType mtype)
{
    return ctx->addMedia(data, data_size, mtype);
}
tcCLinkage tcExport int tcTweet(tcContext *ctx, const char *message)
{
    return ctx->tweet(message);
}
tcCLinkage tcExport tcETweetStatus tcGetTweetStatus(tcContext *ctx, int thandle)
{
    return ctx->getTweetStatus(thandle);
}
