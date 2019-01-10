
//#include "C:/boost_1_68_0_VS2017/libs/beast/example/common/root_certificates.hpp" // прописать свой путь
#include "ThreadPool.h"
#include "Crawler.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>

using boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
using namespace boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using namespace boost::beast::http;
namespace po = boost::program_options;
using namespace std;


std::string url;
std::string output;
unsigned depth;
unsigned network_threads;
unsigned parser_threads;


void parse_cmdl(int argc,char* argv[]){

    po::options_description desc{"Options"};
    desc.add_options()
            ("url", po::value<std::string>())
            ("output", po::value<std::string>())
            ("depth",po::value<unsigned>())
            ("network_threads",po::value<unsigned>())
            ("parser_threads",po::value<unsigned>());

    po::variables_map vm;
    store(parse_command_line(argc, argv, desc), vm);
    notify(vm);


    if(vm.count("ulr")) {
        url = static_cast<std::string>(vm["url"].as<std::string>());
    }
    else if(vm.count("output"))
        output = vm["output"].as<std::string>();
    else if(vm.count("depth"))
        depth = vm["depth"].as<unsigned>();
    else if(vm.count("network_threads"))
        network_threads = vm["network_threads"].as<unsigned>();
    else if(vm.count("parser_threads"))
        parser_threads = vm["parser_threads"].as<unsigned>();
}

int main(int argc, char* argv[]){

    parse_cmdl(argc,argv);
    Crawler k("www.yandex.ru/",output,2,2,2);
    k.crawler_start();

    return 0;
}
