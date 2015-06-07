#include <deque>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <regex>
#include <picojson/picojson.h>
#include "TwitterClient.h"


struct tcMediaData
{
    twitCurlTypes::eTwitCurlMediaType type;
    std::shared_ptr<std::istream> stream;
    std::string media_id;
};
typedef std::list<tcMediaData> tcMediaCont;
typedef std::shared_ptr<tcMediaCont> tcMediaContPtr;


struct tcTweetData
{
    tcEStatusCode code;
    tcMediaContPtr media;
    twitStatus tweet;
    std::string error_message;

    tcTweetData() : code(tcEStatusCode_Unknown) {}
};
typedef std::vector<tcTweetData> tcTweetDataCont;


class tcContext
{
public:
    tcContext();
    ~tcContext();

    void loadCredentials(const char *path);
    void saveCredentials(const char *path);
    void setConsumerKeyAndSecret(const char *consumer_key, const char *consumer_secret);
    void setAccessToken(const char *token, const char *token_secret);

    tcAuthState     verifyCredentials();
    void            verifyCredentialsAsync();
    tcAuthState     getVerifyCredentialsState();

    tcAuthState     requestAuthURL(const char *consumer_key, const char *consumer_secret);
    void            requestAuthURLAsync(const char *consumer_key, const char *consumer_secret);
    tcAuthState     getRequestAuthURLState();

    tcAuthState     enterPin(const char *pin);
    void            enterPinAsync(const char *pin);
    tcAuthState     getEnterPinState();

    bool            addMedia(const void *data, int data_size, twitCurlTypes::eTwitCurlMediaType mtype);
    bool            addMediaFile(const char *path);
    int             tweet(const char *message);
    int             tweetAsync(const char *message);
    tcTweetState    getTweetStatus(int thandle);
    void            eraseTweetCache(int thandle);
    tcTweetData*    getTweetData(int thandle);

private:
    bool getErrorMessage(std::string &dst, bool error_if_response_is_not_json=false);
    int pushTweet(const char *message);
    void tweetImpl(tcTweetData &tw);
    void enqueueTask(const std::function<void()> &f);
    void processTasks();

private:
    twitCurl m_twitter;

    tcEStatusCode m_auth_code;
    std::string m_auth_url;
    std::string m_auth_error;

    tcMediaContPtr m_media;
    tcTweetDataCont m_tweets;

    std::thread m_send_thread;
    std::mutex m_queue_mutex;
    std::condition_variable m_condition;
    std::deque<std::function<void ()>> m_tasks;
    bool m_stop;
};



tcContext::tcContext()
    : m_stop(false)
    , m_auth_code(tcEStatusCode_Unknown)
{
    m_send_thread = std::thread([this](){ processTasks(); });
}

tcContext::~tcContext()
{
    m_stop = true;
    m_condition.notify_all();
    m_send_thread.join();
}


void tcContext::loadCredentials(const char *path)
{
    oAuth &oa = m_twitter.getOAuth();
    std::string tmp;

    std::ifstream f(path, std::ios::binary);
    f >> tmp; oa.setConsumerKey(tmp);
    f >> tmp; oa.setConsumerSecret(tmp);
    f >> tmp; oa.setOAuthTokenKey(tmp);
    f >> tmp; oa.setOAuthTokenSecret(tmp);
}

void tcContext::saveCredentials(const char *path)
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

bool tcContext::getErrorMessage(std::string &dst, bool error_if_response_is_not_json)
{
    dst.clear();

    picojson::value v;
    std::string tmp;
    m_twitter.getLastWebResponse(tmp);
    std::string err = picojson::parse(v, tmp);
    if (!err.empty()) {
        if (error_if_response_is_not_json) {
            dst = err;
            return true;
        }
        else {
            return false;
        }
    }
    if(v.contains("errors")) {
        try {
            picojson::value obj = v.get("errors").get<picojson::array>()[0];
            dst = obj.get("message").get<std::string>();
        }
        catch (...) {}
        return true;
    }
    return false;
}


tcAuthState tcContext::verifyCredentials()
{
    tcEStatusCode code = tcEStatusCode_Failed;
    if (m_twitter.accountVerifyCredGet()) {
        code = getErrorMessage(m_auth_error) ? tcEStatusCode_Failed : tcEStatusCode_Succeeded;
    }
    m_auth_code = code;
    return getVerifyCredentialsState();
}

void tcContext::verifyCredentialsAsync()
{
    m_auth_code = tcEStatusCode_InProgress;
    enqueueTask([this](){ verifyCredentials(); });
}

tcAuthState tcContext::getVerifyCredentialsState()
{
    tcAuthState r = { m_auth_code, m_auth_error.c_str(), nullptr};
    return r;
}



tcAuthState tcContext::requestAuthURL(const char *consumer_key, const char *consumer_secret)
{
    tcEStatusCode code = tcEStatusCode_Failed;
    m_twitter.getOAuth().setConsumerKey(consumer_key);
    m_twitter.getOAuth().setConsumerSecret(consumer_secret);
    if (m_twitter.oAuthRequestToken(m_auth_url)) {
        code = getErrorMessage(m_auth_error) ? tcEStatusCode_Failed : tcEStatusCode_Succeeded;
    }
    m_auth_code = code;;
    return getRequestAuthURLState();
}

void tcContext::requestAuthURLAsync(const char *consumer_key_, const char *consumer_secret_)
{
    m_auth_code = tcEStatusCode_InProgress;
    std::string consumer_key = consumer_key_;
    std::string consumer_secret = consumer_secret_;
    enqueueTask([=](){ requestAuthURL(consumer_key.c_str(), consumer_secret.c_str()); });
}
tcAuthState tcContext::getRequestAuthURLState()
{
    tcAuthState r = { m_auth_code, m_auth_error.c_str(), m_auth_url.c_str() };
    return r;
}



tcAuthState tcContext::enterPin(const char *pin)
{
    tcEStatusCode code = tcEStatusCode_Failed;
    m_twitter.getOAuth().setOAuthPin(pin);
    if (m_twitter.oAuthAccessToken()) {
        std::string tmp;
        m_twitter.getLastWebResponse(tmp);
        if (tmp.find("oauth_token=") != std::string::npos) {
            code = tcEStatusCode_Succeeded;
            m_auth_error.clear();
        }
        else {
            m_auth_error = tmp;
        }
    }
    m_auth_code = code;
    return getEnterPinState();
}

void tcContext::enterPinAsync(const char *pin_)
{
    m_auth_code = tcEStatusCode_InProgress;
    std::string pin = pin_;
    enqueueTask([=](){ enterPin(pin.c_str()); });
}

tcAuthState tcContext::getEnterPinState()
{
    tcAuthState r = { m_auth_code, m_auth_error.c_str(), nullptr };
    return r;
}

bool tcContext::addMedia(const void *data, int data_size, twitCurlTypes::eTwitCurlMediaType mtype)
{
    if (!m_media) { m_media.reset(new tcMediaCont()); }
    m_media->push_back(tcMediaData());
    tcMediaData &md = m_media->back();
    md.type = mtype;
    md.stream.reset(
        new std::istringstream(std::move(std::string((const char*)data, data_size)), std::ios::binary));
    return true;
}


void tcContext::enqueueTask(const std::function<void()> &f)
{
    {
        std::unique_lock<std::mutex> lock(m_queue_mutex);
        m_tasks.push_back(std::function<void()>(f));
    }
    m_condition.notify_one();
}

void tcContext::processTasks()
{
    while (!m_stop)
    {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            while (!m_stop && m_tasks.empty()) {
                m_condition.wait(lock);
            }
            if (m_stop) { return; }

            task = m_tasks.front();
            m_tasks.pop_front();
        }
        task();
    }
}

void tcContext::tweetImpl(tcTweetData &tw)
{
    tcEStatusCode code = tcEStatusCode_Failed;
    if (tw.media) {
        for (auto &media : *tw.media) {
            if (!m_twitter.uploadMedia(*media.stream, media.type, media.media_id, tw.error_message)) {
                break;
            }
            if (!tw.tweet.media_ids.empty()) { tw.tweet.media_ids += ","; }
            tw.tweet.media_ids += media.media_id;
        }
    }
    if (tw.error_message.empty()) {
        if (m_twitter.statusUpdate(tw.tweet)) {
            code = getErrorMessage(tw.error_message) ? tcEStatusCode_Failed : tcEStatusCode_Succeeded;
        }
    }
    tw.media.reset();
    tw.code = code;
}

int tcContext::pushTweet(const char *message)
{
    // handle 0 == invalid
    if (m_tweets.empty()) { m_tweets.push_back(tcTweetData()); }
    int ret = (int)m_tweets.size();

    m_tweets.push_back(tcTweetData());
    tcTweetData &tw = m_tweets.back();
    tw.code = tcEStatusCode_InProgress;
    tw.tweet.status = message;
    tw.media = m_media;
    m_media.reset();
    return ret;
}

int tcContext::tweet(const char *message)
{
    int t = pushTweet(message);
    tweetImpl(m_tweets[t]);
    return t;
}

int tcContext::tweetAsync(const char *message)
{
    int t = pushTweet(message);
    enqueueTask([this, t](){ tweetImpl(m_tweets[t]); });
    return t;
}

tcTweetState tcContext::getTweetStatus(int thandle)
{
    tcTweetState r = { tcEStatusCode_Unknown, nullptr };
    if (thandle <= m_tweets.size())
    {
        auto & tw = m_tweets[thandle];
        r.code = tw.code;
        r.error_message = tw.error_message.c_str();
    }
    return r;
}

void tcContext::eraseTweetCache(int thandle)
{
    if (thandle <= m_tweets.size() && thandle > 0)
    {
        m_tweets.erase(m_tweets.begin()+thandle);
    }
}

tcTweetData* tcContext::getTweetData(int thandle)
{
    if (thandle <= m_tweets.size())
    {
        return &m_tweets[thandle];
    }
    return nullptr;
}

bool tcContext::addMediaFile(const char *path)
{
    twitCurlTypes::eTwitCurlMediaType mtype;
    mtype = tcGetMediaTypeByFilename(path);
    if (mtype == twitCurlTypes::eTwitCurlMediaUnknown) { return false; }

    if (!m_media) { m_media.reset(new tcMediaCont()); }
    m_media->push_back(tcMediaData());
    tcMediaData &md = m_media->back();
    md.type = mtype;
    md.stream.reset(new std::ifstream(path, std::ios::binary));

    return true;
}




bool tcFileToString(std::string &o_buf, const char *path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) { return false; }
    f.seekg(0, std::ios::end);
    o_buf.resize(f.tellg());
    f.seekg(0, std::ios::beg);
    f.read(&o_buf[0], o_buf.size());
    return true;
}

twitCurlTypes::eTwitCurlMediaType tcGetMediaTypeByFilename(const char *path)
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



tcCLinkage tcExport tcContext* tcCreateContext()
{
    return new tcContext();
}

tcCLinkage tcExport void tcDestroyContext(tcContext *ctx)
{
    delete ctx;
}

tcCLinkage tcExport void tcLoadCredentials(tcContext *ctx, const char *path)
{
    ctx->loadCredentials(path);
}
tcCLinkage tcExport void tcSaveCredentials(tcContext *ctx, const char *path)
{
    ctx->saveCredentials(path);
}


tcCLinkage tcExport tcAuthState tcVerifyCredentials(tcContext *ctx)
{
    return ctx->verifyCredentials();
}
tcCLinkage tcExport void tcVerifyCredentialsAsync(tcContext *ctx)
{
    ctx->verifyCredentialsAsync();
}
tcCLinkage tcExport tcAuthState tcGetVerifyCredentialsState(tcContext *ctx)
{
    return ctx->getVerifyCredentialsState();
}

tcCLinkage tcExport tcAuthState  tcRequestAuthURL(tcContext *ctx, const char *consumer_key, const char *consumer_secret)
{
    return ctx->requestAuthURL(consumer_key, consumer_secret);
}
tcCLinkage tcExport void tcRequestAuthURLAsync(tcContext *ctx, const char *consumer_key, const char *consumer_secret)
{
    ctx->requestAuthURLAsync(consumer_key, consumer_secret);
}
tcCLinkage tcExport tcAuthState  tcGetRequestAuthURLState(tcContext *ctx)
{
    return ctx->getRequestAuthURLState();
}

tcCLinkage tcExport tcAuthState tcEnterPin(tcContext *ctx, const char *pin)
{
    return ctx->enterPin(pin);
}
tcCLinkage tcExport void tcEnterPinAsync(tcContext *ctx, const char *pin)
{
    ctx->enterPinAsync(pin);
}
tcCLinkage tcExport tcAuthState tcGetEnterPinState(tcContext *ctx)
{
    return ctx->getEnterPinState();
}


tcCLinkage tcExport bool tcAddMedia(tcContext *ctx, const void *data, int data_size, twitCurlTypes::eTwitCurlMediaType mtype)
{
    return ctx->addMedia(data, data_size, mtype);
}
tcCLinkage tcExport bool tcAddMediaFile(tcContext *ctx, const char *path)
{
    return ctx->addMediaFile(path);
}
tcCLinkage tcExport int tcTweet(tcContext *ctx, const char *message)
{
    return ctx->tweet(message);
}
tcCLinkage tcExport int tcTweetAsync(tcContext *ctx, const char *message)
{
    return ctx->tweetAsync(message);
}
tcCLinkage tcExport tcTweetState tcGetTweetState(tcContext *ctx, int thandle)
{
    return ctx->getTweetStatus(thandle);
}
tcCLinkage tcExport void tcEraseTweetCache(tcContext *ctx, int thandle)
{
    ctx->eraseTweetCache(thandle);
}
