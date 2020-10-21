#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <boost/asio.hpp>
//#include <pthread.h>

#define SO_RCVTIMEO 5;
#define SO_SNDTIMEO 5;

std::string read_(boost::asio::ip::tcp::socket & socket) 
{
        unsigned char* buffer= (unsigned char*)malloc(500000000);
        boost::asio::mutable_buffers_1 buf(buffer, 500000000);
        boost::asio::read(socket, boost::asio::buffer(buf),boost::asio::transfer_at_least(1));
        std::string data = boost::asio::buffer_cast<const char*>(boost::asio::buffer(buf));
        free(buffer);
        return data;
}

std::string read_(boost::asio::ip::tcp::socket & socket, long size) 
{
        unsigned char* buffer= (unsigned char*)malloc(500000000);
        boost::asio::mutable_buffers_1 buf(buffer, 500000000);
        size_t recv = 0;
        while(recv < size)
        {
            recv += boost::asio::read(socket, boost::asio::buffer(buf),boost::asio::transfer_at_least(1));
        }
        std::string data = boost::asio::buffer_cast<const char*>(boost::asio::buffer(buf));
        free(buffer);
        return data;
}

void send_(boost::asio::ip::tcp::socket & socket, const std::string& message)
{
    size_t sent = 0;
    sent += boost::asio::write( socket, boost::asio::buffer((const void*)(message.c_str()), message.size()));
    while(sent < message.size())
    {
        sent += boost::asio::write( socket, boost::asio::buffer((const void*)(message.c_str()+sent), message.size()-sent));
    }
}




class StringMap
{
    std::vector<std::string> keys;
    std::vector<std::string> values;

    public:

    size_t size()
    {
        return keys.size();
    }

    bool contains(std::string key)
    {
        for(int i = 0; i < keys.size(); i++)
        {
            if(keys[i] == key)
            {
                return true;
            }
        }
        return false;
    }

    bool put(std::string key, std::string value)
    {
        if(!contains(key))
        {
            keys.push_back(key);
            values.push_back(value);
            return true;
        }
        return false;
    }

    bool remove(std::string& key)
    {
        for(int i = 0; i < keys.size(); i++)
        {
            if(keys[i] == key)
            {
                keys.erase(keys.begin()+i);
                values.erase(keys.begin()+i);
                return true;
            }
        }
        return false;
    }

    std::string get(std::string key)
    {
        for(int i = 0; i < keys.size(); i++)
        {
            if(keys[i] == key)
            {
                return values[i];
            }
        }
        return "";
    }

    std::string get(long index)
    {
        return values.at(index);
    }

    std::string operator [](long index)
    {
        return values[index];
    }

    std::string keyAt(long index)
    {
        return keys.at(index);
    }

    bool set(std::string existingKey, std::string value)
    {
        for(int i = 0; i < keys.size(); i++)
        {
            if(keys[i] == existingKey)
            {
                values[i] = value;
                return true;
            }
        }
        return false;
    }
};


class HttpRequest
{

    std::string toLowerCase(std::string input)
    {
        for(int i = 0; i < input.length(); i++)
        {
            if(input.at(i) < 91 && input.at(i) > 64)
            {
                input.at(i) += 32;
            }
        }
        return input;
    }

    std::string toUpperCase(std::string input)
    {
        for(int i = 0; i < input.length(); i++)
        {
            if(input.at(i) < 123 && input.at(i) > 96)
            {
                input.at(i) -= 32;
            }
        }
        return input;
    }

public:

    std::string path;
    std::string method;
    StringMap attributes;
    StringMap parameters;

    HttpRequest()
    {

    }

    std::string getQuery()
    {
        std::string result = "";
        for(int i = 0; i < parameters.size(); i++)
        {
            if(i == 0)
            {
                result += '?';
            }
            result += parameters.keyAt(i);
            result += parameters[i];
            if(i + 1 < parameters.size())
            {
                result += "&";
            }
        }
        return result;
    }

    std::string toString()
    {
        std::string generatedHeader = method + " " + path;
        if(parameters.size() > 0 && method == "GET")
        {
            generatedHeader += "?";
            for(int i = 0; i < parameters.size(); i++)
            {
                generatedHeader += parameters.keyAt(i) + "=" + parameters[i];
                if(i < parameters.size()-1)
                {
                    generatedHeader += "&";
                }
            }
        }
        if(parameters.size() > 0 && method == "POST")
        {
            for(int i = 0; i < parameters.size(); i++)
            {
                generatedHeader += parameters.keyAt(i) + "=" + parameters[i];
                if(i < parameters.size()-1)
                {
                    generatedHeader += "&";
                }
            }
        }
        generatedHeader += "\r\n";
        if(attributes.size() > 0 && method == "GET")
        {
            for(int i = 0; i < attributes.size(); i++)
            {
                generatedHeader += attributes.keyAt(i) + ": " + attributes[i];
                generatedHeader += "\r\n";
            }
        }
        generatedHeader += "\r\n";
        return generatedHeader;
    }

    

    HttpRequest(std::string plainRequest)
    {
        int a = 0;
        std::vector<std::string> lines;
        for(int i = 0; i < plainRequest.length(); i++)
        {
            if(plainRequest.at(i) == '\r' && i + 1 < plainRequest.length())
            {
                if(plainRequest.at(i+1) == '\n')
                {
                    lines.push_back(plainRequest.substr(a, i-a));
                    a = i+2;
                }
            }
        }

        for(int i = 0; i < lines.size(); i++)
        {
            if(i > 0)
            {
                std::string var;
                std::string val;
                a = 0;
                for(int o = 0; o < lines.at(i).size(); o++)
                {
                    if(lines.at(i).at(o) == ':')
                    {
                        var = lines.at(i).substr(0, o);
                        a = o + 1;
                    }
                }
                if(a > 1)
                {
                    val = lines.at(i).substr(a, lines.at(i).size());

                    if(val.at(0) == ' ')
                    {
                        val = val.substr(1, val.size());
                    }
                    attributes.put(toLowerCase(var), val);
                }

            }
            else
            {
                a = 0;
                int atr = 0;
                int b = 0;
                for(int o = 0; o < lines.at(0).size(); o++)
                {
                    if(lines.at(0).at(o) == ' ')
                    {
                        if(atr == 0)
                        {
                        method = lines.at(0).substr(a, o-a);
                        method = toUpperCase(method);
                        a = o + 1;
                        }
                        else if(atr == 1)
                        {
                            b = a+1;
                            while(b < lines.at(0).length())
                            {
                                if(lines.at(0).at(b) == ' ')
                                {
                                    break;
                                }
                                b++;
                            }

                            path = lines.at(0).substr(a, b-a);
                            break;
                        }
                        atr++;
                    }
                }

                if(atr > 0)
                {
                    std::vector<std::string> params;
                    a = 0;
                    for(int c = 0; c < path.length(); c++)
                    {
                        if(path.at(c) == '?')
                        {
                            if(c + 1 < path.length())
                            {
                                std::string paras = path.substr(c+1, path.length());
                                path = path.substr(0, c);
                                a = 0;
                                for(int u = 0; u < paras.length(); u++)
                                {
                                    if(paras.at(u) == '&')
                                    {
                                        params.push_back(paras.substr(a, u-a));
                                        a = u + 1;
                                    }
                                }
                                if(a < paras.length())
                                {
                                    params.push_back(paras.substr(a, paras.length()));
                                }
                            }
                            break;
                        }
                    }

                    std::string var;
                    std::string val;
                    for(int c = 0; c < params.size(); c++)
                    {
                        for(int h = 0; h < params.at(c).length(); h++)
                        {
                            if(params.at(c).at(h) == '=')
                            {
                                var = params.at(c).substr(0, h);
                                val = params.at(c).substr(h+1, params.at(c).size());
                                parameters.put(var, val);
                                break;
                            }
                        }
                    }
                }

            }
        }

    }


};

std::string generateResposehead(size_t sizeBytes, std::string type = "text/html")
{
    return "HTTP/1.1 200 OK\r\ncontent-type: "+type+"; charset=utf-8;\r\nContent-Length: "+ std::to_string(sizeBytes) +"\r\nConnection: keep-alive\r\n\r\n";
}

void transfer(boost::asio::ip::tcp::socket & source, boost::asio::ip::tcp::socket & destination, bool response = false, int destport = 80, int targetport = 80)
{
    unsigned char buffer[2048];
    int recv = 0;
    int snd = 0;
    long transferSize = 0;
    long transfered = 0;
    int w = 0;
    long av;
    while (true)
    {
        av = source.available();
        if(av > 2048)
        {
            av = 2048;
        }
        if(av == 0)
        {
            int stp = 0;
            while (av == 0)
            {
                if(stp > 1000)
                {
                    if(transfered == 0)
                    {
                        break;
                    }
                    return;
                }
                usleep(100);
                av = source.available();
                stp ++;
            }
            if(av > 2048)
            {
                av = 2048;
            }
        }
        try
        {
            recv = source.read_some(boost::asio::buffer(buffer, av));
        }
        catch(std::exception e)
        {
            return;
        }
        if(transfered == 0)
        {
            /*std::string buf((const char*)buffer);
            int h = buf.find("Host:") + 7;
            while(buffer[h] != '\r' || buffer[h] != '\n')
            {
                if(buffer[h] == ':')
                {
                    std::string np = std::to_string(targetport);
                    for(int i = 0; i < np.length(); i++)
                    {
                        buffer[h+1+i] = np[i];
                    }
                    break;
                }
                h++;
            }*/

            HttpRequest request(std::string((char*)buffer));
            if(response)
            {
                if(recv > 0)
                {
                    if(request.attributes.contains("content-length"))
                    {
                        printf("%s Bytes %s\n",request.attributes.get("content-length").c_str(), request.attributes.get("content-type").c_str());
                    }
                    else
                    {
                        printf("%d Bytes %s\n",recv, request.attributes.get("content-type").c_str());
                    }
                }
            }
            else if(recv > 0 && request.method == "GET")
            {
                printf("%s %s %s   %s\n",request.method.c_str(), request.path.c_str(), request.getQuery().c_str(), request.attributes.get("content-type").c_str());
            }
            
        }
        if(recv > 0)
        {
            snd = 0;
            try
            {
                while (snd < recv)
                {
                    snd += destination.write_some(boost::asio::buffer(buffer+snd, recv-snd));
                }
            }
            catch(std::exception e)
            {
                return;
            }
            if(snd != -1)
            {
                transfered += snd;
            }
        }
        transferSize += recv;
        /*FILE* f = fopen("communication.txt", "a");
        for(int i = 0; i < recv; i++)
        {
            fputc(buffer[i], f);
        }
        fclose(f);*/
        if(response && recv > 0 && false)
        {
            HttpRequest request(std::string((char*)buffer));
            if(request.attributes.get("content-type").substr(0,9) == "text/html")
            {
                printf("%.1000s\n\n", buffer);
            }
        }
    
    }
        /*if(!request.attributes.contains("content-type"))
        {
            std::string ending = "";
            if(request.path.length() > 3)
            {
                ending = request.path.substr(request.path.size()-4);
            }
            if(ending == ".css")
            {
                type = "text/css";
            }
            else if(ending == ".js")
            {
                type = "application/javascript";
            }
            else if(ending == ".ico")
            {
                type = "image/x-icon";
            }
            else
            {
                type = "text/html";
            }
        }*/
}

/*void* operate(void* input)
{
    boost::asio::ip::tcp::socket* sockets = (boost::asio::ip::tcp::socket*)input;
    transfer(sockets[0], sockets[1]);
    transfer(sockets[1], sockets[0], true);
    sockets[0].close();
    sockets[1].close();
    pthread_exit(NULL);
}*/

int main()
{
    int port = 8082;
    std::string host = "127.0.0.1";
    int targetPort = 8080;

    boost::asio::io_service io_service;

    boost::asio::ip::tcp::endpoint serverEndpoint(boost::asio::ip::tcp::v4(), port);
    boost::asio::ip::tcp::acceptor acceptor(io_service, serverEndpoint);
    //boost::asio::ip::tcp::socket socket(io_service);

    boost::asio::ip::tcp::endpoint target(boost::asio::ip::address::from_string(host), targetPort);

    /*boost::asio::ip::tcp::socket socket(io_service);
    boost::asio::ip::tcp::socket socketClient(io_service);*/

    

    pthread_t threads[8];
    int t = 0;
    while(true)
    {
        boost::asio::ip::tcp::socket sockets[2] = {boost::asio::ip::tcp::socket(io_service), boost::asio::ip::tcp::socket(io_service)};
        acceptor.accept(sockets[0]);
        sockets[1].connect(target);
        transfer(sockets[0], sockets[1]);
        transfer(sockets[1], sockets[0], true);
        sockets[0].close();
        sockets[1].close();
    }

}


