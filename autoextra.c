#include <stdio.h>
#include <regex.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	if (argc != 3)  {fprintf(stderr, "usage %s music.xml note_duty\n", argv[0]); return -1;}
	FILE *in = fopen (argv[1], "r");
	if (!in) return -1;
	const int dutty = atoi(argv[2]); // 0 -100 %
	int bpm = 120;

	char buffer[512];
	int noteMode = 0;
	char n; //note
	int d; //duration
	int o; //actave
	int s; //sharp
	int quarter_division = 1; //quarter duration
	n = 0; d = 0; o = 0; s = 0;

	regex_t note;
	regex_t step;
	regex_t octave;
	regex_t duration;
	regex_t alter;
	regex_t rest;
	regex_t division;
	regex_t tempo;
	regmatch_t match[2];

	regcomp(&note, ".*note.*", 0*REG_EXTENDED);
	regcomp(&step, ".*<step>\\(.*\\)</step>.*", 0*REG_EXTENDED);
	regcomp(&octave, ".*<octave>\\(.*\\)</octave>.*", 0*REG_EXTENDED);
	regcomp(&duration, ".*<duration>\\(.*\\)</duration>.*", 0*REG_EXTENDED);
	regcomp(&alter, ".*<alter>\\(.*\\)</alter>.*", 0*REG_EXTENDED);
	regcomp(&rest, ".*<rest>\\(.*\\)</rest>.*", 0*REG_EXTENDED);
	regcomp(&division, ".*<divisions>\\(.*\\)</divisions>.*", 0*REG_EXTENDED);
	regcomp(&tempo, ".*<per-minute>\\(.*\\)</per-minute>.*", 0*REG_EXTENDED);

	while (fgets(buffer, 512, in))
	{
		if (regexec(&note, buffer, 2, match, 0) == 0)
		{
			if (noteMode)
			{
				if (n == 0)
				{
					printf("\t{.frequency = SILENT, .duration = %d},\n", 100*d*60/(quarter_division*bpm));
				}
				else
				{
					if (s < 0)
					{
						if (n == 'A') {n = 'G';}
						else if (n == 'C') {n = 'B'; o--;}
						else n--;
						s = 1;
					}
					printf("\t{.frequency = NOTE_%c%s%d, .duration = %d},\n", n, s ? "S" : "", o, 100*d*60*dutty/(quarter_division*bpm*100));
					if (dutty < 100)
						printf("\t{.frequency = SILENT, .duration = %d},\n", 100*d*60*(100-dutty)/(quarter_division*bpm*100));
				}
			}
			n = 0; d = 0; o = 0; s = 0;
			noteMode = !noteMode;
		}
		else if (regexec(&step, buffer, 2, match, 0) == 0) n = buffer[match[1].rm_so];
		else if (regexec(&octave, buffer, 2, match, 0) ==  0) o = atoi(buffer + match[1].rm_so);
		else if (regexec(&duration, buffer, 2, match, 0) ==  0) d = atoi(buffer + match[1].rm_so);
		else if (regexec(&alter, buffer, 2, match, 0) == 0) s = atoi(buffer + match[1].rm_so);
		else if (regexec(&rest, buffer, 2, match, 0) ==  0) n = 0;
		else if (regexec(&division, buffer, 2, match, 0) == 0) quarter_division = atoi(buffer + match[1].rm_so);
		else if (regexec(&tempo, buffer, 2, match, 0) == 0) bpm = atoi(buffer + match[1].rm_so);
	}
	printf("\t{.frequency = SILENT, .duration = 0},\n");

	regfree(&note);
	regfree(&step);
	regfree(&octave);
	regfree(&duration);
	regfree(&alter);
	regfree(&rest);
	regfree(&division);
	regfree(&tempo);
	fclose(in);
	return 0;
}

