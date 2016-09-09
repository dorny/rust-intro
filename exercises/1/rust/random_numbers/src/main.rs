extern crate rand;

use std::sync::{Arc, Mutex};
use std::sync::atomic::{AtomicBool, Ordering};
use std::thread;
use std::time;

#[derive(Clone)]
struct Item {
    score: i32,
    is_unique: bool
}

impl Item {
    pub fn new(score: i32) -> Item {
        Item {
            score: score,
            is_unique: true
        }
    }

    pub fn set_isunique(&mut self, value: bool) {
        self.is_unique = value;
    }
}

struct ItemTracker {
    first: Option<Item>,
}

impl ItemTracker {
    pub fn new() -> ItemTracker {
        ItemTracker { first: None }
    }

    pub fn process_item (&mut self, mut item: Item) -> bool {
        match self.first {
            None => {
                self.first = Some(item);
                false
            }

            Some(ref mut first_item) => {
                if first_item.score == item.score {
                    first_item.set_isunique(false);
                    item.set_isunique(false);
                    println!("We have found an duplicate of the first item!");
                    true
                }
                else {
                     false
                }
            }
        }
    }
}

fn main() {

    let items_store = Arc::new(Mutex::new(Vec::<Item>::new()));
    let done = Arc::new(AtomicBool::new(false));

    let producer_store = items_store.clone();
    let producer_done = done.clone();

    let producer = thread::spawn(move || {
        while !producer_done.load(Ordering::Relaxed) {
            let mut items = producer_store.lock().unwrap();
            let score = rand::random::<i32>() % 10000;
            items.push(Item::new(score));
        }
    });

    let consumer_store = items_store.clone();
    let consumer = thread::spawn(move || {
        let mut tracker = ItemTracker::new();
        let mut index = 0;

        'consumer: loop {
            std::thread::sleep(time::Duration::from_millis(10));
            let mut items = consumer_store.lock().unwrap();
            for ref mut item in items[index..].iter_mut() {
                if tracker.process_item(item.clone())
                {
                    done.store(true, Ordering::Relaxed);
                    break 'consumer;
                }
            }
            index = items.len();
        }
    });

    producer.join().unwrap();
    consumer.join().unwrap();

    println!("Done");
}