#include<iostream>
#include<queue>
#include<unordered_map>
#include<functional>
#include<memory>
#include<thread>

struct Request {
    std::function<void()> callback;
};

class Client {
private:
    std::shared_ptr<std::queue<int>> _channel;
    int _requestId = 0;
    std::unordered_map<int,Request> _pendingRequest;
    bool _closed = false;

public:

    Client(std::shared_ptr<std::queue<int>> channel)
        : _channel(channel)
    { }

    bool invoke(int operationId, std::function<void()> onResponse)
    {
        if (_closed) {
            return false;
        }

        int id = ++_requestId;
        _pendingRequest[id] = Request { onResponse };
        _channel->push(id);

        return true;
    }

    void processResponse(int requestId) {
        auto it = _pendingRequest.find(requestId);

        if (it != _pendingRequest.end())
        {
            it->second.callback();
            _pendingRequest.erase(it);
        }
    }

    void close()
    {
        _pendingRequest.clear();
        _closed = true;
    }
};

int main(int argc, char** argv) {

    std::shared_ptr<std::queue<int>> channel(new std::queue<int>());

    bool done = false;

    Client client(channel);

    client.invoke(1, [] { std::cout << "Request 1" << std::endl; });
    client.invoke(2, [] { std::cout << "Request 2" << std::endl; });
    client.invoke(3, [] { std::cout << "Request 3" << std::endl; });
    client.invoke(4, [] { std::cout << "Request 4" << std::endl; });
    client.invoke(5, [&] {
        done = true;
        client.close();
        std::cout << "Closed" << std::endl;
    });


    std::thread server([channel,&client,&done] {
       while (!done) {
           if (channel->size() > 0) {
               int id = channel->front();
               channel->pop();
               client.processResponse(id);
           }
       }
    });

    server.join();

    return 0;
}