## Assignment 2 Questions

#### Directions

Please answer the following questions and submit in your repo for the second assignment. Please keep the answers as short and concise as possible.

1. In this assignment I asked you provide an implementation for the `get_student(...)` function because I think it improves the overall design of the database application. After you implemented your solution do you agree that externalizing `get_student(...)` into it's own function is a good design strategy? Briefly describe why or why not.

   > **Answer**: I think yes, because "externalizing" get_student() into its own function is a good design strategy since it reminds me how RISC-V's load-store architecture provides a standardized way to access memory. get_student() will establish consistent interface for retrieving student records, reducing code duplication and centralizing error handling. HOWEVER, its effectiveness depends on the database's underlying implementation since if the database were structured differently than anticipated, the function may not work as expected, so it's better if we have control over its implementation. (P.S. In retrospect, provided the first param is storage and last param is the pointer to the storage, the fun will most probably work regardless of the underlying implementation of the database.)

2. Another interesting aspect of the `get_student(...)` function is how its function prototype requires the caller to provide the storage for the `student_t` structure:

   ```c
   int get_student(int fd, int id, student_t *s);
   ```

   Notice that the last parameter is a pointer to storage **provided by the caller** to be used by this function to populate information about the desired student that is queried from the database file. This is a common convention (called pass-by-reference) in the `C` programming language.

   In other programming languages an approach like the one shown below would be more idiomatic for creating a function like `get_student()` (specifically the storage is provided by the `get_student(...)` function itself):

   ```c
   //Lookup student from the database
   // IF FOUND: return pointer to student data
   // IF NOT FOUND: return NULL
   student_t *get_student(int fd, int id){
       student_t student;
       bool student_found = false;

       //code that looks for the student and if
       //found populates the student structure
       //The found_student variable will be set
       //to true if the student is in the database
       //or false otherwise.

       if (student_found)
           return &student;
       else
           return NULL;
   }
   ```

   Can you think of any reason why the above implementation would be a **very bad idea** using the C programming language? Specifically, address why the above code introduces a subtle bug that could be hard to identify at runtime?

   > **ANSWER:** Oh geez where do I even start here. The first thing that comes to my mind is that it will remove the dynamic nature of student_t. So if we want to add a new field to student_t, we'll have to modify the function itself and recompile the code. This makes it bad design with no scalability in mind. Secondly the returned pointer would be dangling and invalid since it points to the local variable student (on stack btw) which is may be deallocated when the function returns, making any attempt to use that pointer afterwards result in undefined behavior and any undefined behavior is "hard to identify at runtime."

3. Another way the `get_student(...)` function could be implemented is as follows:

   ```c
   //Lookup student from the database
   // IF FOUND: return pointer to student data
   // IF NOT FOUND or memory allocation error: return NULL
   student_t *get_student(int fd, int id){
       student_t *pstudent;
       bool student_found = false;

       pstudent = malloc(sizeof(student_t));
       if (pstudent == NULL)
           return NULL;

       //code that looks for the student and if
       //found populates the student structure
       //The found_student variable will be set
       //to true if the student is in the database
       //or false otherwise.

       if (student_found){
           return pstudent;
       }
       else {
           free(pstudent);
           return NULL;
       }
   }
   ```

   In this implementation the storage for the student record is allocated on the heap using `malloc()` and passed back to the caller when the function returns. What do you think about this alternative implementation of `get_student(...)`? Address in your answer why it work work, but also think about any potential problems it could cause.

   > **ANSWER:** The biggest issue I have with this way of implementation is the unnecessary memory management burden, not only does it force the caller to handle malloc and free, but it risks memory leaks if they forget to free the allocated memory after each get_student() call. Additionally, using heap allocation for every lookup is significantly less efficient than the current approach due to the additional overhead. Finally, not to mention having NULL represent both "student not found" and "memory allocation failed" creates ambiguous error handling making it just harder to maintain or debug.

4. Lets take a look at how storage is managed for our simple database. Recall that all student records are stored on disk using the layout of the `student_t` structure (which has a size of 64 bytes). Lets start with a fresh database by deleting the `student.db` file using the command `rm ./student.db`. Now that we have an empty database lets add a few students and see what is happening under the covers. Consider the following sequence of commands:

   ```bash
   > ./sdbsc -a 1 john doe 345
   > ls -l ./student.db
       -rw-r----- 1 bsm23 bsm23 128 Jan 17 10:01 ./student.db
   > du -h ./student.db
       4.0K    ./student.db
   > ./sdbsc -a 3 jane doe 390
   > ls -l ./student.db
       -rw-r----- 1 bsm23 bsm23 256 Jan 17 10:02 ./student.db
   > du -h ./student.db
       4.0K    ./student.db
   > ./sdbsc -a 63 jim doe 285
   > du -h ./student.db
       4.0K    ./student.db
   > ./sdbsc -a 64 janet doe 310
   > du -h ./student.db
       8.0K    ./student.db
   > ls -l ./student.db
       -rw-r----- 1 bsm23 bsm23 4160 Jan 17 10:03 ./student.db
   ```

   For this question I am asking you to perform some online research to investigate why there is a difference between the size of the file reported by the `ls` command and the actual storage used on the disk reported by the `du` command. Understanding why this happens by design is important since all good systems programmers need to understand things like how linux creates sparse files, and how linux physically stores data on disk using fixed block sizes. Some good google searches to get you started: _"lseek syscall holes and sparse files"_, and _"linux file system blocks"_. After you do some research please answer the following:

   - Please explain why the file size reported by the `ls` command was 128 bytes after adding student with ID=1, 256 after adding student with ID=3, and 4160 after adding the student with ID=64?

     > **ANSWER:** Each time a new student is added, the file size increases by the size of the student record (64 bytes). Formula: `file size = (student id + 1) * (student record size)`.

   - Why did the total storage used on the disk remain unchanged when we added the student with ID=1, ID=3, and ID=63, but increased from 4K to 8K when we added the student with ID=64?

     > **ANSWER:** Linux allocates a new block of 4K for the new student record, and then adds the new student record to the end of the file. The take away is the disk storage is allocated in blocks.

   - Now lets add one more student with a large student ID number and see what happens:

     ```bash
     > ./sdbsc -a 99999 big dude 205
     > ls -l ./student.db
     -rw-r----- 1 bsm23 bsm23 6400000 Jan 17 10:28 ./student.db
     > du -h ./student.db
     12K     ./student.db
     ```

     We see from above adding a student with a very large student ID (ID=99999) increased the file size to 6400000 as shown by `ls` but the raw storage only increased to 12K as reported by `du`. Can provide some insight into why this happened?

     > **ANSWER:** The large file size reported by ls (6400000 bytes) represents the logical size needed to store a student at offset 99999 * 64, but thanks to Linux's sparse file feature  :3, the actual physical storage (12K) only allocates blocks where real data exists, treating the empty spaces between records as "holes" that don't consume disk space. (P.S. This was my favorite thing covered in the lecture since before learning about it, I had no idea how to compress the database without it being lossy due to the records in mem were tied to their address.)
