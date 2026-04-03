Programming Task: Reviewers
Memory: 20 MB
Time: 0.5 seconds
Input file: reviewers.in
Output file: reviewers.out
Description

The editor-in-chief of the journal "All About Science" noticed one day that there were problems with ensuring 
high-quality reviews of all submitted articles. In recent years, science has been developing rapidly in all fields; 
there are many articles, and consequently, many reviewers associated with the editorial board. 
Managing all reviewers without a computerized system has become difficult.

It was decided to create an electronic database to replace the existing card system. 
Each reviewer has a unique name consisting of a string of symbols {a..z, A..Z, 0..9} of length [1..100]. 
To ensure greater anonymity, each reviewer has [1..50] unique pseudonyms. 
Numbers in the range [1..1’000’000’000] were used for the pseudonyms. 
The same pseudonym cannot be assigned to different reviewers; that is, at any given time, 
a specific reviewer can be identified by any of the pseudonyms. 
The journal’s editorial board decided that there would be no more than 10,000 reviewers at any one time.

Since the record-keeping had not been accurate up to that point, it happened that a single reviewer’s pseudonym was 
listed multiple times in the old card system (duplicates in the insertion command), and information about the reviewer 
had to be entered in multiple passes (there may be multiple entry commands for a single reviewer). Pseudonyms are 
not sorted in any way within the entry command, as the old cards were not sorted before being entered into the new system.

An electronic database must be created to enable the following actions regarding reviewers:
    adding reviewers or updating their list of pseudonyms (new reviewers are added, 
        or the list of pseudonyms for an already registered reviewer is updated);
    removal (termination of a reviewer’s services);
    searching (finding a reviewer’s unique name if one of their pseudonyms is known).
After a reviewer is removed from the database, new reviewers can once again use pseudonyms that were previously assigned to a reviewer.
Your task is to program the core of this database, which performs all operations very quickly ( ~O(log(n)) ).
The input is a file that simulates real-life events, i.e., commands to the file system. 
Each command with its parameters is written on a separate line. 
Each line of the file may contain as many trailing spaces as desired. 
The file size is unlimited. The input data is valid according to the input data format and the given constraints.


Input and Output (File System Commands and Operations)

Insertion or Appending to the List of Pseudonyms
The input is a line in the format:
I name count key_1 ... key_count
    I is an abbreviation for Insert
    name is the reviewer’s unique name
    count is the number of aliases [1..50] assigned to the reviewer
    key_1 ... key_count are all aliases separated by spaces
Output a line containing:
    the word “ok” if the reviewer was successfully added or the list of pseudonyms for an existing reviewer was successfully updated,
ok
    the word “no” if the reviewer cannot be added because a pseudonym is already in use
no

Removing a reviewer
Input a line in the following format:
D key
    D is short for Delete
    key is one of the pseudonyms of the reviewer to be removed
Output a line containing:
    the word “ok” if the reviewer has been successfully removed
    ok
    the word “no” if the reviewer cannot be removed because there is no reviewer with the specified pseudonym
    no

Searching for a reviewer
Input: a line in the format:
L key
    L is short for Look_up
    key is one of the nicknames of the reviewer to be searched for
Output: a line containing:
    the reviewer's name, if the reviewer is successfully found,
    Name
    the word “no” if the reviewer cannot be found because there is no reviewer with the specified pseudonym.
    no

Example:
Content of the input file reviewers.in:
I JackSmart 3 9 1009 1000009
L 1000009
I TedPumpkinhead 1 19
I PeterMeter 1 9
L 19
D 19
L 19
I JohnCritic 2 1 19
L 19
L 1
L 9
Content of the output file reviewers.out:
ok
JackSmart
ok
no
TedPumpkinhead
ok
no
ok
JohnCritic
JohnCritic
JackSmart