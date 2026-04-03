Description

While monitoring the airwaves, the national security services picked up some messages that piqued their interest. 
These consisted of short and long beeps and brief pauses every so often. The messages were written down on paper—short 
signals with a dot ‘.’, long signals with a horizontal dash ‘-‘, and short pauses with a vertical bar ‘|’. 
The new employees were at a loss for a long time, wondering what it could be. Is it a spy? Maybe aliens?

After prolonged unsuccessful attempts to decipher the strange messages, they consulted a retired employee to see if 
he had ever encountered such a thing. The older employee looked at the notes and chuckled at the younger generation, 
who are obsessed with new IT technologies and know nothing about the good old Morse code anymore.

From memory, he wrote down some of the Morse code decodings in a table:
Letter     Morse code     Number or symbol     Morse code
A 	        .- 	            0 	                -----
B 	        -... 	        1 	                .----
C 	        -.-. 	        2 	                ..---
D 	        -.. 	        3 	                ...--
E 	        . 	            4 	                ....-
F 	        ..-. 	        5 	                .....
G 	        --. 	        6 	                -....
H 	        .... 	        7 	                --...
I 	        .. 	            8 	                ---..
J 	        .--- 	        9 	                ----.
K 	        -.- 	        . 	                .-.-.-
L 	        .-.. 	        , 	                --..--
M 	        -- 	            : 	                ---...
N 	        -. 	            ? 	                ..--..
O 	        --- 	        - 	                -....-
P 	        .--. 	        / 	                -..-.
Q 	        --.- 	        Space ‘ ‘ 	        .----.
R 	        .-. 	        Symbol end 	        |
S 	        ... 		
T 	        - 		
U 	        ..- 		
V 	        ...- 		
W 	        .-- 		
X 	        -..- 		
Y 	        -.-- 		
Z 	        --.. 	

P.S. The text reconstructed from memory may not match the actual Morse code alphabet (https://en.wikipedia.org/wiki/Morse_code), 
for example, in terms of symbol and word spacing.

After attempting to decipher the text a bit, he found that, either due to weak signals or the transcriber’s carelessness, 
not all pauses were marked, or there were too many of them. In addition, he warned that a Morse code chart created from 
memory might be incomplete, meaning that part of the text would remain undeciphered until a complete Morse code chart was found.

The security service immediately turned to LU DF students with a request to write a program that would decode the message. 
All message fragments located between two symbol-ending pauses and not found in the Morse code alphabet must be marked with ‘!’. 
Extra pauses must be ignored. If there is no pause at the end of the text, then the last fragment is unrecognizable.

Input

The file contains only one line, which may be very long (up to 1 MB) and consists solely of the characters ‘.’, ‘-‘, and ‘|’.
Output

Based on the input file, the result is a single line containing the decoded text according to the Morse code alphabet 
specified in the problem. Undecoded segments are marked with the symbol ‘!’.
Example:

Content of the input file morse.in:

...|---|...|.----.|-.... -|.----.|--.|.-..|.-|-...|..|.|-|.----.|--|..-|...|..-|.----.|-..|...-|.|...|.|.-..|.|...|.-.-.-|

Content of the output file morse.out:

SOS – SAVE OUR SOULS.
