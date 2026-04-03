Description

Until now, the organization has kept its employees’ personal data in a notebook. 
Each entry for an individual includes their first name, last name, and date of birth. 
Over time, the number of employees grew rapidly, and it became difficult to keep track 
of which employees had upcoming birthdays that needed to be prepared for (greetings and celebrations).

It was decided to implement a computerized record-keeping system. All employees must be registered 
in the new system. Duplicate entries for employees, which had arisen from manually maintaining the 
ledger, must be eliminated. Duplicate entries are considered to be all entries where the first name, 
last name, and date of birth match. Only one record for such a person should be registered in the new system.

After transferring the ledger entries into the system, there will be at least one person in the system, 
i.e., the system database will not be empty. After that, by entering a date, you can retrieve information 
about upcoming celebrations. New employees can be added to the system (duplicates should be ignored and not registered).

Information about the upcoming celebrations means that the nearest date on which one of the employees has a birthday is found. 
It is possible that there are several people celebrating a birthday on this day. 
All found birthday celebrants must be printed in strict order:
    first, the oldest employees;
    if the age is the same, then in alphabetical order by last name;
    if the last names are also the same, then in alphabetical order by first name.

When comparing first and last names, uppercase and lowercase letters are not distinguished. 
For example, the names "Andris", "ANDRIS", and "andris" are considered identical.

A person’s age in full years changes exactly on their birthday. All input data is correct according to the specification. 
Dates will be correct. For simplicity, it is assumed that February 29 does not exist, i.e., all years are of equal length. 
The date of interest for the upcoming celebration will definitely be later than the birth date of the most recently registered person.

Employees are not deleted from the system because, at least for now, 
the company is so ideal that no one wants to leave. And no one dies either.

The system must support up to 10,000 people.

The input data is valid according to the input format and the given constraints.


Input:
Each line of the input file contains one of two commands, where the first allows you to register a new person, 
and the second to retrieve the celebration date and the list of people celebrating:
    P FirstName LastName Date
or
    D Today
FirstName specifies the person’s first name (Latin alphabet characters [a..z, A..Z] of length [1..30])
LastName specifies the person's last name (Latin alphabet characters [a..z, A..Z] of length [1..30])
Date specifies the person's date of birth in the format "dd.mm.yyyy" within the range [01.01.1900 .. 31.12.2099]
Today specifies today's date in the format "dd.mm.yyyy" within the range [01.01.2000 .. 31.12.2099].


Output:
Based on the input file, the output should contain the response to each command D, where the first line 
will print the date of the nearest celebration, followed by the details for each celebrant on a separate line:
    Date 
    Age First Name Last Name
    ...
Date specifies the date of the nearest celebration in the format "dd.mm.yyyy" within the range [01.01.2000 .. 31.12.2099]
Age specifies the person's age on the day of the celebration [1..200]
First Name specifies the person's first name (Latin alphabet letters [a..z, A..Z] of length [1..30])
Last Name specifies the person's last name (Latin alphabet letters [a..z, A..Z] of length [1..30])

Example:

Content of the input file anniversary.in:

P Andris Kalns 05.06.1945
P Ansis Leja 17.11.1967
P Ivo Strazds 29.01.1980
P Ieva Zieds 31.12.1950
D 01.06.2013
D 06.06.2013
P Anna Leja 17.11.1967
P Eduards Garaisciemapagastvecis 17.11.1914
D 01.08.2013
X

Content of the output file anniversary.out:

05.06.2013
68 Andris Kalns
17.11.2013
46 Ansis Leja
17.11.2013
99 Eduards Garaisciemapagastvecis
46 Anna Leja
46 Ansis Leja
