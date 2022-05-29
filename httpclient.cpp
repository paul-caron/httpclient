//Linux
extern "C"{
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
}

//C++ STL
#include <sstream>
#include <iostream>
#include <map>
#include <string>

class Http_client{
    std::string method {"GET"};
    std::string request{};
    std::string response{};
    std::string ressource {"/"};
    std::string host{};
    std::string ip{};
    std::string body{};
    std::string multipart_boundary{"xhiehbabfjfbdbsggsv"};
    std::string multipart_boundary_quoted{"\"xhiehbabfjfbdbsggsv\""};
    
    int port{80};
    int socket_file_descriptor;
    int socket_domain = AF_INET;
    int socket_type = SOCK_STREAM;
    struct sockaddr_in server_address;
    
    std::map<std::string, std::string> request_headers{};
    std::map<std::string, std::string> request_multipart_form_data{};
    
    //timeout polling
    struct pollfd poll_fds[1];
    int timeout_milliseconds{1000};
    
    public:
    Http_client(){}
    void set_host(std::string h){
        host = h;
        request_headers["Host"] = host;
    }
    void set_port(int p){
        port = p;
    }
    void set_ressource(std::string res){
        ressource = res;
    }
    void set_method(std::string m){
        method = m;
    }
    void set_socket_domain(int sd){
        socket_domain = sd;
    }
    void set_timeout_milliseconds(int timeout){
        timeout_milliseconds = timeout;
    }
    std::string get_ip(){
        return ip;
    }
    void set_header(std::string header, std::string value){
        request_headers[header] = value;
    }
    void set_body(std::string b){
        body = b;
    }
    void set_form_data(std::string field_name, std::string value){
        request_multipart_form_data[field_name] = value;
    }
    void build_socket(){
        socket_file_descriptor = socket(socket_domain, SOCK_STREAM, 0);
        server_address.sin_family = socket_domain;
        server_address.sin_port = htons(port);
        inet_pton(socket_domain, ip.c_str(),&server_address.sin_addr);
    }
    void build_request(){
        if(!request_multipart_form_data.empty()){
            set_header("Content-Type", std::string("multipart/form-data;boundary=")+ multipart_boundary_quoted );
            std::stringstream ss_body{};
            for(const auto & fd: request_multipart_form_data){
            ss_body << "--" <<  multipart_boundary << "\r\n"
            << "Content-Disposition: form-data; name="
            << fd.first << "\r\n\r\n"
            << fd.second << "\r\n";
            }
            ss_body << "--" << multipart_boundary << "--\r\n\r\n";
            body = ss_body.str();
            set_header("Content-Length", std::to_string(body.size()));
        }
        std::stringstream ss;
        ss << std::noskipws << method
           << " " << ressource << " HTTP/1.1\r\n";
        for(const auto & p: request_headers){
            ss << p.first << ": " << p.second << "\r\n";
        }
        ss  <<  "\r\n" ;
        
        request = ss.str();
        request = request + body;
    }
    void resolve_ip(){
        struct addrinfo *server_info,*p;
        struct sockaddr_in *h;
        int rv = getaddrinfo(host.c_str(), "http", 0, &server_info);
        if(rv){
            perror("error getaddrinfo");
        }
        for(p=server_info;p!= NULL;p=p->ai_next){
            h = (struct sockaddr_in *) p->ai_addr;
            ip = inet_ntoa(h->sin_addr);
            break;//only need first one IP
        }
        freeaddrinfo(server_info);
    }
    void connect(){
        int status = ::connect(socket_file_descriptor,
                 (struct sockaddr*) &server_address,
                 sizeof server_address);
        if(status<0) perror("Could not connect socket");
    }
    void write(){
        int status = ::write(socket_file_descriptor,
             request.c_str(),
             request.size());
        if(status<0) perror("Could not write to server");
    }
    void read(){
        int bytes{};
        response = "";
        poll_fds[0].fd = socket_file_descriptor ;
        poll_fds[0].events = POLLIN;
        int total_bytes{0};
        while(poll(poll_fds,1,timeout_milliseconds)>0){
            std::string server_response(4096,0);
            bytes = recv(socket_file_descriptor,
                         (char*)server_response.data(),
                         server_response.size(),
                         0);
            if(bytes < 0){
                perror("Could not read from server");
                break;
            }
            if(!bytes) break;
            total_bytes += bytes;
            response = response + server_response;
        }
        response.resize(total_bytes);
    }
    void close_socket(){
        close(socket_file_descriptor);
    }
    struct R{
        std::string response;
        std::string request;
    };
    struct R send(){
        resolve_ip();
        build_socket();
        build_request();
        connect();
        write();
        read();
        close_socket();
        return {response, request};
    }
};




#include <signal.h>

int main(){
    signal(SIGTERM, SIG_IGN);
    //create client
    Http_client client{};
    
    //setting the host
    client.set_host("httpbingo.org");
    
    //setting the url endpoint (optional, default to root "/")
    client.set_ressource("/post");
    
    //setting the port (optional, default 80)
    client.set_port(80);
    
    //setting the method (optional, default GET)
    client.set_method("POST");
    
    //setting timeout (optional, default value 1000)
    client.set_timeout_milliseconds(500);
    
    //setting few headers (optional)
    client.set_header("User-Agent", "Farts");
    client.set_header("Accept","*");
    client.set_header("Accept-Language","*");
    client.set_header("Cross-Origin","*");

    //setting up some form data (optional)
    client.set_form_data("custname","chillpill");
    
    //sending request and receiving response
    auto rr = client.send();
    auto head = rr.response.substr(
                    0,
                    rr.response.find("\r\n\r\n")+4);
    auto body = rr.response.substr(head.size());
    std::cout << "Request\n=======\n" << rr.request
              << "Head\n====\n" << head
              << "Body\n====\n" << body;
    return 0;
}










