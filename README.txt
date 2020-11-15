Team name: SegFaultBusters

Members:
1) Rohit Kundu (203050030)
2) Pramod S Rao (20305R007)
3) Sumit Thorat (203050087)


Commands to run the server and client:

To clean the working directory:
    use command "make clean"

To run the server:
    use command "make server"

To run test client:
    use command "make run_test_client"

Server configurations (server_config.txt):
    - Server port
    - Number of worker threads
    - Number of cache lines
    - Cache replacement policy

Using the server_config.txt:
   - SERVER_PORT : Port number which server will listen to
   - NUM_WORKER_THREADS: Number of worker threads the server will start.
   - CACHE_LEN: The number of cache lines the server will have.
   - REPLACEMENT_POLICY: User can choose, if he wants to start the server based on LRU/LFU policy.

Performance Analysis:
   - We have submitted 4 graph plots in the "Graph plots" directory/folder.
   - The GET and PUT values are very close, hence they overlap in the graph, we have added a legend for each graph
   also, to verify, please check the reports in the "Reports" folder
   - 1st - Throughput vs load (Number of clients), which has 3 lines for 500 reqs/client.
   - 2nd - Response time vs load (Number of clients), which has 3 lines for response time of GET,PUT and DEL
           for 1010 reqs per client per GET and PUT.
   - 3rd - Response time vs load (Number of clients), which has 3 lines for response time of GET,PUT and DEL
           for 2010 reqs per client per GET and PUT.
   - 4th - Response time vs load (Number of clients), which has 3 lines for response time of GET,PUT and DEL
           for 4010 reqs per client per GET and PUT.

    Note: ALSO, ALL THE REPORTS FOR VARIOUS RUNS ARE AVAILABLE IN THE "REPORTS" FOLDER.


Key value store design:

    Server design:
        - The server starts the number of threads mentioned in the server_config.txt file
        - These threads will be assigned incoming connections in a round robin fashion.
        - These threads use epoll to listen to the connected threads
        - As per the problem statement mentioned in the PDF file, Multiple requests for the same key is done atomically. We handle this   		  using a hash table, and in case there are such requests, we add it to a queue and process these requests by dequeuing from the 		  queue.
        - ERROR code: 240, SUCCESS CODE: 200, these are unsigned chars.

    Cache design:
        - The number of cache lines can be configured in server_config.txt
        - The cache utilizes LRU or LFU for the replacement policy.
        - While iterating over the cache, we find the LRU line, required line, or the line needed to be replaced in an O(n)
          where n is the number of cache lines.
        - The cache contains information such as:
            - Dirty line
            - Valid line
            - Key-value
            - Timestamp (For LRU)
            - Frequency (For LFU)
            - Each line has a write and read lock for high concurrency.
    
    GET call design:
        - For any GET call, the request handler first searches the cache.
            - If found, it returns the value immediately and updates the frequency and Timestamp
            - If not, it will then go to the PS and search there. Either success(200) or error of (240)
        - When a get to a request happens, and it is not in the cache, the key-value is brought to the cache
          and response is sent for a higher locality of client requests.
    
    PUT call design:
        - For any PUT call, it is first pushed into the cache and the dirty value is set to 1. This value is later
          pushed into the PS only when it has to be evicted from the cache due to LRU. 
        - The above point elaborates the lazy update technique.

    DEL call design:
        - The key is marked as deleted in the cache line. 
        - It is deleted from the PS.
    
    Persistent storage design:
        - We have 67(prime number) data files in the Persistent storage folder. 
        - We use three custom algorithms to index into the key-value into location of the key value in  the persistent storage. 
        - When a key-value arrives to the persistent storage, the key is first sent to an hashing function
          to find the hash index into our "indexer". Each location at the indexer contains a linked list of inodes.
        - The inode entries contain information to point to locations of the particular key-value in that file.
        - Instead of storing the entire key into the inode, in order to save space, we are creating a digest of 64 bit
          (8 Bytes) to save a lot of space and utilise our memory effectively.
        - Therefore, in order to get a key-value, we create a digest of the key, and compare with the digests in the inodes and then seek 		  into the file and fetch the key-value, and then compare once more the key and fetched key from the Persistent storage to make 		  sure there was no hashing collision. 


KVClient API (Usage):
    IMPORTANT: 
        - Whichever key/val (string) is sent to get/put/del api should be null terminated.
        - Do not add dot character(".") as key or value because it is used as padding.

    int get(key, value, error, socket_fd):
        - char* - key - The key we want to get.
        - char* - value - Value you want to pass.
        - char** - error - If incase of error, this is set.
        - int - socket_fd - Pass the FD of the socket.
    Return value: less than 0 indicates an error with request (Key not found, Invalid message size or error code)
    
    int put(key, value, error, socket_fd):
        - char* - key - The key we want to get.
        - char* - value - Value you want to pass.
        - char** - error - If incase of error, this is set.
        - int - socket_fd - Pass the FD of the socket.
    Return value: less than 0 indicates an error with request (Key not found, Invalid message size or error code)
    
    int del(key, error, socket)
       - char* - key - The key you want to delete.
       - char** - error - In case of error, this value is set to the error message.
       - int - socket - Pass the socket of the connection.
    Return value less than 0 indicates an error with request (Key not found, Invalid message size or error code)

Using the client_config.txt:
   - Mention the server port and the server IP in the first two lines.
   - The NO_OF_GET/NO_OF_PUT/NO_OF_DEL is the number of gets, number of puts and deletes respectively per client.
   - NO_OF_CLIENTS is the number of clients you want to start concurrently
   - NO_OF_INITIAL_ENTRIES is the number of initial entries we want to start off with.
    
    Note(s): !-IMPORTANT-! 
    - PLEASE DON’T HAVE “(NO_OF_DEL * NO_OF_CLIENTS) > NO_OF_INITIAL_ENTRIES” in your configuration.
      This will cause a floating point exception.
    - If you want to increase the number of initial entries more than 1000, please increase the array size in test_client.c at line 	    
      number 28 and then run “make run_test_client”
    
