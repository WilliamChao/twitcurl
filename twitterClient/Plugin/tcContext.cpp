



class tcContext
{
public:
    tcContext();
    ~tcContext();

private:
    std::string m_filepath;
    std::string m_consumer_key;
    std::string m_consumer_secret;
    std::string m_token;
    std::string m_token_secret;
    std::string m_authorize_url;
};

tcCLinkage tcExport tcContext*  tcCreateContext(const char *path);
tcCLinkage tcExport void        tcDestroyContext(tcContext *ctx);
tcCLinkage tcExport bool        tcIsAuthorized(tcContext *ctx);
tcCLinkage tcExport const char* tcGetAuthorizeURL(tcContext *ctx, const char *consumer_key, const char *consumer_secret);
tcCLinkage tcExport bool        tcEnterPin(tcContext *ctx, const char *pin);
tcCLinkage tcExport bool        tcTweet(tcContext *ctx, const char *message);
tcCLinkage tcExport bool        tcTweetWithMedia(tcContext *ctx, const char *message, const char *data, int data_size);
tcCLinkage tcExport bool        tcTweetWithMediaFile(tcContext *ctx, const char *message, const char *path_to_media);
