/*
morse.c - Morze
Implementation notes:

Create a simple array that represents a binary tree of Morse codes.
Since the Morse code doesn't grow beyond 6 chars, we can have
an array of size 2^7 = 128 to cover all combinations.

Calculate the index for each character based on this formula:
    uint8_t i = 1
    i = i << 1 | (c == '-' ? 1 : 0)
The << 1 is a simple bitshift to the left, which is the same as 
multiplying by 2; then | for bitwise OR to differentiate between '.' and '-'.
Then print the corresponding Morse code.
*/

#include <stdio.h>
#include <stdint.h>

static char morse_table[128];

static void init_morse_table(void)
{
    // Alphabet (1-4 chars)
    morse_table[2]   = 'E';  /* "." */
    morse_table[3]   = 'T';  /* "-" */

    morse_table[4]   = 'I';  /* ".." */
    morse_table[5]   = 'A';  /* ".-" */
    morse_table[6]   = 'N';  /* "-." */
    morse_table[7]   = 'M';  /* "--" */

    morse_table[8]   = 'S';  /* "..." */
    morse_table[9]   = 'U';  /* "..-" */
    morse_table[10]  = 'R';  /* ".-." */
    morse_table[11]  = 'W';  /* ".--" */
    morse_table[12]  = 'D';  /* "-.." */
    morse_table[13]  = 'K';  /* "-.-" */
    morse_table[14]  = 'G';  /* "--." */
    morse_table[15]  = 'O';  /* "---" */

    morse_table[16]  = 'H';  /* "...." */
    morse_table[17]  = 'V';  /* "...-" */
    morse_table[18]  = 'F';  /* "..-." */
    morse_table[20]  = 'L';  /* ".-.." */
    morse_table[22]  = 'P';  /* ".--." */
    morse_table[23]  = 'J';  /* ".---" */
    morse_table[24]  = 'B';  /* "-..." */
    morse_table[25]  = 'X';  /* "-..-" */
    morse_table[26]  = 'C';  /* "-.-." */
    morse_table[27]  = 'Y';  /* "-.--" */
    morse_table[28]  = 'Z';  /* "--.." */
    morse_table[29]  = 'Q';  /* "--.-" */

    // Numbers (5 chars)
    morse_table[32]  = '5';  /* "....." */
    morse_table[33]  = '4';  /* "....-" */
    morse_table[35]  = '3';  /* "...--" */
    morse_table[39]  = '2';  /* "..---" */
    morse_table[47]  = '1';  /* ".----" */
    morse_table[48]  = '6';  /* "-...." */
    morse_table[56]  = '7';  /* "--..." */
    morse_table[60]  = '8';  /* "---.." */
    morse_table[62]  = '9';  /* "----." */
    morse_table[63]  = '0';  /* "-----" */

    // Symbols (5-6 chars)
    morse_table[85]  = '.';  /* ".-.-.-" */
    morse_table[115] = ',';  /* "--..--" */
    morse_table[120] = ':';  /* "---..." */
    morse_table[76]  = '?';  /* "..--.." */
    morse_table[97]  = '-';  /* "-....-" */
    morse_table[50]  = '/';  /* "-..-." */
    morse_table[94]  = ' ';  /* ".----." */
}

int main(void) {
    if (!freopen("morse.in",  "r", stdin))  { perror("morse.in");  return 1; }
    if (!freopen("morse.out", "w", stdout)) { perror("morse.out"); return 1; }

    init_morse_table();

    char c;
    uint8_t idx = 1;
    uint8_t count = 0;

    while (scanf(" %c", &c) == 1) {
        if (c == '.' || c == '-') {
            idx = (idx << 1) | (c == '-');
            /* word too long for given dictionary
            1 (start) + 6 (max word len) = 7; count starts from 0 */
            if (++count > 6) {
                while (scanf(" %c", &c) == 1 && c != '|');
                /* if we haven't read the whole string */
                if (c == '|') {
                    putchar('!');
                    idx = 1;
                    count = 0;
                }
            }
        }
        /* if terminating and having at least one input value */
        else if (c == '|' && idx != 1) {
            char m = morse_table[idx];
            putchar(m ? m : '!');
            idx = 1;
            count = 0;
        }
    }

    /* if the last character is not terminated, treat it as unknown */
    if (count) {
        putchar('!');
    }

    return 0;
}
