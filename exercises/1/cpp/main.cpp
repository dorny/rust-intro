#include<iostream>
#include<vector>
#include<random>
#include<thread>
#include<mutex>

class Item {
private:
    int _score;
    bool _isUnique = true;
public:
    Item(int score)
        : _score(score)
    { }

    int score() { return _score; }
    void setIsUnique(bool value) { _isUnique = value; }
};

class ItemTracker {
private:
    Item* _firstItem = nullptr;

public:
    bool processItem(Item* item) {
        if (_firstItem == nullptr)
        {
            _firstItem = item;
        }
        else if (item->score() == _firstItem->score())
        {
            _firstItem->setIsUnique(false);
            item->setIsUnique(false);
            std::cout << "We have found an duplicate of the first item!" << std::endl;
            return true;
        }

        return false;
    }
};

int main(int argc, char** argv) {

    std::mt19937 rng;
    rng.seed(std::random_device()());
    std::uniform_int_distribution<std::mt19937::result_type> dist(1,100000);

    std::vector<Item> items;
    std::mutex mut;

    bool done = false;

    std::thread producer([&] {
        while (!done) {
            mut.lock();
            items.push_back(Item(dist(rng)));
            mut.unlock();
        }
    });


    std::thread consumer([&] {
        ItemTracker tracker;
        int index = 0;
        do {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            mut.lock();
            for (int i = index; i< items.size(); i++)
            {
                if (tracker.processItem(&items[i]))
                {
                    done = true;
                    break;
                }
            }
            index = items.size();
            mut.unlock();
        } while(!done);
    });


    producer.join();
    consumer.join();

    std::cout << "Done" << std::endl;

    return 0;
}