#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* izmērs šifrēšanas failam baitos */
#define TABLE_SIZE 256
/* maksimāla izmēra buferis io operācijām */
#define BUF_SIZE   65536
static unsigned char buf[BUF_SIZE];

void build_identity_table(unsigned char table[TABLE_SIZE])
{
    for (int i = 0; i < TABLE_SIZE; i++) {
        table[i] = (unsigned char)i;
    }
}

void build_xor_table(unsigned char table[TABLE_SIZE])
{
    for (int i = 0; i < TABLE_SIZE; i++) {
        table[i] = (unsigned char)(i ^ 0x80);
    }
}

/* nolasa tieši 256 baitus no šifrēšanas faila */
int load_cypher_table(const char *path, unsigned char table[TABLE_SIZE])
{
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "Nevar atvērt: '%s'\n", path);
        return -1;
    }

    size_t n = fread(table, 1, TABLE_SIZE, fp);
    if (n != TABLE_SIZE) {
        /* ja ir vairāk par 256 batiem tos vienkārši ignorē */
        fprintf(stderr, "'%s' jābūt vismaz %d baitiem\n", path, TABLE_SIZE);
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

int load_translation_file(const char *path, unsigned char table[TABLE_SIZE])
{
    /* sāk ar identitātes tabulu, nomaina tos, kuriem ir definēta pāreja */
    build_identity_table(table);

    FILE *fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "Nevar atvērt: '%s'\n", path);
        return -1;
    }

    /* Read entire file into memory for easier parsing */
    if (fseek(fp, 0, SEEK_END) != 0) {
        fprintf(stderr, "Nevar meklēt failā: '%s'\n", path);
        fclose(fp);
        return -1;
    }
    long fsize = ftell(fp);
    if (fsize < 0) {
        fprintf(stderr, "Nevar noteikt faila: '%s' izmēru\n", path);
        fclose(fp);
        return -1;
    }
    rewind(fp);

    if (fsize == 0) {
        fclose(fp);
        return 0;
    }

    unsigned char *buf = malloc((size_t)fsize);
    if (!buf) {
        fprintf(stderr, "Nevar alocēt atmiņu failam: '%s'\n", path);
        fclose(fp);
        return -1;
    }

    size_t got = fread(buf, 1, (size_t)fsize, fp);
    fclose(fp);
    if ((long)got != fsize) {
        fprintf(stderr, "Kļūda lasot failu: '%s'\n", path);
        free(buf);
        return -1;
    }

    unsigned char sep = buf[0];

    unsigned char *p = buf + 1;
    unsigned char *end = buf + fsize;
    /* memchr atrod pirmo char parādību virknē */
    unsigned char *sep1 = memchr(p, sep, (size_t)(end - p)); 
    if (!sep1) {
        fprintf(stderr, "translācijas failā '%s': trūkst otrais atdalītājs\n", path);
        free(buf);
        return -1;
    }

    size_t from_len = (size_t)(sep1 - p);
    unsigned char *from_str = p;
    unsigned char *to_str = sep1 + 1;
    size_t remaining = (size_t)(end - to_str);
    unsigned char sep_replacement_valid = 0;
    unsigned char sep_replacement = 0;

    /* ja beigās ir vēl viens simbols, tad tas ir atdalītāja translācijai */
    if (remaining == from_len) {
        /* nav atdalītājsimbola translācijas */
    } else if (remaining == from_len + 1) {
        sep_replacement_valid = 1;
        sep_replacement = to_str[from_len];
    } else {
        fprintf(stderr, "Translācijas failā '%s' nav atbilstoši garumi starp virknēm\n", path);
        free(buf);
        return -1;
    }

    for (size_t i = 0; i < from_len; i++) {
        table[(unsigned char)from_str[i]] = to_str[i];
    }

    if (sep_replacement_valid) {
        table[sep] = sep_replacement;
    }

    free(buf);
    return 0;
}

/* translē ievaddatus izmantojot tabulu */
int process(FILE *in, FILE *out, const unsigned char table[TABLE_SIZE])
{
    size_t total = 0;
    size_t n;
    while ((n = fread(buf, 1, BUF_SIZE, in)) > 0) {
        for (size_t i = 0; i < n; i++) {
            buf[i] = table[buf[i]]; /* in-place manipulācijas */
        }

        if (fwrite(buf, 1, n, out) != n) {
            fprintf(stderr, "Kļūda rakstot izvaddatus\n");
            return -1;
        }

        total += 2;
    }

    if (ferror(in)) {
        fprintf(stderr, "Kļūda lasot ievaddatus\n");
        return -1;
    }

    printf("Veikti %zu izsaucieni\n", total);
    return 0;
}

void print_usage()
{
    fprintf(stderr,
        "kd1 [-t translation-file] [-s cypher-table] "
        "[-o output-file] [input-file]\n\n"
        "-t  nosaka translācijas failu\n"
        "-s  nosaka šifrēšanas tabulas failu\n"
        "-o  nosaka izvada failu\n"
    );
}

int main(int argc, char *argv[])
{
    const char *t_file = NULL;
    const char *s_file = NULL;
    const char *o_file = NULL;
    const char *in_file = NULL;

    /* apstrādā padotos karogus */
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] != '\0') {
            char flag = argv[i][1];

            switch (flag) {
            case 't': case 's': case 'o':
                if (i + 1 >= argc) {
                    fprintf(stderr, "opcijai -%c nepieciešams arguments\n", flag);
                    return EXIT_FAILURE;
                }
                if (flag == 't') {
                    t_file = argv[++i];
                } else if (flag == 's') {
                    s_file = argv[++i];
                } else {
                    o_file = argv[++i];
                }
                break;
            case 'h':
                print_usage(argv[0]);
                return EXIT_SUCCESS;
            default:
                fprintf(stderr, "Nezināma opcija -%c\n", flag);
                print_usage(argv[0]);
                return EXIT_FAILURE;
            }
        } else {
            if (in_file) {
                fprintf(stderr, "Var būt tikai viens ievaddatu fails\n");
                return EXIT_FAILURE;
            }
            in_file = argv[i];
        }
    }

    if (t_file && s_file) {
        fprintf(stderr, "-t un -s karogu nevar izmantot vienlaicīgi\n");
        return EXIT_FAILURE;
    }

    unsigned char table[TABLE_SIZE];

    if (s_file) {
        if (load_cypher_table(s_file, table) != 0)
            return EXIT_FAILURE;
    } else if (t_file) {
        if (load_translation_file(t_file, table) != 0)
            return EXIT_FAILURE;
    } else {
        build_xor_table(table);
    }

    FILE *in = stdin;
    if (in_file) {
        in = fopen(in_file, "rb");
        if (!in) {
            fprintf(stderr, "Nevar atvērt ievaddatu failu: '%s'\n", in_file);
            return EXIT_FAILURE;
        }
    }

    FILE *out = stdout;
    if (o_file) {
        out = fopen(o_file, "wb");
        if (!out) {
            fprintf(stderr, "Nevar atvērt izvaddatu failu: '%s'\n", in_file);
            return EXIT_FAILURE;
        }
    }

    int rc = process(in, out, table);

    if (in != stdin && fclose(in) != 0) {
        fprintf(stderr, "Kļūda verot ciet ievades plūsmas failu\n");
        rc = -1;
    }
    if (out != stdout && fclose(out) != 0) {
        fprintf(stderr, "Kļūda verot ciet izvades plūsmas failu\n");
        rc = -1;
    }

    return (rc == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}


/*
Iesniedzamie faili:
kd1.c  		– Programmas pirmkods vienā failā
Makefile 	– Nodrošina programmmas kompilāciju
one		– Vienības translēšanas fails
zero		– Nulles translēšanas fails
tran		– Patvaļīgs translēšanas fails
chip		– Patvaļīgs šifrēšanas fails
test.in		– Ievaddatu testa fails
analysis.pdf 	– Komentāri un analīze

Izstrādāt programmu kd1, kas pārveido ieejas datus lietojot translācijas failu (translation-file) vai šifrēšanas tabulu (cypher-table), un ir izsaucama sekojoši:

		kd1 [-t translation-file] [-s cypher-table] 
			[-o output-file] [input-file]

		-t nosaka translācijas failu
		-s nosaka šifrēšanas tabulas failu 
		-o nosaka izvada failu

Ieejas dati jālasa no stdin ja faila vārds nav norādīts parametros (input-file). 
Izejas dati jāraksta uz stdout ja faila vārds nav norādīts parametros (output-file).
Parametrus -t un -s nav paredzēts lietot vienlaicīgi.

Ja nav uzrādīti ne šifrēšanas tabula ne translācijas fails, tad lieto iebūvēto šifrēšanu, kas darbojas sekojoši: ar katru ievada-simbolu izpilda operāciju XOR 0x80, tātad, nomainot katra baita kreiso bitu uz pretējo.


Šifrēšanas fails (cypher-table) ir binārs fails ar 256 baitiem, kas definē šifrēšanas tabulu.

Ievada-simbols tiek šifrēts atrodot atbilstošo baitu tabulā pēc simbola koda. Piemēram, ja simbola 'A' kods ir 65, tad no tabulas tiek paņemts 65. baits (indeksējot no 0) un ar to tiek aizstāti visi ‘A’ kas ir ievaddatos.

Translēšanas fails (translation-file) ir fails, kurā ir divas simbolu virknes, kas norādīs kuri simboli ir tulkojami uz citiem. 

Faila pirmais baits norāda, ar kādu simbolu tiks atdalītas abas simbolu virknes turpmāk failā. Tas var būt jebkuršs simbols, piemēram, jaunas rindas simbols.

Pirmā simbolu virkne satur simbolus kurus jātulko, bet otrā satur jaunos simbolus – uz kuriem tulkot.
Ja jātulko arī pats atdalītājsimbols uz ko citu, tad tā jauno simbolu pievieno pašās faila beigās.

Piemēram, translēšanas fails:

XabcXABCY

nomainītu visus mazos burtus a, b, un c uz attiecīgajiem lielajiem burtiem. Pats atdalītājsimbols X tiks tulkots par Y.

Piezīme: par atdalītājsimbolu varētu būt ērti lietot “newline” (apzīmēts ar ↵), 0x0A simbolu, tad translācijas fails būtu sekojošs: pirmā rinda tukša, tad seko pirmā virkne jaunā rindā, un tad jaunā rindā otrā virkne:

	↵
	abc↵
    ABC↵

Uzdevums: Izstrādāt un testēt programmu, kas realizē šo specifikāciju. 
Izveidot un pievienot arī testa failus, saskaņā ar iepriekšminētajiem nosaukumiem, tai skaitā:
(5%) vienības translēšanas fails (visi simboli katrs uz sevi),
(5%) nulles translēšanas fails (visi simboli uz nulli ’\0’),
(5%) patvaļīga šifrēšanas tabula, 
(5%) patvaļīgs translēšanas fails, 
(5%) ievaddati testam.

Vēlams realizēt pēc iespējas optimālu programmas darbību. 

Komentējiet kodu pēc būtības.

Veiciet kļūdu pārbaudi visos pēc jūsu domām kritiskajos gadījumos. Kodam jābūt pēc iespējas noturīgam pret ievada, parametru un sistēmas kļūmēm.

Vērtējums:
25% - Pievienotie testa faili, kā aprakstīts iepriekš
5% - Saturīgs un komentēts pirmkods kas risina problēmu
5% - Makefile programmas kompilēšanai un testēšanai (darbinot make un make test)
5% - Pirmkods kompilējas bez kļūdām
40% - Atbilst pārbaudes testiem.

//// šī ir vienkārša, bet neefektīva implementācija, kas apstrādā datus pa vienam.
// #include <unistd.h>
// unsigned char b;
// int fd_in = fileno(in);
// int fd_out = fileno(out);
// while (read(fd_in, &b, 1) == 1) {
//     write(fd_out, &table[b], 1);
// }
// return 0;

*/
