Challenge:

In a large city (with several million inhabitants, but no more than a billion), there is a single hair salon employing a small number of hairdressers [1..9]. Each hairdresser has a unique number [1..9] to facilitate the organization of the salon’s work. Time at the salon is measured in units of time [1..2,000,000,000], and counting began when the salon opened.

Although there are many customers in the city and hairdressers are in high demand, every hairdresser must take a break. A hairdresser’s mandatory break time is when the digit in the hundreds place of the current time matches the hairdresser’s number; that is, a hairdresser with number 5 must take a break during the time intervals [500..599], [1500..1599], [2500..2599], etc. During the break, the hairdresser is strictly prohibited from serving a customer. Furthermore, serving a customer must not be divided into multiple stages; that is, only one hairdresser may serve the customer, and without any interruptions. Thus, a hairdresser must not begin serving a customer if they cannot finish by the time of the break.

A customer must be served immediately if there is an available hairdresser and there are no restrictions on performing this work. Upon finishing work with the current client, the hairdresser must attempt to serve a client at the next available time. More precise specification: client K1 arrives at time T1, and the service takes time A1. There is an available hairdresser F1. Therefore, the service takes place in the time interval [T1..T1+A1−1]. The service ends at time T1+A1−1. If customer K2 has already arrived before or exactly at time T1+A1, then hairdresser F1 can begin serving customer K2 at time T1+A1.

Customers wait in a strict (fair) line. Only one customer may arrive at any given time. The receptionist immediately assigns the customer a queue number [1 .. 200 000] and a service time [1..900].

If there is an unserved customer and several available hairdressers are simultaneously vying for them, then:

    priority is given to the stylist who has been without a client the longest (measured from the end of the previous client’s service);
    if stylists have the same waiting time, priority is given to the stylist with the lowest number.

Given the customers’ arrival times, customer queue numbers, and service durations, print the times when customer service ends, the number of the serving stylist, and the customer’s queue number. The output must be in ascending order of time. If service for multiple customers ends at the same time, the results must be printed in order of hairdresser number.
Input:

The first line contains an integer specifying the number of hairdressers. This is followed by information about the customers in the order of their arrival.

Stylist
Time Customer Duration
...
0

    Stylist specifies the stylist number [1..9]
    Time specifies the time when the customer arrived and is ready for service [1..2,000,000,000]
    Client specifies the client’s queue number [1 .. 200 000]
    Duration specifies the duration of the required service [1..900]
    0 indicates that the input data has ended. In this case, Client and Duration are not specified

The input data is in strict chronological order.

The input data is valid according to the input format and the given constraints.
Output:

Based on the input file, each entry for the end of a customer’s service is in the following format:

Time Hairdresser Customer

    Time specifies the time of the last service provided to the client [1 .. 2 000 000 000]
    Hairdresser specifies the hairdresser who served the client, i.e., the hairdresser’s number [1..9]
    Client specifies the sequence number of the client who was served [1 .. 200 000]

In the results, no service end time will be greater than 2 000 000 000.
Example:

Content of the input file hair.in:

2
11 1 10
21 2 50
31 3 20
0

Content of the output file hair.out:

20 1 1
50 1 3
70 2 2

Additional explanations:
Customers must be served in strict order

There must be no situation where a customer is served before a customer who arrived earlier.
One hairdresser

Content of the input file hair.in:

1
10 1 100
20 2 10
0

Content of the output file hair.out:

299 1 1
309 1 2

Two hairdressers compete at time 300

Content of the input file hair.in:

2
110 1 100
120 2 10
0

Content of the output file hair.out:

299 1 1
309 2 2

Two hairdressers

Content of the input file hair.in:

2
110 1 99
120 2 10
0

Content of the output file hair.out:

298 1 1
308 1 2

Multiple customers can be served simultaneously

It is allowed to start serving multiple customers simultaneously. This is not considered a queue violation.

Two hairdressers become available simultaneously and there are waiting customers

Content of the input file hair.in:

2
1 1 10
2 2 9
3 3 10
4 4 10

Contents of the output file hair.out:

10 1 1
10 2 2
20 1 3
20 2 4

