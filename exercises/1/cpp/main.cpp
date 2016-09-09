// This is an c++ anti-example
// Sometimes it will ends normally, sometimes it will crash.
// Your task is to find out WHY.
// Good luck ;)

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

// Separate component which process the newly added items.
// It does not have any clue where are the items stored.
class ItemTracker {
private:
    Item* _firstItem = nullptr;

public:

    // Some sample internal logic: it saves pointer to the first item and then
    // checks the following one if they have same score as the first one.
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

    // Random numbers generator
    std::mt19937 rng;
    rng.seed(std::random_device()());
    std::uniform_int_distribution<std::mt19937::result_type> dist(1,100000);

    // Storage of the items
    std::vector<Item> items;

    // Mutex guarding access to Items
    std::mutex mut;

    // Flag signaling we have found a duplicate and we can stop
    bool done = false;

    // Produced thread: it just pushes new items into the vector
    std::thread producer([&] {
        while (!done) {
            mut.lock();
            items.push_back(Item(dist(rng)));
            mut.unlock();
        }
    });

    // Consumer thread: processes newly added items
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
                    // we have found a duplicate and we can end
                    done = true;
                    break;
                }
            }
            index = items.size();
            mut.unlock();
        } while(!done);
    });

    // Wait until producer and consumer thread finishes
    producer.join();
    consumer.join();

    std::cout << "Done" << std::endl;

    return 0;
}