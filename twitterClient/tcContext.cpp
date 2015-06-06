#include "TwitterClient.h"


struct tcMediaData
{
    twitCurlTypes::eTwitCurlMediaType type;
    std::string data;
    std::string media_id;
};


class tcContext
{
public:
    tcContext();
    ~tcContext();

    void load(const char *path);
    void save(const char *path);
    void setConsumerKeyAndSecret(const char *consumer_key, const char *consumer_secret);
    void setAccessToken(const char *token, const char *token_secret);

    bool            isAuthorized();
    std::string&    getAuthoosizeURL();
    bool            enterPin(const char *pin);

    bool            addHashTag(const char *tag);
    bool            addMedia(const void *data, int data_size, twitCurlTypes::eTwitCurlMediaType mtype);
    bool            addMediaFromFile(const char *path);
    int             tweet(const char *message);
    tcETweetStatus  getTweetStatus(int thandle);

private:
    twitCurl m_twitter;
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
    oAuth &oa = m_twitter.getOAuth();
    std::string tmp;

    std::ifstream f(path, std::ios::binary);
    f >> tmp; oa.setConsumerKey(tmp);
    f >> tmp; oa.setConsumerSecret(tmp);
    f >> tmp; oa.setOAuthTokenKey(tmp);
    f >> tmp; oa.setOAuthTokenSecret(tmp);
}

void tcContext::save(const char *path)
{
    oAuth &oa = m_twitter.getOAuth();
    std::string tmp;

    std::ofstream f(path, std::ios::binary);
    oa.getConsumerKey(tmp);     f << tmp << std::endl;
    oa.getConsumerSecret(tmp);  f << tmp << std::endl;
    oa.getOAuthTokenKey(tmp);   f << tmp << std::endl;
    oa.getOAuthTokenSecret(tmp);f << tmp << std::endl;
}


void tcContext::setConsumerKeyAndSecret(const char *consumer_key, const char *consumer_secret)
{
    m_twitter.getOAuth().setConsumerKey(consumer_key);
    m_twitter.getOAuth().setConsumerSecret(consumer_secret);
}

void tcContext::setAccessToken(const char *token, const char *token_secret)
{
    m_twitter.getOAuth().setOAuthTokenKey(token);
    m_twitter.getOAuth().setOAuthTokenSecret(token_secret);
}


bool tcContext::isAuthorized()
{
    std::string token;
    m_twitter.getOAuth().getOAuthTokenSecret(token);
    return !token.empty() && m_twitter.oAuthAccessToken();
}

std::string& tcContext::getAuthoosizeURL()
{
    m_twitter.oAuthRequestToken(m_authorize_url);
    return m_authorize_url;
}

bool tcContext::enterPin(const char *pin)
{
    m_twitter.getOAuth().setOAuthPin(pin);
    return isAuthorized();
}

bool tcContext::addMedia(const void *data, int data_size, twitCurlTypes::eTwitCurlMediaType mtype)
{
    // 画像は 4 枚まで。動画は 1枚まで。
    if (m_media.size() >= 4 ||
        (!m_media.empty() && (m_media.front().type == twitCurlTypes::eTwitCurlMediaGIF || m_media.front().type == twitCurlTypes::eTwitCurlMediaMP4)))
    {
        return false;
    }

    m_media.push_back(tcMediaData());
    tcMediaData &md = m_media.back();
    md.type = mtype;
    md.data.assign((const char*)data, data_size);
    return true;
}


inline bool tcFileToString(std::string &o_buf, const char *path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) { return false; }
    f.seekg(0, std::ios::end);
    o_buf.resize(f.tellg());
    f.seekg(0, std::ios::beg);
    f.read(&o_buf[0], o_buf.size());
    return true;
}

inline twitCurlTypes::eTwitCurlMediaType tcGetMediaTypeFromFilename(const char *path)
{
    std::regex png("\\.png$", std::regex::grep | std::regex::icase);
    std::regex jpg("\\.jpg$", std::regex::grep | std::regex::icase);
    std::regex gif("\\.gif$", std::regex::grep | std::regex::icase);
    std::regex webp("\\.webp$", std::regex::grep | std::regex::icase);
    std::regex mp4("\\.mp4$", std::regex::grep | std::regex::icase);
    std::cmatch match;
    if (std::regex_search(path, match, png)) { return twitCurlTypes::eTwitCurlMediaPNG; }
    if (std::regex_search(path, match, jpg)) { return twitCurlTypes::eTwitCurlMediaJPEG; }
    if (std::regex_search(path, match, gif)) { return twitCurlTypes::eTwitCurlMediaGIF; }
    if (std::regex_search(path, match, webp)){ return twitCurlTypes::eTwitCurlMediaWEBP; }
    if (std::regex_search(path, match, mp4)) { return twitCurlTypes::eTwitCurlMediaMP4; }
    return twitCurlTypes::eTwitCurlMediaUnknown;
}

bool tcContext::addMediaFromFile(const char *path)
{
    std::string binary;
    twitCurlTypes::eTwitCurlMediaType type;
    type = tcGetMediaTypeFromFilename(path);
    if (type == twitCurlTypes::eTwitCurlMediaUnknown) { return false; }
    if (!tcFileToString(binary, path)) { return false; }
    return addMedia(&binary[0], binary.size(), type);
}


int tcContext::tweet(const char *message)
{
    twitStatus stat;
    stat.status = message;
    for (auto &media : m_media)
    {
        if (media.media_id.empty()) {
            media.media_id = m_twitter.uploadMedia(media.data, media.type);
            if (media.media_id.empty()) { continue; }
        }
        if (!stat.media_ids.empty()) { stat.media_ids += ","; }
        stat.media_ids += media.media_id;
    }
    if (m_twitter.statusUpdate(stat)) {
        m_media.clear();
    }
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

tcCLinkage tcExport bool tcAddMedia(tcContext *ctx, const void *data, int data_size, twitCurlTypes::eTwitCurlMediaType mtype)
{
    return ctx->addMedia(data, data_size, mtype);
}

tcCLinkage tcExport bool tcAddMediaFromFile(tcContext *ctx, const char *path)
{
    return ctx->addMediaFromFile(path);
}

tcCLinkage tcExport int tcTweet(tcContext *ctx, const char *message)
{
    return ctx->tweet(message);
}
tcCLinkage tcExport tcETweetStatus tcGetTweetStatus(tcContext *ctx, int thandle)
{
    return ctx->getTweetStatus(thandle);
}
