// This is an c++ anti-example
// Sometimes it will ends normally, sometimes it will crash.
// Your task is to find out WHY.
// Good luck ;)

#include<iostream>
#include<queue>
#include<unordered_map>
#include<functional>
#include<memory>
#include<thread>
#include<mutex>

// Server response to RPC.
// For purpose of this example, we will just use an empty struct.
struct Response{};

// Client representation of send RPC.
struct Request {

    // Callback will be invoked when server returns response.
    std::function<void(Response)> callback;
};


class Client {
private:
    // Channel to communicate with server.
    std::shared_ptr<std::queue<int>> _channel;

    // Request ID counter. We will just increment it every time, so we have always new unique ID.
    int _requestId = 0;

    // Mutex guarding access to pending requests
    std::mutex _mut;

    // Storage for pending requests.
    // Yes we support multiple request done in parallel.
    std::unordered_map<int,Request> _pendingRequests;

    // Flag that the client was closed.
    bool _closed = false;

public:

    Client(std::shared_ptr<std::queue<int>> channel)
        : _channel(channel)
    { }

    bool invoke(int operationId, std::function<void(Response)> onResponse) {
        std::lock_guard<std::mutex> lock(_mut);

        if (_closed) {
            return false;
        }

        int id = ++_requestId;
        _pendingRequests[id] = Request { onResponse };
        _channel->push(id);

        return true;
    }

    void processResponse(int requestId) {
        std::lock_guard<std::mutex> lock(_mut);

        auto it = _pendingRequests.find(requestId);
        if (it != _pendingRequests.end())
        {
            it->second.callback(Response{});
            _pendingRequests.erase(it);
        }
    }

    void close()
    {
        _pendingRequests.clear();
        _closed = true;
    }
};

int main(int argc, char** argv) {

    std::shared_ptr<std::queue<int>> channel(new std::queue<int>());

    bool done = false;

    Client client(channel);

    std::thread server([channel,&client,&done] {
       while (!done) {
           if (channel->size() > 0) {
               int id = channel->front();
               channel->pop();
               client.processResponse(id);
           }
       }
    });

    std::array<std::thread,5> threads;
    for (int i=0; i<threads.size(); i++) {
        threads[i] = std::thread([&]{
            std::cout << "Invoking request" << i << std::endl;
            client.invoke(i, [&] (Response resp) { std::cout << "Request:" << i << " done" << std::endl; });
        });
    }

    std::cout << "Waiting for RPC..." << std::endl;
    for (auto& t : threads) {
        t.join();
    }

    std::cout << "Last RPC" << std::endl;
    client.invoke(100, [&] (Response resp) {
        done = true;
        client.close();
        std::cout << "Client closed" << std::endl;
    });

    server.join();

    std::cout << "Done" << std::endl;

    return 0;
}
