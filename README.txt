Commands:

To clean the working directory:
    use command "make clean"

To run the server:
    use command "make server"

To run test client:
    use command "make run_test_client"


Key value store design:

    Cache design:
        - The number of cache lines can be configured in server_config.txt
        - The cache utilizes LRU (Least recently used) for the replacement policy.
        - There is also a function for LFU available.
        - While iterating over the cache, we find the LRU line, required line, or the line needed to be replaced in an O(n)
          where n is the number of cache lines.
        - The cache contains information such as:
            - Dirty line
            - Valid line
            - Key-value
            - Timestamp (For LRU)
            - Frequency (For LFU)
            - Each line has a write and read lock.
    
    GET call design:
        - For any GET call, the request handler first searches the cache.
            - If found, it returns the value immediately and updates the frequency and Timestamp
            - If not, it will then go to the PS and search there. Either success(200) or error of (240)

    PUT call design:
        - For any PUT call, the request handler first searches the cache.
            - If found, it updates the value immediately and updates the frequency and Timestamp. And also sets the dirty
              bit to True
            - If not, it will add the entry in an available cache line (if any) or create space for the entry in the cache
              by evicting previous entry.
    
    DEL call design:
        - For any DEL call, the request handler first searches the cache.
            - If found, it makes the cache line available by resetting valid line to False
        - It will then go to PS and search there and remove the entry

    PS design:
        - There is multiple files used for storing the entries into persistent storage. 

Server configurations (server_config.txt):
    - Server port
    - Number of worker threads
    - Number of cache lines

