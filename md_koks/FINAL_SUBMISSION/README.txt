Tiek pieņemts, ka personas nevar tikt deklarētas divreiz izmantojot VARDS
tas ir:

"""
VARDS es
TEVS tevs 

(...)

VARDS es
MATE mate 
"""

nav atļauts. Nolūks ir novērst personas ar vienādiem vārdiem, jo 
mēs nevaram zināt, vai tas ir tas pats cilvēks vai kāds cits.

------------------------------------------------------------------------

Programma strādā:
1. nolasot ievaddatus un uzglabājot tikai personu vecākus
2. tad rekursīvi visiem iet cauri un sagrupē tos apakškopās
3. visbeidzot iet caur apakškopām, sakārto tās un izvada

To var ļoti optimizēt, piemēram, ar 'hashmap', bet tā 10 sekundēs apstrādāja 
grafus ar 5000 cilvēkiem un 100 paaudžu dziļumu uz manas ierīces.

------------------------------------------------------------------------
