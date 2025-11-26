import threading
import queue
import time
import random

# Shared buffer (thread-safe queue)
buffer = queue.Queue(maxsize=10)

# Track statistics
stats = {
    'produced': 0,
    'consumed': 0,
    'lock': threading.Lock()
}

def producer(producer_id, num_items):
    """Producer thread: Creates items and adds them to the buffer"""
    for i in range(num_items):
        # Simulate production time
        time.sleep(random.uniform(0.1, 0.5))
        
        # Produce an item
        item = f"Item-{producer_id}-{i}"
        
        # Add to buffer (blocks if buffer is full)
        buffer.put(item)
        
        # Update stats
        with stats['lock']:
            stats['produced'] += 1
        
        print(f"🏭 Producer {producer_id}: Produced {item} | Buffer size: {buffer.qsize()}")
    
    print(f"✅ Producer {producer_id}: Finished producing {num_items} items")

def consumer(consumer_id, num_items):
    """Consumer thread: Takes items from buffer and processes them"""
    for i in range(num_items):
        # Get item from buffer (blocks if buffer is empty)
        item = buffer.get()
        
        # Update stats
        with stats['lock']:
            stats['consumed'] += 1
        
        print(f"🛒 Consumer {consumer_id}: Consumed {item} | Buffer size: {buffer.qsize()}")
        
        # Simulate consumption time
        time.sleep(random.uniform(0.2, 0.6))
        
        # Mark task as done
        buffer.task_done()
    
    print(f"✅ Consumer {consumer_id}: Finished consuming {num_items} items")

def main():
    print("=" * 60)
    print("          PRODUCER-CONSUMER PROBLEM")
    print("=" * 60)
    print(f"Buffer Size: {buffer.maxsize}")
    print()
    
    # Configuration
    num_producers = 3
    num_consumers = 2
    items_per_producer = 5
    
    total_items = num_producers * items_per_producer
    items_per_consumer = total_items // num_consumers
    
    # Create producer threads
    producers = []
    for i in range(num_producers):
        p = threading.Thread(target=producer, args=(i, items_per_producer))
        producers.append(p)
    
    # Create consumer threads
    consumers = []
    for i in range(num_consumers):
        c = threading.Thread(target=consumer, args=(i, items_per_consumer))
        consumers.append(c)
    
    # Start all threads
    start_time = time.time()
    
    print("🚀 Starting producers and consumers...\n")
    
    for p in producers:
        p.start()
    
    for c in consumers:
        c.start()
    
    # Wait for all producers to finish
    for p in producers:
        p.join()
    
    # Wait for all consumers to finish
    for c in consumers:
        c.join()
    
    # Wait for all tasks to be completed
    buffer.join()
    
    end_time = time.time()
    
    # Print final statistics
    print()
    print("=" * 60)
    print("                    SUMMARY")
    print("=" * 60)
    print(f"Total items produced: {stats['produced']}")
    print(f"Total items consumed: {stats['consumed']}")
    print(f"Time taken: {end_time - start_time:.2f} seconds")
    print(f"Final buffer size: {buffer.qsize()}")
    print("=" * 60)

if __name__ == "__main__":
    main()