#include <deque>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "TwitterClient.h"


struct tcMediaData
{
    twitCurlTypes::eTwitCurlMediaType type;
    std::string data;
    std::string file;
    std::string media_id;
};
typedef std::list<tcMediaData> tcMediaCont;
typedef std::shared_ptr<tcMediaCont> tcMediaContPtr;

struct tcTweetData
{
    tcETweetState state;
    tcMediaContPtr media;
    twitStatus status;
    std::string response;
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
    bool            addMediaFile(const char *path);
    int             tweet(const char *message);
    tcTweetData*    getTweetData(int thandle);

private:
    void enqueueTask(const std::function<void()> &f);
    void processTasks();

private:
    twitCurl m_twitter;
    std::string m_authorize_url;
    tcMediaContPtr m_media;
    std::vector<tcTweetData> m_tweets;

    std::thread m_send_thread;
    std::mutex m_queue_mutex;
    std::condition_variable m_condition;
    std::deque<std::function<void ()>> m_tasks;
    bool m_stop;
};



tcContext::tcContext()
    : m_stop(false)
{
    m_send_thread = std::thread([this](){ processTasks(); });
}

tcContext::~tcContext()
{
    m_stop = true;
    m_condition.notify_all();
    m_send_thread.join();
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
    if (!m_media) { m_media.reset(new tcMediaCont()); }
    m_media->push_back(tcMediaData());
    tcMediaData &md = m_media->back();
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

bool tcContext::addMediaFile(const char *path)
{
    // todo: file stream
    std::string binary;
    twitCurlTypes::eTwitCurlMediaType mtype;
    mtype = tcGetMediaTypeFromFilename(path);
    if (mtype == twitCurlTypes::eTwitCurlMediaUnknown) { return false; }
    if (!tcFileToString(binary, path)) { return false; }

    return addMedia(&binary[0], (int)binary.size(), mtype);
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


int tcContext::tweet(const char *message)
{
    // handle 0 must be invalid
    if (m_tweets.empty()) { m_tweets.push_back(tcTweetData()); }
    int ret = (int)m_tweets.size();

    m_tweets.push_back(tcTweetData());
    tcTweetData &tw = m_tweets.back();
    tw.state = tcE_NotCompleted;
    tw.status.status = message;
    tw.media = m_media;
    m_media.reset();

    enqueueTask([this, &tw](){
        if (tw.media) {
            for (auto &media : *tw.media) {
                if (media.media_id.empty()) {
                    media.media_id = m_twitter.uploadMedia(media.data, media.type);
                    if (media.media_id.empty()) { continue; }
                }
                if (!tw.status.media_ids.empty()) { tw.status.media_ids += ","; }
                tw.status.media_ids += media.media_id;
            }
        }
        if (m_twitter.statusUpdate(tw.status)) {
            m_twitter.getLastWebResponse(tw.response);
            tw.state = tcE_Succeeded;
        }
        tw.media.reset();
        tw.state = tcE_Failed;
    });
    return ret;
}

tcTweetData* tcContext::getTweetData(int thandle)
{
    if (thandle <= m_tweets.size())
    {
        return &m_tweets[thandle];
    }
    return nullptr;
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
tcCLinkage tcExport bool tcAddMediaFile(tcContext *ctx, const char *path)
{
    return ctx->addMediaFile(path);
}
tcCLinkage tcExport int tcTweet(tcContext *ctx, const char *message)
{
    return ctx->tweet(message);
}
tcCLinkage tcExport tcETweetState tcGetTweetStatus(tcContext *ctx, int thandle)
{
    return ctx->getTweetData(thandle)->state;
}
tcCLinkage tcExport const char* tcGetTweetResponse(tcContext *ctx, int thandle)
{
    return ctx->getTweetData(thandle)->response.c_str();
}
