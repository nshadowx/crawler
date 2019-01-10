//
// Created by root on 07.01.19.
//

#ifndef UNTITLED1_CRAWLER_HPP
#define UNTITLED1_CRAWLER_HPP

#include "ThreadPool.h"
#include "root_certificates.hpp" // прописать свой путь
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <gumbo.h>
#include <future>
#include <cstdlib>
#include <mutex>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <map>

using tcp = boost::asio::ip::tcp;
namespace ssl = boost::asio::ssl;
namespace http = boost::beast::http;

class Crawler{
    public:
        Crawler(const std::string& url,const std::string& output,const unsigned& depth,const unsigned& network_threads,const unsigned& parser_threads);
        void crawler_start();
    private:
        void parse_url_for_pict(std::string str,unsigned depth);
        void downloader(std::string& url, unsigned depth);
        void parse_url(GumboNode* node);
        std::string parse_url_to_host(std::string url);
        std::string parse_url_to_target(std::string url);
        std::string downloader_url(std::string host, std::string target);
        void do_something(GumboNode* node, unsigned depth);


        std::vector<std::string> urls_;
        std::map <std::string,std::string> urlss_;
        std::mutex mtx;
        std::recursive_mutex mx;
        std::recursive_mutex mx1;
        std::string url_;
        std::string output_;
        unsigned depth_;
        unsigned network_threads_;
        unsigned parser_threads_;
        ThreadPool pools;
        ThreadPool parses_pools;
};

//Crawler::Crawler(const std::string& url):url_(url){}
Crawler::Crawler(const std::string& url,const std::string& output,const unsigned& depth,const unsigned& network_threads,const unsigned& parser_threads):
    url_(url),
    output_(output),
    depth_(depth),
    pools(network_threads),
    parses_pools(parser_threads)
{}
    //pools = 4;
    //url_=url;
    //output_=output;
    //depth_=depth;
    //network_threads_=network_threads;
    //parser_threads_=parser_threads;

//}
std::string Crawler::parse_url_to_host(std::string url){
    if (url.find("https://") == 0)
        url = url.substr(8);
    std::string result_host ;
    for (unsigned i = 0; i < url.size(); ++i) {
        if ((url[i] == '/') || (url[i] == '?')) break;
        result_host+=url[i];
    }
    return result_host;
}
std::string Crawler::parse_url_to_target(std::string url){
    if (url.find("https:") == 0)
        url = url.substr(8);
    std::string result_target;
    unsigned pos = 0;
    while (url[pos] != '/') { ++pos; }
    for (unsigned i = pos; i < url.size(); ++i) {
        result_target += url[i];
    }
    return result_target;
}
std::string Crawler::downloader_url(std::string host_,std::string target_){
    std::scoped_lock<std::recursive_mutex> sl{mx};
    //try {
        //std::cout << std::this_thread::get_id() << std::endl;
        auto const host = host_.c_str();
        auto const port = "443";
        auto const target = target_.c_str();
        int version = 11;

        boost::asio::io_context ioc;

        ssl::context ctx{ssl::context::sslv23_client};

        load_root_certificates(ctx);

        tcp::resolver resolver{ioc};
        ssl::stream<tcp::socket> stream{ioc, ctx};

        //if (!SSL_set_tlsext_host_name(stream.native_handle(), host)) {
            //boost::system::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
            //throw boost::system::system_error{ec};
        //}

        auto const results = resolver.resolve(host, port);

        boost::asio::connect(stream.next_layer(), results.begin(), results.end());

        stream.handshake(ssl::stream_base::client);

        http::request<http::string_body> req{http::verb::get, target, version};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        http::write(stream, req);

        boost::beast::flat_buffer buffer;

        http::response<http::string_body> res;

        http::read(stream, buffer, res);

        boost::system::error_code ec;
        stream.shutdown(ec);

            //std::scoped_lock<std::recursive_mutex> sl{mx};
            //if (ec == boost::asio::error::eof) {
               // ec.assign(0, ec.category());
            //}
            //if (ec)
               // throw boost::system::system_error{ec};
            //unsigned count = urlss_.size();
            //std::cout << "test" << std::endl;
            //urlss_.insert(std::pair<std::string, std::string>("https://" + static_cast<std::string>(host) + target,
                                                 //             res.body()));
            //mtx.unlock();
            std::string str = "https://"+static_cast<std::string>(host)+target;
            std::cout<<str<<std::endl;
            //if (urlss_.size() > count)
                return res.body();

    //}catch (std::exception const e){
            //mtx.lock();
         //   std::cerr << e.what() << std::endl;
            //mtx.unlock();
    //}
    return "";
}

void Crawler::parse_url(GumboNode* node) {
    if (node->type != GUMBO_NODE_ELEMENT) {
        return;
    }
    GumboAttribute* src = nullptr;
    if (node->v.element.tag == GUMBO_TAG_IMG &&
        (src = gumbo_get_attribute(&node->v.element.attributes, "src"))) {
        ;
    }

    GumboVector* children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
        parse_url(static_cast<GumboNode*> (children->data[i]));
    }
}
void Crawler::parse_url_for_pict(std::string str,unsigned depth) {
    //static int i = 0;
    std::scoped_lock<std::recursive_mutex> sl{mx1};
    std::future<std::string> str2 =pools.enqueue(&Crawler::downloader_url,this,parse_url_to_host(str),parse_url_to_target(str));
    std::string str1 = str2.get();
    if(str1.size() > 100) {
        parses_pools.enqueue(&Crawler::parse_url, this, gumbo_parse(str1.c_str())->root);
        if(depth > 0) {
            GumboOutput* output = gumbo_parse(str1.c_str());
            do_something(output->root,depth);
            gumbo_destroy_output(&kGumboDefaultOptions, output);
        }
    }
}

void Crawler::do_something(GumboNode* node,unsigned depth){
    if (node->type != GUMBO_NODE_ELEMENT) {
        return;
    }
    GumboAttribute* href = nullptr;
    if(node)
    if (node->v.element.tag == GUMBO_TAG_A &&(href = gumbo_get_attribute(&node->v.element.attributes, "href"))) {
        std::string curr_str = href->value;
        if (curr_str.find("https:") == 0) {
            parse_url_for_pict(curr_str, depth - 1);
        }
    }
    //delete href;
    GumboVector* children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
        do_something(static_cast<GumboNode*>(children->data[i]),depth);
    }
}


void Crawler::downloader(std::string& url, unsigned depth){
    GumboOutput* output = gumbo_parse(downloader_url(parse_url_to_host(url),parse_url_to_target(url)).c_str());
    do_something(output->root,depth);
    gumbo_destroy_output(&kGumboDefaultOptions, output);
}
void Crawler::crawler_start() {

    downloader(url_,depth_);

}

#endif //UNTITLED1_CRAWLER_HPP
