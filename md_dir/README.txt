Md_dir realizēts izmantojot Hash tabulu, kura uzglabā vienvirziena sarakstu kā vērtību.
MD5 summas aprēķinam tiek izmantota 'libssl-dev' bibliotēka, kuru vajag piesaistīt kompilējot,
izmantojot '-lcrypto' karogu. No tās tiek lietots evp API, kas atbilst mūsdienu standartiem -
openssl/md5 bibliotēka ir novecojusi.

Izdrukas formāts nebija skaidri definēts uzdevuma nosacījumos, tādēļ šeit piemēri ar loģisku izvada darbību.
Tiek uzskaitītas tikai īpašības, kuras ir vienādas starp atrastajiem failiem. Secība: datums, izmērs, nosaukums, md5

Darbības piemēri:
----------------------------------------------------------------------------------------
Izsaucot bez papild karogiem:
----------------------------------------------------------------------------------------
    === 2 yes
    working_directory/test2/same_content_but_diff_time/1/yes 
    working_directory/test2/same_content_but_diff_time/yes

    === 4 okay
    working_directory/okay
    working_directory/test2/okay 
    working_directory/test2/test3/okay
----------------------------------------------------------------------------------------
Izsaucot ar -m:
----------------------------------------------------------------------------------------
    === 48dbcca6ea1112562a59ce59a52933aa
    working_directory/a.out 
    working_directory/MD3

    === 33bc1a3a191a75a22b764f83bac3dbbe
    working_directory/test/1
    working_directory/test/2

    ===b026324c6904b2a9cb4b88d6d61c81d1
    working_directory/test2/same_content_but_diff_time/1/yes 
    working_directory/test2/same_content_but_diff_time/yes

    ===ba1f2511fc30423bdbb183fe33f3dd0f
    working_directory/okay
    working_directory/test2/okay 
    working_directory/test2/test3/okay
----------------------------------------------------------------------------------------
Izsaucot ar -d:
----------------------------------------------------------------------------------------
    === 2026-02-21 13:13 2 count
    working_directory/same_time_diff_hash/1/count 
    working_directory/same_time_diff_hash/count

    === 2026-02-20 21:16 4 okay 
    working_directory/test2/okay 
    working_directory/test2/test3/okay
----------------------------------------------------------------------------------------
Izsaucot ar -m -d:
----------------------------------------------------------------------------------------
    === 2026-02-20 21:16 4 okay ba1f2511fc30423bdbb183fe33f3dd0f 
    working_directory/test2/okay 
    working_directory/test2/test3/okay
----------------------------------------------------------------------------------------
Izsaucot ar jebko nederīgu vai ar -h karogu:
----------------------------------------------------------------------------------------
    Izsaukšana: md3 [-d | -m | -h]
    Apstaigā esošās direktorijas koku un izprintē duplikātu faila atrašanās vietas, ja tādi ir.
    Faili uzskatāmi par vienādiem, ja sakrīt izmērs un nosaukums, izņemot MD5 režīmā, kad salīdzina visu pārējo. 
    Režīmi:
        -d: pārbauda arī satura izmaiņu datumus
        -m: aprēķina MD5 vērtību saturam, neiekļaujot vārdu un datumu 
        -h: izvada šo palīgtekstu.
    Izvades formāts:
    === datums izmērs nosaukums MD5
----------------------------------------------------------------------------------------
